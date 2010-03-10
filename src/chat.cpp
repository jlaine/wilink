/*
 * wDesktop
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
#include <QDesktopWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextBrowser>
#include <QTimer>

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppDiscoveryIq.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppPingIq.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppTransferManager.h"
#include "qxmpp/QXmppUtils.h"
#include "qxmpp/QXmppVCardManager.h"

#include "qnetio/dns.h"
#include "chat.h"
#include "chat_console.h"
#include "chat_dialog.h"
#include "chat_form.h"
#include "chat_room.h"
#include "chat_roster.h"
#include "chat_roster_item.h"
#include "chat_transfers.h"
#include "systeminfo.h"

#ifdef QT_MAC_USE_COCOA
void application_alert_mac();
#endif

using namespace QNetIO;

static QRegExp jidValidator("[^@]+@[^@]+");

static qint64 fileSizeLimit = 10000000; // 10 MB

enum StatusIndexes {
    AvailableIndex = 0,
    BusyIndex = 1,
    OfflineIndex = 2,
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

Chat::Chat(QSystemTrayIcon *trayIcon)
    : chatConsole(0),
    isBusy(false),
    isConnected(false),
    reconnectOnDisconnect(false),
    systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);
    rosterModel =  new ChatRosterModel(client);

    /* set up logger */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SIGNAL);
    client->setLogger(logger);

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

    /* set up transfers window */
    client->getTransferManager().setSupportedMethods(
        QXmppTransferJob::SocksMethod);
    chatTransfers = new ChatTransfers;
    chatTransfers->setObjectName("transfers");
    connect(chatTransfers, SIGNAL(openTab()), this, SLOT(showTransfers()));
    connect(chatTransfers, SIGNAL(closeTab()), this, SLOT(hideTransfers()));

    /* set up client */
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(&client->getTransferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
            chatTransfers, SLOT(fileReceived(QXmppTransferJob*)));

    /* set up timers */
    pingTimer = new QTimer(this);
    pingTimer->setInterval(60000);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(15000);
    connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_J), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(showTransfers()));
    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_D), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(showConsole()));
#ifdef Q_OS_MAC
    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_W), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
#endif
}

Chat::~Chat()
{
    // disconnect
    client->disconnect();

    delete chatTransfers;
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
            widget->setFocus();
    }
}

void Chat::connected()
{
    qWarning("Connected to chat server");
    isConnected = true;
    addButton->setEnabled(true);
    pingTimer->start();
    statusCombo->setCurrentIndex(isBusy ? BusyIndex : AvailableIndex);

    /* discover services */
    QXmppDiscoveryIq disco;
    disco.setTo(client->getConfiguration().domain());
    disco.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    client->sendPacket(disco);

    /* re-join conversations */
    QTimer::singleShot(500, this, SLOT(rejoinConversations()));
}

/** Create a conversation dialog for the specified recipient.
 *
 * @param jid
 */
ChatConversation *Chat::createConversation(const QString &jid, bool room)
{
    ChatConversation *dialog;
    if (room)
        dialog  = new ChatRoom(client, jid);
    else
        dialog = new ChatDialog(client, rosterModel, jid);
    dialog->setObjectName(jid);
    dialog->setLocalName(rosterModel->ownName());
    dialog->setRemoteName(rosterModel->contactName(jid));
    connect(dialog, SIGNAL(closeTab(const QString&)), this, SLOT(leaveConversation(const QString&)));
    if (room)
    {
        dialog->setRemotePixmap(QPixmap(":/chat.png"));
        rosterModel->addItem(ChatRosterItem::Room, jid);
    } else {
        dialog->setRemotePixmap(rosterModel->contactAvatar(jid));
    }
    addPanel(dialog);

    // join conversation
    dialog->join();
    return dialog;
}

void Chat::disconnected()
{
    qWarning("Disconnected from chat server");
    isConnected = false;
    addButton->setEnabled(false);
    roomButton->setEnabled(false);
    statusCombo->setCurrentIndex(OfflineIndex);
    pingTimer->stop();
    timeoutTimer->stop();

    if (reconnectOnDisconnect)
    {
        qWarning("Reconnecting to chat server");
        reconnectOnDisconnect = false;
        client->connectToServer(client->getConfiguration());
    }
}

