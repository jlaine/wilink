/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPluginLoader>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>

#include "idle/idle.h"

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppMucIq.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"
#include "qxmpp/QXmppUtils.h"
#include "qxmpp/QXmppVCardManager.h"
#include "qxmpp/QXmppVersionIq.h"

#include "qnetio/dns.h"
#include "chat.h"
#include "chat_dialog.h"
#include "chat_form.h"
#include "chat_plugin.h"
#include "chat_room.h"
#include "chat_roster.h"
#include "chat_roster_item.h"
#include "chat_transfers.h"
#include "systeminfo.h"

#ifdef QT_MAC_USE_COCOA
void application_alert_mac();
#endif

#define AWAY_TIME 300 // set away after 50s

using namespace QNetIO;

static QRegExp jidValidator("[^@]+@[^@]+");

enum StatusIndexes {
    AvailableIndex = 0,
    BusyIndex = 1,
    AwayIndex = 2,
    OfflineIndex = 3,
};

void dumpElement(const QXmppElement &item, int level)
{
    QString pad(level * 2, ' ');
    if (item.value().isEmpty())
        qDebug() << (pad + "*").toAscii().constData() << item.tagName();
    else
        qDebug() << (pad + "*").toAscii().constData() << item.tagName() << ":" << item.value();
    foreach (const QString &attr, item.attributeNames())
        qDebug() << (pad + "  -").toAscii().constData() << attr << ":" << item.attribute(attr);
    QXmppElement child = item.firstChildElement();
    while (!child.isNull())
    {
        dumpElement(child, level+1);
        child = child.nextSiblingElement();
    }
}

ChatClient::ChatClient(QObject *parent)
    : QXmppClient(parent)
{
    connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
    connect(this, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(slotDiscoveryIqReceived(const QXmppDiscoveryIq&)));
}

bool ChatClient::handleStreamElement(const QDomElement &element)
{
    if (element.tagName() == "iq")
    {
        if (QXmppShareGetIq::isShareGetIq(element))
        {
            QXmppShareGetIq getIq;
            getIq.parse(element);
            emit shareGetIqReceived(getIq);
            return true;
        }
        else if (QXmppShareSearchIq::isShareSearchIq(element))
        {
            QXmppShareSearchIq searchIq;
            searchIq.parse(element);
            emit shareSearchIqReceived(searchIq);
            return true;
        }
        else if (QXmppMucOwnerIq::isMucOwnerIq(element))
        {
            QXmppMucOwnerIq mucIq;
            mucIq.parse(element);
            emit mucOwnerIqReceived(mucIq);
            return true;
        }
    }
    return false;
}

void ChatClient::slotConnected()
{
    /* discover services */
    QXmppDiscoveryIq disco;
    disco.setTo(getConfiguration().domain());
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    sendPacket(disco);
}

void ChatClient::slotDiscoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (disco.type() != QXmppIq::Result)
        return;

    if (disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == getConfiguration().domain())
    {
        // root items
        discoQueue.clear();
        foreach (const QXmppDiscoveryIq::Item &item, disco.items())
        {
            if (!item.jid().isEmpty() && item.node().isEmpty())
            {
                discoQueue.append(item.jid());
                // get info for item
                QXmppDiscoveryIq info;
                info.setQueryType(QXmppDiscoveryIq::InfoQuery);
                info.setTo(item.jid());
                sendPacket(info);
            }
        }
    }
    else if (disco.queryType() == QXmppDiscoveryIq::InfoQuery &&
             discoQueue.contains(disco.from()))
    {
        discoQueue.removeAll(disco.from());
        // check if it's a conference server
        foreach (const QXmppDiscoveryIq::Identity &id, disco.identities())
        {
            if (id.category() == "conference" &&
                id.type() == "text")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found chat room server " + disco.from());
                emit mucServerFound(disco.from());
            }
            else if (id.category() == "proxy" &&
                     id.type() == "bytestreams")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found bytestream proxy " + disco.from());
                getTransferManager().setProxy(disco.from());
            }
            else if (id.category() == "store" &&
                     id.type() == "file")
            {
                logger()->log(QXmppLogger::InformationMessage, "Found share server " + disco.from());
                emit shareServerFound(disco.from());
            }
        }
    }
}

