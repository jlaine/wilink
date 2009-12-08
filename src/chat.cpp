/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <QContextMenuEvent>
#include <QDebug>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QStringList>
#include <QSystemTrayIcon>

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"

#include "qnetio/dns.h"
#include "chat.h"

using namespace QNetIO;

ContactsList::ContactsList(QWidget *parent)
    : QListWidget(parent)
{
    contextMenu = new QMenu(this);
    QAction *action = contextMenu->addAction(tr("Remove contact"));
    connect(action, SIGNAL(triggered()), this, SLOT(removeContact()));

    setContextMenuPolicy(Qt::DefaultContextMenu);
    setMinimumSize(QSize(140, 140));
}

void ContactsList::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = itemAt(event->pos());
    if (!item)
        return;

    contextMenu->popup(event->globalPos());
}

void ContactsList::removeContact()
{
    QListWidgetItem *item = currentItem();
    if (!item)
        return;

    QString jid = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, tr("Remove contact"),
        tr("Do you want to remove %1 from your contact list?").arg(jid),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        qDebug() << "remove" << jid;
    }
}

Chat::Chat(QSystemTrayIcon *trayIcon)
    : systemTrayIcon(trayIcon)
{
    client = new QXmppClient(this);
    connect(client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(handleMessage(const QXmppMessage&)));
    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(handlePresence(const QXmppPresence&)));
    connect(client, SIGNAL(connected()), this, SLOT(connected()));

    /* assemble UI */
    QVBoxLayout *layout = new QVBoxLayout;
    contacts = new ContactsList;
    layout->addWidget(contacts);

    statusLabel = new QLabel;
    layout->addWidget(statusLabel);

    setLayout(layout);
}

void Chat::connected()
{
    statusLabel->setText(tr("Connected"));
    connect(&client->getRoster(), SIGNAL(rosterReceived()), this, SLOT(rosterReceived()));
}

void Chat::handleMessage(const QXmppMessage &msg)
{
    const QString body = msg.getBody();
    const QString from = msg.getFrom().split("@")[0];

    if (body.isEmpty())
        return;
    
    if (systemTrayIcon)
        systemTrayIcon->showMessage(from, body);
}

void Chat::handlePresence(const QXmppPresence &presence)
{
    if (presence.getType() != QXmppPresence::Subscribe)
        return;
    qDebug() << "Subscribe received from" << presence.getFrom();
    if (QMessageBox::question(this, tr("Subscribe request"),
        tr("%1 has asked to add you to his or her contact list. Do you accept?").arg(presence.getFrom()),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        qDebug("Subscribe accepted");
    } else {
        qDebug("Subscribe refused");
    }

}

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
    config.setUser(bits[0]);
    config.setDomain(bits[1]);

    /* get the server */
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.getDomain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.getDomain());
    }

    /* connect to server */
    statusLabel->setText(tr("Connecting.."));
    config.setPasswd(password);
    client->connectToServer(config);
    return true;
}

void Chat::rosterReceived()
{
    QMap<QString, QXmppRoster::QXmppRosterEntry> entries = client->getRoster().getRosterEntries();

    QIcon icon(":/contact.png");
    foreach (const QString &key, entries.keys())
    {
        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setData(Qt::UserRole, key);
        newItem->setIcon(icon);
        QString name = entries[key].getName();
        if (name.isEmpty())
            name = key.split("@")[0];
        newItem->setText(name);
        contacts->addItem(newItem);
    }
}