void Chat::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    // we only want results
    if (disco.type() != QXmppIq::Result)
        return;

#if 0
    qDebug("Received discovery result");
    foreach (const QXmppElement &item, disco.queryItems())
        dumpElement(item);
#endif

    if (disco.queryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.from() == client->getConfiguration().domain())
    {
        // root items
        discoQueue.clear();
        foreach (const QXmppElement &item, disco.queryItems())
        {
            if (item.tagName() == "item" && !item.attribute("jid").isEmpty() && item.attribute("node").isEmpty())
            {
                discoQueue.append(item.attribute("jid"));
                // get info for item
                QXmppDiscoveryIq info;
                info.setQueryType(QXmppDiscoveryIq::InfoQuery);
                info.setTo(item.attribute("jid"));
                client->sendPacket(info);
            }
        }
    }
    else if (disco.queryType() == QXmppDiscoveryIq::InfoQuery &&
             discoQueue.contains(disco.from()))
    {
        discoQueue.removeAll(disco.from());
        // check if it's a conference server
        foreach (const QXmppElement &item, disco.queryItems())
        {
            if (item.tagName() != "identity")
                continue;
            if (item.attribute("category") == "conference" &&
                item.attribute("type") == "text")
            {
                chatRoomServer = disco.from();
                roomButton->setEnabled(true);
                qDebug() << "Found chat room server" << chatRoomServer;
            }
            else if (item.attribute("category") == "proxy" &&
                     item.attribute("type") == "bytestreams")
            {
                client->getTransferManager().setProxy(disco.from());
                qDebug() << "Found bytestream proxy" << disco.from();
            }
        }
    }
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

void Chat::iqReceived(const QXmppIq &iq)
{
    timeoutTimer->stop();
    if (iq.type() == QXmppIq::Result && !iq.extensions().isEmpty())
    {
        const QXmppElement query = iq.extensions().first();
        const QXmppElement form = query.firstChildElement("x");
        if (query.tagName() == "query" &&
            query.attribute("xmlns") == ns_muc_owner &&
            form.attribute("type") == "form" &&
            form.attribute("xmlns") == "jabber:x:data")
        {
            ChatForm dialog(form, this);
            if (dialog.exec())
            {
                QXmppElement query;
                query.setTagName("query");
                query.setAttribute("xmlns", ns_muc_owner);
                query.appendChild(dialog.form());

                QXmppIq iqPacket(QXmppIq::Set);
                iqPacket.setExtensions(query);
                iqPacket.setTo(iq.from());
                client->sendPacket(iqPacket);
            }
        }
    }
}

void Chat::joinConversation(const QString &jid, bool isRoom)
{
    ChatConversation *dialog = conversationPanel->findChild<ChatConversation*>(jid);
    if (!dialog)
        dialog = createConversation(jid, isRoom);

    conversationPanel->setCurrentWidget(dialog);
    dialog->setFocus();
}

void Chat::leaveConversation(const QString &jid)
{
    ChatConversation *dialog = conversationPanel->findChild<ChatConversation*>(jid);
    if (!dialog)
        return;

    // leave room
    if (qobject_cast<ChatRoom*>(dialog))
        rosterModel->removeItem(jid);
    dialog->leave();

    // close view
    removePanel(dialog);
    dialog->deleteLater();
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
        return;
    case QXmppMessage::Chat:
        if (!conversationPanel->findChild<ChatDialog*>(bareJid) && !msg.body().isEmpty())
        {
            ChatDialog *dialog = qobject_cast<ChatDialog*>(createConversation(bareJid, false));
            dialog->messageReceived(msg);
        }
        break;
    case QXmppMessage::Error:
        qWarning() << "Received an error message" << msg.body();
        return;
    default:
        return;
    }

    // don't alert the user for empty messages or chat rooms
    ChatDialog *dialog = conversationPanel->findChild<ChatDialog*>(bareJid);
    if (msg.body().isEmpty() || !dialog)
        return;

    // add pending message
    if (conversationPanel->currentWidget() != dialog)
        rosterModel->addPendingMessage(bareJid);

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
    QApplication::alert(dialog);