Chat::Chat(QSystemTrayIcon *trayIcon)
    : autoAway(false),
    isBusy(false),
    isConnected(false),
    systemTrayIcon(trayIcon)
{
    client = new ChatClient(this);
    rosterModel =  new ChatRosterModel(client);

    /* set up idle monitor */
    idle = new Idle;
    connect(idle, SIGNAL(secondsIdle(int)), this, SLOT(secondsIdle(int)));
    idle->start();

    /* set up logger */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    client->setLogger(logger);

    /* set up transfers window */
    client->getTransferManager().setSupportedMethods(
        QXmppTransferJob::SocksMethod);
    chatTransfers = new ChatTransfers(client);
    chatTransfers->setObjectName("transfers");
    connect(chatTransfers, SIGNAL(hidePanel()), this, SLOT(hidePanel()));
    connect(chatTransfers, SIGNAL(registerPanel()), this, SLOT(registerPanel()));
    connect(chatTransfers, SIGNAL(showPanel()), this, SLOT(showPanel()));

    /* build splitter */
    splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    /* left panel */
    rosterView = new ChatRosterView(rosterModel);
    connect(rosterView, SIGNAL(itemAction(int, const QString&, int)), this, SLOT(rosterAction(int, const QString&, int)));
    connect(rosterView->model(), SIGNAL(modelReset()), this, SLOT(resizeContacts()));
    splitter->addWidget(rosterView);
    splitter->setStretchFactor(0, 0);

    /* right panel */
    conversationPanel = new QStackedWidget;
    conversationPanel->hide();
    connect(conversationPanel, SIGNAL(currentChanged(int)), this, SLOT(panelChanged(int)));
    splitter->addWidget(conversationPanel);
    splitter->setStretchFactor(1, 1);

    /* build status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);

    addButton = new QPushButton;
    addButton->setEnabled(false);
    addButton->setIcon(QIcon(":/add.png"));
    addButton->setToolTip(tr("Add a contact"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    hbox->addWidget(addButton);

    roomButton = new QPushButton;
    roomButton->setEnabled(false);
    roomButton->setIcon(QIcon(":/chat.png"));
    roomButton->setToolTip(tr("Join a chat room"));
    connect(roomButton, SIGNAL(clicked()), this, SLOT(addRoom()));
    hbox->addWidget(roomButton);

    hbox->addStretch();

    statusCombo = new QComboBox;
    statusCombo->addItem(QIcon(":/contact-available.png"), tr("Available"));
    statusCombo->addItem(QIcon(":/contact-busy.png"), tr("Busy"));
    statusCombo->addItem(QIcon(":/contact-away.png"), tr("Away"));
    statusCombo->addItem(QIcon(":/contact-offline.png"), tr("Offline"));
    statusCombo->setCurrentIndex(OfflineIndex);
    connect(statusCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(statusChanged(int)));
    hbox->addWidget(statusCombo);

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(splitter, 1);
    layout->addLayout(hbox);

    setLayout(layout);
    setWindowIcon(QIcon(":/chat.png"));

    /* set up client */
    connect(client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(mucOwnerIqReceived(const QXmppMucOwnerIq&)), this, SLOT(mucOwnerIqReceived(const QXmppMucOwnerIq&)));
    connect(client, SIGNAL(mucServerFound(const QString&)), this, SLOT(mucServerFound(const QString&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_T), this);
    connect(shortcut, SIGNAL(activated()), chatTransfers, SIGNAL(showPanel()));
#ifdef Q_OS_MAC
    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_W), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
#endif
}

Chat::~Chat()
{
    delete idle;
    delete chatTransfers;
    foreach (ChatPanel *panel, chatPanels)
        delete panel;

    // disconnect
    client->disconnect();
}

/** Prompt the user for a new contact then add it to the roster.
 */
void Chat::addContact()
{
    bool ok = true;
    QString jid = "@" + client->getConfiguration().domain();
    while (!jidValidator.exactMatch(jid))
    {
        jid = QInputDialog::getText(this, tr("Add a contact"),
            tr("Enter the address of the contact you want to add."),
            QLineEdit::Normal, jid, &ok).toLower();
        if (!ok)
            return;
        jid = jid.trimmed().toLower();
    }

    QXmppPresence packet;
    packet.setTo(jid);
    packet.setType(QXmppPresence::Subscribe);
    client->sendPacket(packet);
}

