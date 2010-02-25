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
#include "qxmpp/QXmppIbbTransferManager.h"
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
    : isBusy(false), isConnected(false), reconnectOnDisconnect(false), systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);
    rosterModel =  new ChatRosterModel(client);

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
    chatTransfers = new ChatTransfers;

    /* set up client */
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));
    connect(client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(&client->getTransferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
            this, SLOT(fileReceived(QXmppTransferJob*)));

    /* set up timers */
    pingTimer = new QTimer(this);
    pingTimer->setInterval(60000);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(10000);
    connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_T), this);
    connect(shortcut, SIGNAL(activated()), chatTransfers, SLOT(show()));
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
    QString jid = "@" + client->getConfiguration().getDomain();
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
    disco.setTo(client->getConfiguration().getDomain());
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
        dialog = new ChatDialog(client, jid);
    dialog->setLocalName(rosterModel->ownName());
    dialog->setRemoteName(rosterModel->contactName(jid));
    connect(dialog, SIGNAL(leave(const QString&)), this, SLOT(leaveConversation(const QString&)));
    conversationPanel->addWidget(dialog);
    conversationPanel->show();
    if (conversationPanel->count() == 1)
        resizeContacts();
    chatDialogs[jid] = dialog;

    if (room)
    {
        dialog->setRemotePixmap(QPixmap(":/chat.png"));
        rosterModel->addRoom(jid);
    } else {
        dialog->setRemotePixmap(rosterModel->contactAvatar(jid));
    }

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
    if (disco.getType() != QXmppIq::Result)
        return;

#if 0
    qDebug("Received discovery result");
    foreach (const QXmppElement &item, disco.getQueryItems())
        dumpElement(item);
