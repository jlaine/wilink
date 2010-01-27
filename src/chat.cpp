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
#include "chat_roster.h"

#ifdef QT_MAC_USE_COCOA
void application_alert_mac();
#endif

using namespace QNetIO;

Chat::Chat(QSystemTrayIcon *trayIcon)
    : reconnectOnDisconnect(false), systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);
    rosterModel =  new RosterModel(&client->getRoster(), &client->getVCardManager());

    /* build splitter */
    splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    rosterView = new RosterView(rosterModel);
    connect(rosterView, SIGNAL(clicked(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(rosterView, SIGNAL(doubleClicked(const QString&)), this, SLOT(chatContact(const QString&)));
    connect(rosterView, SIGNAL(removeContact(const QString&)), this, SLOT(removeContact(const QString&)));
    connect(rosterView->model(), SIGNAL(modelReset()), this, SLOT(resizeContacts()));
    splitter->addWidget(rosterView);
    splitter->setStretchFactor(0, 0);

    conversationPanel = new QStackedWidget;
    conversationPanel->hide();
    splitter->addWidget(conversationPanel);
    splitter->setStretchFactor(1, 1);

    /* build status bar */
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setMargin(10);
    hbox->setSpacing(10);

    QPushButton *addButton = new QPushButton;
    addButton->setIcon(QIcon(":/add.png"));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addContact()));
    hbox->addWidget(addButton);
    hbox->addStretch();

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

    /* set up timers */
    pingTimer = new QTimer(this);
    pingTimer->setInterval(60000);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(10000);
    connect(timeoutTimer, SIGNAL(timeout()), this, SLOT(reconnect()));
}

/** Prompt the user for a new contact then add it to the roster.
 */
void Chat::addContact()
{
    bool ok = false;
    QString defaultJid = "@" + client->getConfiguration().getDomain();
    QString jid = QInputDialog::getText(this, tr("Add a contact"),
        tr("Enter the address of the contact you want to add."),
        QLineEdit::Normal, defaultJid, &ok);
    if (!ok || jid.isEmpty())
        return;

    QXmppPresence packet;
    packet.setTo(jid);
    packet.setType(QXmppPresence::Subscribe);
    client->sendPacket(packet);
}

void Chat::archiveChatReceived(const QXmppArchiveChat &chat)
{
    QString bareJid = chat.with.split("/")[0];
    if (chatDialogs.contains(bareJid))
        chatDialogs[bareJid]->archiveChatReceived(chat);
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

void Chat::connected()
{
    qWarning("CONNECTED");
    pingTimer->start();
    statusLabel->setText(tr("Connected"));
}

/** Get or create a conversation dialog for the specified recipient.
 *
 * @param jid
 */
ChatDialog *Chat::conversation(const QString &jid)
{
    if (!chatDialogs.contains(jid))
    {
        chatDialogs[jid] = new ChatDialog(jid, rosterModel->contactName(jid));
        chatDialogs[jid]->setAvatar(rosterModel->contactAvatar(jid));
        connect(chatDialogs[jid], SIGNAL(sendMessage(const QString&, const QString&)),
            this, SLOT(sendMessage(const QString&, const QString&)));
        conversationPanel->addWidget(chatDialogs[jid]);
        conversationPanel->show();

        // get archives
        client->getArchiveManager().getCollections(jid);
    }
    return chatDialogs[jid];
}

void Chat::disconnected()
{
    qWarning("DISCONNECTED");
    pingTimer->stop();
    timeoutTimer->stop();
    rosterModel->disconnected();

    if (reconnectOnDisconnect)
    {
        qWarning("RECONNECT");
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
        qWarning("RESOURCE CONFLICT");
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
    const QString body = msg.getBody();

    if (body.isEmpty())
        return;

    ChatDialog *dialog = conversation(bareJid);
    dialog->messageReceived(msg);

    if (!isVisible())
    {
#ifdef Q_OS_MAC
        show();
#else
        showMinimized();
#endif
    }
    if (conversationPanel->currentWidget() != dialog)
        rosterModel->addPendingMessage(bareJid);

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
            if (QMessageBox::question(this,
                tr("Invitation from %1").arg(presence.getFrom()),
                tr("%1 has asked to add you to his or her contact list. Do you accept?").arg(presence.getFrom()),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
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
    QStringList bits = jid.split("@");
    if (bits.size() != 2)
    {
        qWarning("Invalid JID");
        return false;
    }
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
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
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

/** Send a chat message to the specified recipient.
 *
 * @param jid
 * @param message
 */
void Chat::sendMessage(const QString &jid, const QString message)
{
    client->sendPacket(QXmppMessage("", jid, message));
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