/** Add a panel or show it.
 */
void Chat::addPanel(QWidget *panel)
{
    if (conversationPanel->indexOf(panel) >= 0)
        return;

    conversationPanel->addWidget(panel);
    conversationPanel->show();
    if (conversationPanel->count() == 1)
        resizeContacts();
}

void Chat::hidePanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (conversationPanel->indexOf(panel) < 0)
        return;

    // close view
    removePanel(panel);

    // cleanup
    ChatConversation *conversation = qobject_cast<ChatConversation*>(panel);
    if (conversation)
    {
        if (qobject_cast<ChatRoom*>(conversation))
            rosterModel->removeItem(conversation->objectName());
        conversation->leave();
        conversation->deleteLater();
    }
}

/** Notify the user of activity on a panel.
 */
void Chat::notifyPanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (conversationPanel->indexOf(panel) < 0)
        return;

    // add pending message
    if (!isActiveWindow() || conversationPanel->currentWidget() != panel)
        rosterModel->addPendingMessage(panel->objectName());

    // show the chat window
    if (!isVisible())
    {
#ifdef Q_OS_MAC
        show();
#else
        showMinimized();
#endif
    }

    /* NOTE : in Qt built for Mac OS X using Cocoa, QApplication::alert
     * only causes the dock icon to bounce for one second, instead of
     * bouncing until the user focuses the window. To work around this
     * we implement our own version.
     */
#ifdef QT_MAC_USE_COCOA
    application_alert_mac();
#else
    QApplication::alert(panel);
#endif
}

/** Remove a panel.
 */
void Chat::removePanel(QWidget *panel)
{
    if (conversationPanel->indexOf(panel) < 0)
        return;

    // close view
    if (conversationPanel->count() == 1)
    {
        conversationPanel->hide();
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    conversationPanel->removeWidget(panel);
}

/** Register a panel in the roster list.
  */
void Chat::registerPanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (!panel)
        return;
    ChatRosterItem::Type type = ChatRosterItem::Other;
    if (qobject_cast<ChatRoom*>(panel))
        type = ChatRosterItem::Room;
    rosterModel->addItem(type,
        panel->objectName(),
        panel->windowTitle(),
        panel->windowIcon());
}

/** Show a panel.
 */
void Chat::showPanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (!panel)
        return;
    rosterModel->addItem(ChatRosterItem::Other,
        panel->objectName(),
        panel->windowTitle(),
        panel->windowIcon());
    addPanel(panel);
    conversationPanel->setCurrentWidget(panel);
}

void Chat::panelChanged(int index)
{
    QWidget *widget = conversationPanel->widget(index);
    if (!widget)
        return;
    rosterModel->clearPendingMessages(widget->objectName());
    rosterView->selectContact(widget->objectName());
}

/** Prompt the user for a new group chat then join it.
 */
void Chat::addRoom()
{
    ChatRoomPrompt prompt(client, chatRoomServer, this);
    if (!prompt.exec())
        return;
    joinConversation(prompt.textValue(), true);
}

/** When the window is activated, pass focus to the active chat.
 */
void Chat::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
    {
        QWidget *widget = conversationPanel->currentWidget();
        if (widget)
        {
            rosterModel->clearPendingMessages(widget->objectName());
            widget->setFocus();
        }
    }
}

void Chat::connected()
{
    qWarning() << "Connected to chat server" << client->getConfiguration().host();
    isConnected = true;
    addButton->setEnabled(true);
    statusCombo->setCurrentIndex(isBusy ? BusyIndex : AvailableIndex);
}

/** Create a conversation dialog for the specified recipient.
 *
 * @param jid
 */
