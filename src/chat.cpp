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
#include <QDebug>
#include <QDesktopWidget>
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

#include "qxmpp/QXmppArchiveIq.h"
#include "qxmpp/QXmppArchiveManager.h"
#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppPingIq.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppVCardManager.h"

#include "qnetio/dns.h"
#include "chat.h"
#include "chat_dialog.h"
#include "chat_room.h"
#include "chat_rooms.h"
#include "chat_roster.h"

#ifdef QT_MAC_USE_COCOA
void application_alert_mac();
#endif

using namespace QNetIO;

static QRegExp jidValidator("[^@]+@[^@]+");

Chat::Chat(QSystemTrayIcon *trayIcon)
    : reconnectOnDisconnect(false), systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);
    rosterModel =  new RosterModel(&client->getRoster(), &client->getVCardManager());
    roomsModel =  new ChatRoomsModel(client);

    /* build splitter */
    splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    /* left panel */
    QWidget *leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->setMargin(0);
    rosterView = new RosterView(rosterModel);
    connect(rosterView, SIGNAL(clicked(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(rosterView, SIGNAL(doubleClicked(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(rosterView, SIGNAL(removeContact(const QString&)), this, SLOT(removeContact(const QString&)));
    connect(rosterView->model(), SIGNAL(modelReset()), this, SLOT(resizeContacts()));
    leftLayout->insertWidget(0, rosterView, 1);

    roomsView = new ChatRoomsView(roomsModel);
    connect(roomsView, SIGNAL(clicked(const QString&)), this, SLOT(chatRoom(const QString&)));
    connect(roomsView, SIGNAL(doubleClicked(const QString&)), this, SLOT(chatRoom(const QString&)));
    roomsView->hide();
    leftLayout->insertWidget(1, roomsView, 0);
    leftPanel->setLayout(leftLayout);

    splitter->addWidget(leftPanel);
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
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    hbox->addWidget(addButton);

    roomButton = new QPushButton;
    roomButton->setEnabled(false);
    roomButton->setIcon(QIcon(":/chat.png"));
    connect(roomButton, SIGNAL(clicked()), this, SLOT(addRoom()));
    hbox->addWidget(roomButton);

    hbox->addStretch();

    statusIconLabel = new QLabel;
    statusIconLabel->setPixmap(QPixmap(":/contact-offline.png"));
    hbox->addWidget(statusIconLabel);
    statusLabel = new QLabel;
    hbox->addWidget(statusLabel);

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(splitter, 1);
    layout->addLayout(hbox);

    setLayout(layout);
    setWindowIcon(QIcon(":/chat.png"));
    setWindowTitle(tr("Chat"));

    /* set up client */
    connect(client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(client, SIGNAL(iqReceived(const QXmppIq&)), this, SLOT(iqReceived(const QXmppIq&)));
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));

    connect(&client->getArchiveManager(), SIGNAL(archiveChatReceived(const QXmppArchiveChat &)), this, SLOT(archiveChatReceived(const QXmppArchiveChat &)));
    connect(&client->getArchiveManager(), SIGNAL(archiveListReceived(const QList<QXmppArchiveChat> &)), this, SLOT(archiveListReceived(const QList<QXmppArchiveChat> &)));
    connect(&client->getVCardManager(), SIGNAL(vCardReceived(const QXmppVCard&)), this, SLOT(vCardReceived(const QXmppVCard&)));

    /* set up timers */
    pingTimer = new QTimer(this);
    pingTimer->setInterval(60000);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(10000);
    connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(reconnect()));

    /* set up keyboard shortcuts */
#ifdef Q_OS_MAC
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_W), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
#endif
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
    bool ok = true;
    QString jid = "@conference." + client->getConfiguration().getDomain();
    while (!jidValidator.exactMatch(jid))
    {
        jid = QInputDialog::getText(this, tr("Join a group chat"),
            tr("Enter the address of the room you want to join."),
            QLineEdit::Normal, jid, &ok).toLower();
        if (!ok)
            return;
        jid = jid.trimmed().toLower();
    }

    chatRoom(jid);
}

void Chat::archiveChatReceived(const QXmppArchiveChat &chat)
{
    QString bareJid = chat.with.split("/")[0];
    if (chatDialogs.contains(bareJid))
        chatDialogs[bareJid]->archiveChatReceived(chat);
}

void Chat::archiveListReceived(const QList<QXmppArchiveChat> &chats)
{
    for (int i = chats.size() - 1; i >= 0; i--)
        client->getArchiveManager().retrieveCollection(chats[i].with, chats[i].start);
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

void Chat::chatContact(const QString &jid)
{
    ChatDialog *dialog = conversation(jid);
    rosterModel->clearPendingMessages(jid);

    conversationPanel->setCurrentWidget(dialog);
    dialog->setFocus();
}

void Chat::chatRoom(const QString &jid)
{
    ChatRoom *dialog = room(jid);
    conversationPanel->setCurrentWidget(dialog);
    dialog->setFocus();
}

void Chat::connected()
{
    qWarning("Connected to chat server");
    addButton->setEnabled(true);
    roomButton->setEnabled(true);
    pingTimer->start();
    statusIconLabel->setPixmap(QPixmap(":/contact-available.png"));
    statusLabel->setText(tr("Connected"));

    /* request own vCard */
    ownName = client->getConfiguration().getUser();
    client->getVCardManager().requestVCard(
        client->getConfiguration().getJidBare());
}

/** Get or create a conversation dialog for the specified recipient.
 *
 * @param jid
 */
ChatDialog *Chat::conversation(const QString &jid)
{
    if (!chatDialogs.contains(jid))
    {
        chatDialogs[jid] = new ChatDialog(jid);
        chatDialogs[jid]->setLocalName(ownName);
        chatDialogs[jid]->setRemoteAvatar(rosterModel->contactAvatar(jid));
        chatDialogs[jid]->setRemoteName(rosterModel->contactName(jid));
        connect(chatDialogs[jid], SIGNAL(sendMessage(const QXmppMessage&)),
            this, SLOT(sendMessage(const QXmppMessage&)));
        conversationPanel->addWidget(chatDialogs[jid]);
        conversationPanel->show();

        // list archives for the past week
        client->getArchiveManager().listCollections(jid,
            QDateTime::currentDateTime().addDays(-7));
    }
    return chatDialogs[jid];
}

void Chat::disconnected()
{
    qWarning("Disconnected from chat server");
    addButton->setEnabled(false);
    roomButton->setEnabled(false);
    pingTimer->stop();
    timeoutTimer->stop();
    rosterModel->disconnected();

    statusIconLabel->setPixmap(QPixmap(":/contact-offline.png"));
    if (reconnectOnDisconnect)
    {
        qWarning("Reconnecting to chat server");
        reconnectOnDisconnect = false;
        statusLabel->setText(tr("Connecting.."));
        client->connectToServer(client->getConfiguration());
    } else {
        statusLabel->setText(tr("Disconnected"));
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

void Chat::iqReceived(const QXmppIq&)
{
    timeoutTimer->stop();
}

void Chat::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = msg.getFrom().split("/")[0];

    if (msg.getBody().isEmpty())
        return;

    switch (msg.getType())
    {
    case QXmppMessage::GroupChat:
        if (chatRooms.contains(bareJid))
            chatRooms[bareJid]->messageReceived(msg);
        return;
    case QXmppMessage::Error:
        qWarning() << "Received an error message" << msg.getBody();
        return;
    case QXmppMessage::Headline:
        // FIXME : what is this type ?
        return;
    default:
        break;
    }

    ChatDialog *dialog = conversation(bareJid);
    dialog->messageReceived(msg);
    if (conversationPanel->currentWidget() != dialog)
        rosterModel->addPendingMessage(bareJid);

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
    switch (presence.getType())
    {
    case QXmppPresence::Subscribe:
        {
            QXmppRoster::QXmppRosterEntry entry = client->getRoster().getRosterEntry(presence.getFrom());
            QXmppRosterIq::Item::SubscriptionType type = static_cast<QXmppRosterIq::Item::SubscriptionType>(entry.getSubscriptionType());

            /* if the contact is in our roster accept subscribe, otherwise ask user */
            bool accepted = false;
            if (type == QXmppRosterIq::Item::To || type == QXmppRosterIq::Item::Both)
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
        break;
    }
}

/** Open the connection to the chat server.
 *
 * @param jid
 * @param password
 */
bool Chat::open(const QString &jid, const QString &password)
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
    config.setIgnoreSslErrors(false);

    /* connect to server */
    statusLabel->setText(tr("Connecting.."));
    client->connectToServer(config);
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

    resize(hint.expandedTo(size()));
}

ChatRoom *Chat::room(const QString &jid)
{
    if (!chatRooms.contains(jid))
    {
        roomsModel->addRoom(jid);
        roomsView->show();

        chatRooms[jid] = new ChatRoom(jid);
        chatRooms[jid]->setLocalName(ownName);
        chatRooms[jid]->setRoomName(roomsModel->roomName(jid));
        connect(chatRooms[jid], SIGNAL(sendMessage(const QXmppMessage&)),
            this, SLOT(sendMessage(const QXmppMessage&)));
        conversationPanel->addWidget(chatRooms[jid]);
        conversationPanel->show();

        QXmppPresence packet;
        packet.setTo(jid + "/" + ownName);
        packet.setType(QXmppPresence::Available);
        client->sendPacket(packet);
    }
    return chatRooms[jid];
}

/** Send a chat message to the specified recipient.
 *
 * @param msg
 */
void Chat::sendMessage(const QXmppMessage &msg)
{
    client->sendPacket(msg);
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

void Chat::vCardReceived(const QXmppVCard& vcard)
{
    const QString bareJid = vcard.getFrom();
    if (bareJid == client->getConfiguration().getJidBare())
    {
        if (!vcard.getNickName().isEmpty())
            ownName = vcard.getNickName();
    }
}