#endif
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

/** Open the connection to the chat server.
 *
 * @param jid
 * @param password
 */
bool Chat::open(const QString &jid, const QString &password, bool ignoreSslErrors)
{
    QXmppConfiguration config;
    config.setResource("wDesktop");

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

    /* connect to server */
    client->connectToServer(config);
    return true;
}

/** Re-join open conversations after reconnection.
 */
void Chat::rejoinConversations()
{
    for (int i = 0; i < conversationPanel->count(); i++)
    {
        ChatConversation *dialog = qobject_cast<ChatRoom*>(conversationPanel->widget(i));
        if (dialog)
        {
            rosterModel->addItem(ChatRosterItem::Room, dialog->remoteJid());
            dialog->join();
        }
    }
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
        QXmppRosterIq packet(QXmppIq::Set);
        packet.addItem(item);
        client->sendPacket(packet);
    }
}

void Chat::reconnect()
{
    reconnectOnDisconnect = true;
    client->disconnect();
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
        else if (action == ChatRosterView::RemoveAction)
            removeContact(jid);
        else if (action == ChatRosterView::SendAction)
        {
            // find first resource supporting file transfer
            QStringList fullJids = rosterModel->contactFeaturing(jid, ChatRosterModel::FileTransferFeature);
            if (!fullJids.size())
                return;
            QString fullJid = fullJids.first();

            // get file name
            QString filePath = QFileDialog::getOpenFileName(this, tr("Send a file"));
            if (filePath.isEmpty())
                return;

            // check file size
            if (QFileInfo(filePath).size() > fileSizeLimit)
            {
                QMessageBox::warning(this,
                    tr("Send a file"),
                    tr("Sorry, but you cannot send files bigger than %1.")
                        .arg(ChatTransfers::sizeToString(fileSizeLimit)));
                return;
            }

            // send file
            QXmppTransferJob *job = client->getTransferManager().sendFile(fullJid, filePath);
            job->setData(LocalPathRole, filePath);
            chatTransfers->addJob(job);
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
            if (jid == chatTransfers->objectName())
                showTransfers();
            else if (jid == chatConsole->objectName())
                showConsole();
        }
    }
}

/** Send an XMPP Ping as described in XEP-0199:
 *  http://xmpp.org/extensions/xep-0199.html
 */
void Chat::sendPing()
{
    QXmppPingIq ping;
    ping.setFrom(client->getConfiguration().jid());
    ping.setTo(client->getConfiguration().domain());
    client->sendPacket(ping);
    timeoutTimer->start();
}

void Chat::statusChanged(int currentIndex)
{
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

void Chat::hideConsole()
{
    if (chatConsole)
    {
        rosterModel->removeItem(chatConsole->objectName());
        removePanel(chatConsole);
        chatConsole->deleteLater();
        chatConsole = 0;
    }
}

void Chat::showConsole()
{
    if (!chatConsole)
    {
        chatConsole = new ChatConsole;
        chatConsole->setObjectName("console");
        connect(client->logger(), SIGNAL(message(QXmppLogger::MessageType,QString)), chatConsole, SLOT(message(QXmppLogger::MessageType,QString)));
        connect(chatConsole, SIGNAL(closeTab()), this, SLOT(hideConsole()));
    }
    rosterModel->addItem(ChatRosterItem::Other,
        chatConsole->objectName(),
        chatConsole->windowTitle(),
        QIcon(":/options.png"));
    addPanel(chatConsole);
    conversationPanel->setCurrentWidget(chatConsole);
}

void Chat::hideTransfers()
{
    removePanel(chatTransfers);
}

void Chat::showTransfers()
{
    rosterModel->addItem(ChatRosterItem::Other,
        chatTransfers->objectName(),
        chatTransfers->windowTitle(),
        QIcon(":/album.png"));
    addPanel(chatTransfers);
    conversationPanel->setCurrentWidget(chatTransfers);
}