ChatConversation *Chat::createConversation(const QString &jid, bool room)
{
    ChatConversation *dialog;
    if (room)
    {
        rosterModel->addItem(ChatRosterItem::Room, jid);
        dialog  = new ChatRoom(client, jid);
    } else {
        dialog = new ChatDialog(client, rosterModel, jid);
        dialog->setWindowIcon(rosterModel->contactAvatar(jid));
    }
    dialog->setObjectName(jid);
    dialog->setLocalName(rosterModel->ownName());
    dialog->setRemoteName(rosterModel->contactName(jid));
    connect(dialog, SIGNAL(hidePanel()), this, SLOT(hidePanel()));
    connect(dialog, SIGNAL(notifyPanel()), this, SLOT(notifyPanel()));
    connect(dialog, SIGNAL(showPanel()), this, SLOT(showPanel()));
    QTimer::singleShot(0, dialog, SIGNAL(showPanel()));
    return dialog;
}

void Chat::disconnected()
{
    qWarning("Disconnected from chat server");
    isConnected = false;
    addButton->setEnabled(false);
    roomButton->setEnabled(false);
    statusCombo->setCurrentIndex(OfflineIndex);
}

void Chat::error(QXmppClient::Error error)
{
    if(error == QXmppClient::XmppStreamError &&
       client->getXmppStreamError() == QXmppClient::ConflictStreamError)
    {
        // if we received a resource conflict, exit
        qWarning("Received a resource conflict from chat server");
        qApp->quit();
    }
}

void Chat::inviteContact(const QString &jid)
{
    ChatRoomPrompt prompt(client, chatRoomServer, this);
    if (!prompt.exec())
        return;

    // join chat room
    const QString roomJid = prompt.textValue();
    joinConversation(roomJid, true);

    // invite contact
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_conference);
    x.setAttribute("jid", roomJid);
    x.setAttribute("reason", "Let's talk");

    QXmppMessage message;
    message.setTo(jid);
    message.setType(QXmppMessage::Normal);
    message.setExtensions(x);
    client->sendPacket(message);
}

void Chat::joinConversation(const QString &jid, bool isRoom)
{
    ChatConversation *dialog = conversationPanel->findChild<ChatConversation*>(jid);
    if (!dialog)
        dialog = createConversation(jid, isRoom);

    conversationPanel->setCurrentWidget(dialog);
    dialog->setFocus();
}

void Chat::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.from());

    switch (msg.type())
    {
    case QXmppMessage::Normal:
        foreach (const QXmppElement &extension, msg.extensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
            {
                const QString contactName = rosterModel->contactName(bareJid);
                const QString roomJid = extension.attribute("jid");
                if (!roomJid.isEmpty() && !conversationPanel->findChild<ChatRoom*>(roomJid))
                {
                    ChatRoomInvitePrompt *dlg = new ChatRoomInvitePrompt(contactName, roomJid, this);
                    connect(dlg, SIGNAL(itemAction(int, const QString&, int)), this, SLOT(rosterAction(int, const QString&, int)));
                    dlg->show();
                }
                break;
            }
        }
        break;
    case QXmppMessage::Chat:
        if (!conversationPanel->findChild<ChatDialog*>(bareJid) && !msg.body().isEmpty())
        {
            ChatDialog *dialog = qobject_cast<ChatDialog*>(createConversation(bareJid, false));
            dialog->messageReceived(msg);
        }
        break;
    default:
        break;
    }
}

void Chat::mucOwnerIqReceived(const QXmppMucOwnerIq &iq)
{
    if (iq.type() != QXmppIq::Result || iq.form().isNull())
        return;

    ChatForm dialog(iq.form(), this);
    if (dialog.exec())
    {
        QXmppMucOwnerIq iqPacket;
        iqPacket.setType(QXmppIq::Set);
        iqPacket.setTo(iq.from());
        iqPacket.setForm(dialog.form());
        client->sendPacket(iqPacket);
    }
}

void Chat::mucServerFound(const QString &mucServer)
{
    chatRoomServer = mucServer;
    roomButton->setEnabled(true);
}