#endif

    if (disco.getQueryType() == QXmppDiscoveryIq::ItemsQuery &&
        disco.getFrom() == client->getConfiguration().getDomain())
    {
        // root items
        discoQueue.clear();
        foreach (const QXmppElement &item, disco.getQueryItems())
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
    else if (disco.getQueryType() == QXmppDiscoveryIq::InfoQuery &&
             discoQueue.contains(disco.getFrom()))
    {
        discoQueue.removeAll(disco.getFrom());
        // check if it's a conference server
        foreach (const QXmppElement &item, disco.getQueryItems())
        {
            if (item.tagName() == "identity" && item.attribute("category") == "conference" && item.attribute("type") == "text")
            {
                chatRoomServer = disco.getFrom();
                roomButton->setEnabled(true);
                qDebug() << "Found chat room server" << chatRoomServer;
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
    if (iq.getType() == QXmppIq::Result && !iq.getExtensions().isEmpty())
    {
        const QXmppElement query = iq.getExtensions().first();
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
                iqPacket.setTo(iq.getFrom());
                client->sendPacket(iqPacket);
            }
        }
    }
}

void Chat::joinConversation(const QString &jid, bool isRoom)
{
    if (!chatDialogs.contains(jid))
        createConversation(jid, isRoom);

    ChatConversation *dialog = chatDialogs.value(jid);
    rosterModel->clearPendingMessages(jid);
    rosterView->selectContact(jid);
    conversationPanel->setCurrentWidget(dialog);
    dialog->setFocus();
}

void Chat::leaveConversation(const QString &jid)
{
    if (!chatDialogs.contains(jid))
        return;
    ChatConversation *dialog = chatDialogs.take(jid);

    // leave room
    if (dialog->isRoom())
        rosterModel->removeRoom(jid);
    dialog->leave();

    // close view
    if (conversationPanel->count() == 1)
    {
        conversationPanel->hide();
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    dialog->deleteLater();
}

void Chat::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.getFrom());

    switch (msg.getType())
    {
    case QXmppMessage::Normal:
        foreach (const QXmppElement &extension, msg.getExtensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_conference)
            {
                const QString contactName = rosterModel->contactName(bareJid);
                const QString roomJid = extension.attribute("jid");
                if (!roomJid.isEmpty() &&
                    !chatDialogs.contains(roomJid) &&
                    QMessageBox::question(this,
                        tr("Invitation from %1").arg(contactName),
                        tr("%1 has asked to add you to join the '%2' chat room. Do you accept?").arg(contactName, roomJid),
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                {
                    joinConversation(roomJid, true);
                }
                break;
            }
        }
        return;
    case QXmppMessage::Chat:
        if (!chatDialogs.contains(bareJid) && !msg.getBody().isEmpty())
        {
            ChatDialog *dialog = qobject_cast<ChatDialog*>(createConversation(bareJid, false));
            dialog->messageReceived(msg);
        }
        break;
    case QXmppMessage::Error:
        qWarning() << "Received an error message" << msg.getBody();
        return;
    default:
        return;
    }

    // add message
    ChatConversation *dialog = chatDialogs.value(bareJid);
    if (!dialog)
        return;
    if (conversationPanel->currentWidget() != dialog)
        rosterModel->addPendingMessage(bareJid);
    else
        rosterView->selectContact(bareJid);

    // don't alert the user for empty messages or chat rooms
    if (msg.getBody().isEmpty() || dialog->isRoom())
        return;

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
    packet.setTo(presence.getFrom());
    const QString bareJid = jidToBareJid(presence.getFrom());
    
    switch (presence.getType())
    {
    case QXmppPresence::Error:
        if (!chatDialogs.contains(bareJid))
            return;
        foreach (const QXmppElement &extension, presence.getExtensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc)
            {
                leaveConversation(bareJid);

                QXmppStanza::Error error = presence.getError();
                QMessageBox::warning(this,
                    tr("Chat room error"),
                    tr("Sorry, but you cannot join chat room %1.\n\n%2")
                        .arg(bareJid)
                        .arg(error.getText()));
                break;
            }
        }
        break;
    case QXmppPresence::Unavailable:
        if (!chatDialogs.contains(bareJid) ||
            chatDialogs[bareJid]->localName() != jidToResource(presence.getFrom()))
            return;
        foreach (const QXmppElement &extension, presence.getExtensions())
        {
            if (extension.tagName() == "x" && extension.attribute("xmlns") == ns_muc_user)
            {
                leaveConversation(bareJid);

                QXmppElement reason = extension.firstChildElement("item").firstChildElement("reason");
                QMessageBox::warning(this,
                    tr("Chat room error"),
                    tr("Sorry, but you were kicked from chat room %1.\n\n%2")
                        .arg(bareJid)
                        .arg(reason.value()));
                break;
            }
        }
        break;
    case QXmppPresence::Subscribe:
        {
            QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(presence.getFrom());
            QXmppRoster::QXmppRosterEntry::SubscriptionType type = entry.getSubscriptionType();

            /* if the contact is in our roster accept subscribe, otherwise ask user */
            bool accepted = false;
            if (type == QXmppRoster::QXmppRosterEntry::To || type == QXmppRoster::QXmppRosterEntry::Both)
                accepted = true;
            else if (QMessageBox::question(this,
                    tr("Invitation from %1").arg(presence.getFrom()),
                    tr("%1 has asked to add you to his or her contact list. Do you accept?").arg(presence.getFrom()),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                accepted = true;

            if (accepted)
            {
                qDebug("Subscribe accepted");
                packet.setType(QXmppPresence::Subscribed);
                client->sendPacket(packet);

                packet.setType(QXmppPresence::Subscribe);
                client->sendPacket(packet);
            } else {
                qDebug("Subscribe refused");
                QXmppPresence packet;
                packet.setType(QXmppPresence::Unsubscribed);
                client->sendPacket(packet);
            }
        }
        break;
    default:
#if 0
        if (!presence.getExtension().isNull())
        {
            qDebug("Received a presence with an extension");
            dumpElement(presence.getExtension());
        }
#endif
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

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::NONE);

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
#ifdef USE_DNS_SRV
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.getDomain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.getDomain());
    }
#else
    config.setHost(config.getDomain());
#endif

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
    foreach (const QString &bareJid, chatDialogs.keys())
    {
        ChatConversation *dialog = chatDialogs.value(bareJid);
        if (dialog->isRoom())
        {
            rosterModel->addRoom(bareJid);
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
        else if (action == ChatRosterView::LeaveAction)
            leaveConversation(jid);
        else if (action == ChatRosterView::RemoveAction)
            removeContact(jid);
        else if (action == ChatRosterView::SendAction)
        {
            // FIXME : we need to discover client support for file transfer!
            QStringList resources = client->getRoster().getResources(jid);
            if (!resources.size())
                return;
            QString fullJid = jid + "/" + resources.first();

            QFileDialog dlg(this);
            if (dlg.exec())
            {
                QStringList files = dlg.selectedFiles();
                if (!files.size())
                    return;
                const QString filePath = files.first();

                QXmppTransferJob *job = client->getTransferManager().sendFile(fullJid, filePath);
                job->setLocalFilePath(filePath);
                chatTransfers->addJob(job);
                chatTransfers->show();
            }
        }
    } else if (type == ChatRosterItem::Room) {
        if (action == ChatRosterView::JoinAction)
            joinConversation(jid, true);
        else if (action == ChatRosterView::LeaveAction)
            leaveConversation(jid);
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
    }
}

/** Send an XMPP Ping as described in XEP-0199:
 *  http://xmpp.org/extensions/xep-0199.html
 */
void Chat::sendPing()
{
    QXmppPingIq ping;
    ping.setFrom(client->getConfiguration().getJid());
    ping.setTo(client->getConfiguration().getDomain());
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

void Chat::fileReceived(QXmppTransferJob *job)
{
    const QString bareJid = jidToBareJid(job->jid());
    const QString contactName = rosterModel->contactName(bareJid);

    if (QMessageBox::question(this,
        tr("File from %1").arg(contactName),
        tr("%1 wants to send you a file called '%2'. Do you accept?").arg(contactName, job->fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        QDir downloadsDir(SystemInfo::downloadsLocation());
        const QString filePath = downloadsDir.absoluteFilePath(job->fileName());

        QFile *file = new QFile(filePath, job);
        if (file->open(QIODevice::WriteOnly))
        {
            job->setLocalFilePath(filePath);
            chatTransfers->addJob(job);
            chatTransfers->show();
            job->accept(file);
        }
    }
}