void Chat::presenceReceived(const QXmppPresence &presence)
{
    QXmppPresence packet;
    packet.setTo(presence.from());
    const QString bareJid = jidToBareJid(presence.from());

    switch (presence.getType())
    {
    case QXmppPresence::Subscribe:
        {
            QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(presence.from());
            QXmppRoster::QXmppRosterEntry::SubscriptionType type = entry.subscriptionType();

            /* if the contact is in our roster accept subscribe */
            if (type == QXmppRoster::QXmppRosterEntry::To || type == QXmppRoster::QXmppRosterEntry::Both)
            {
                qDebug("Subscribe accepted");
                packet.setType(QXmppPresence::Subscribed);
                client->sendPacket(packet);

                packet.setType(QXmppPresence::Subscribe);
                client->sendPacket(packet);
                return;
            }

            /* otherwise ask user */
            ChatRosterPrompt *dlg = new ChatRosterPrompt(client, presence.from(), this);
            dlg->show();
        }
        break;
    default:
        break;
    }
}

ChatClient *Chat::chatClient()
{
    return client;
}

ChatRosterModel *Chat::chatRosterModel()
{
    return rosterModel;
}

/** Open the connection to the chat server.
 *
 * @param jid
 * @param password
 */
bool Chat::open(const QString &jid, const QString &password, bool ignoreSslErrors)
{
    QXmppConfiguration config;
    config.setResource(qApp->applicationName());

    /* get user and domain */
    if (!jidValidator.exactMatch(jid))
    {
        qWarning("Cannot connect to chat server using invalid JID");
        return false;
    }
    QStringList bits = jid.split("@");
    config.setPasswd(password);
    config.setUser(bits[0]);
    config.setDomain(bits[1]);

    /* get the server */
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.domain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.domain());
    }

    /* set security parameters */
    config.setAutoAcceptSubscriptions(false);
    config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
    config.setIgnoreSslErrors(ignoreSslErrors);

    /* set keep alive */
    config.setKeepAliveInterval(60);
    config.setKeepAliveTimeout(15);

    /* connect to server */
    client->connectToServer(config);

    /* load plugins */
    QObjectList plugins = QPluginLoader::staticInstances();
    foreach (QObject *object, plugins)
    {
        ChatPlugin *plugin = qobject_cast<ChatPlugin*>(object);
        if (!plugin)
            continue;
        ChatPanel *panel = plugin->createPanel(this);
        if (!panel)
            continue;
        chatPanels << panel;
        connect(panel, SIGNAL(hidePanel()), this, SLOT(hidePanel()));
        connect(panel, SIGNAL(notifyPanel()), this, SLOT(notifyPanel()));
        connect(panel, SIGNAL(registerPanel()), this, SLOT(registerPanel()));
        connect(panel, SIGNAL(showPanel()), this, SLOT(showPanel()));
    }
    return true;
}

/** Prompt the user for confirmation then remove a contact.
 *
 * @param jid
 */
void Chat::removeContact(const QString &jid)
{
    if (QMessageBox::question(this, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        QXmppRosterIq::Item item;
        item.setBareJid(jid);
        item.setSubscriptionType(QXmppRosterIq::Item::Remove);
        QXmppRosterIq packet;
        packet.setType(QXmppIq::Set);
        packet.addItem(item);
        client->sendPacket(packet);
    }
}

/** Prompt the user to rename a contact.
 *
 * @param jid
 */
void Chat::renameContact(const QString &jid)
{
    bool ok = true;
    QXmppRosterIq::Item item = client->getRoster().getRosterEntry(jid);
    QString name = QInputDialog::getText(this, tr("Rename contact"),
        tr("Enter the name for this contact."),
        QLineEdit::Normal, item.name(), &ok);
    if (ok)
    {
        item.setName(name);
        QXmppRosterIq packet;
        packet.setType(QXmppIq::Set);
        packet.addItem(item);
        client->sendPacket(packet);
    }
}

/** Try to resize the window to fit the contents of the contacts list.
 */
void Chat::resizeContacts()
{
    QSize hint = rosterView->sizeHint();
    hint.setHeight(hint.height() + 32);
    if (conversationPanel->isVisible())
        hint.setWidth(hint.width() + 500);

    // respect current size
    if (conversationPanel->isVisible() && hint.width() < size().width())
        hint.setWidth(size().width());
    if (hint.height() < size().height())
        hint.setHeight(size().height());

    /* Make sure we do not resize to a size exceeding the desktop size
     * + some padding for the window title.
     */
    QDesktopWidget *desktop = QApplication::desktop();
    const QRect &available = desktop->availableGeometry(this);
    if (hint.height() > available.height() - 100)
    {
        hint.setHeight(available.height() - 100);
        hint.setWidth(hint.width() + 32);
    }

    resize(hint);
}

void Chat::rosterAction(int action, const QString &jid, int type)
{
    if (type == ChatRosterItem::Contact)
    {
        if (action == ChatRosterView::InviteAction)
            inviteContact(jid);
        else if (action == ChatRosterView::JoinAction)
            joinConversation(jid, false);
        else if (action == ChatRosterView::OptionsAction)
        {
            ChatRosterItem *item = rosterModel->contactItem(jid);
            QString url = item ? item->data(ChatRosterModel::UrlRole).toString() : QString();
            if (!url.isEmpty())
                QDesktopServices::openUrl(url);

#if 0
            QStringList fullJids = rosterModel->contactFeaturing(jid, ChatRosterModel::VersionFeature);
            if (!fullJids.size())
                return;

            QXmppVersionIq iq;
            iq.setType(QXmppIq::Get);
            iq.setTo(fullJids.first());
            client->sendPacket(iq);
#endif
        }
        else if (action == ChatRosterView::RemoveAction)
            removeContact(jid);
        else if (action == ChatRosterView::RenameAction)
            renameContact(jid);
        else if (action == ChatRosterView::SendAction)
        {
            // find first resource supporting file transfer
            QStringList fullJids = rosterModel->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
            if (fullJids.size())
                chatTransfers->sendFile(fullJids.first());
       }
    } else if (type == ChatRosterItem::Room) {
        if (action == ChatRosterView::JoinAction)
            joinConversation(jid, true);
        else if (action == ChatRosterView::OptionsAction)
        {
            // get room info
            QXmppIq iq;
            QXmppElement query;
            query.setTagName("query");
            query.setAttribute("xmlns", ns_muc_owner);
            iq.setExtensions(query);
            iq.setTo(jid);
            client->sendPacket(iq);
        }
        else if (action == ChatRosterView::MembersAction)
        {
            ChatRoomMembers dialog(client, jid, this);
            dialog.exec();
        }
    } else if (type == ChatRosterItem::RoomMember) {
        if (action == ChatRosterView::RemoveAction)
        {
            QXmppElement item;
            item.setTagName("item");
            item.setAttribute("nick", jidToResource(jid));
            item.setAttribute("role", "none");

            QXmppElement query;
            query.setTagName("query");
            query.setAttribute("xmlns", ns_muc_admin);
            query.appendChild(item);

            QXmppIq iq(QXmppIq::Set);
            iq.setTo(jidToBareJid(jid));
            iq.setExtensions(query);

            client->sendPacket(iq);
        }
    } else if (type == ChatRosterItem::Other) {
        if (action == ChatRosterView::JoinAction)
        {
            foreach (ChatPanel *panel, chatPanels)
            {
                if (panel->objectName() == jid)
                {
                    QTimer::singleShot(0, panel, SIGNAL(showPanel()));
                    break;
                }
            }
        }
    }
}

void Chat::secondsIdle(int secs)
{
    if (secs >= AWAY_TIME)
    {
        if (statusCombo->currentIndex() == AvailableIndex)
        {
            statusCombo->setCurrentIndex(AwayIndex);
            autoAway = true;
        }
    } else if (autoAway) {
        const int oldIndex = isBusy ? BusyIndex : AvailableIndex;
        statusCombo->setCurrentIndex(oldIndex);
    }
}

void Chat::statusChanged(int currentIndex)
{
    autoAway = false;
    if (currentIndex == AvailableIndex)
    {
        isBusy = false;
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            client->sendPacket(presence);
        }
        else
            client->connectToServer(client->getConfiguration());
    } else if (currentIndex == AwayIndex) {
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            presence.setStatus(QXmppPresence::Status::Away);
            client->sendPacket(presence);
        }
    } else if (currentIndex == BusyIndex) {
        isBusy = true;
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            presence.setStatus(QXmppPresence::Status::DND);
            client->sendPacket(presence);
        }
        else
            client->connectToServer(client->getConfiguration());
    } else if (currentIndex == OfflineIndex) {
        if (isConnected)
            client->disconnect();
    }
}

