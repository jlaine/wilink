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

#include <QCryptographicHash>
#include <QDir>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppDiscoveryIq.h"

#include "chat_shares.h"

const char* ns_shares = "http://wifirst.net/protocol/shares";

ChatShares::ChatShares(QXmppClient *xmppClient, QWidget *parent)
    : QWidget(parent), client(xmppClient)
{
    connect(client, SIGNAL(discoveryIqReceived(const QXmppDiscoveryIq&)), this, SLOT(discoveryIqReceived(const QXmppDiscoveryIq&)));

    // FIXME : find shared files in a thread
    findLocalFiles();
}

ChatShares::~ChatShares()
{
//    unregisterFromServer();
}

void ChatShares::discoveryIqReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.from() == shareServer &&
        disco.type() == QXmppIq::Get &&
        disco.queryNode() == ns_shares &&
        disco.queryType() == QXmppDiscoveryIq::ItemsQuery)
    {
        QXmppDiscoveryIq iq;
        iq.setId(disco.id());
        iq.setTo(disco.from());
        iq.setType(QXmppIq::Result);
        iq.setQueryType(disco.queryType());

        QList<QXmppElement> items;

        const QString ownJid = client->getConfiguration().jid();
        foreach (const QString &key, sharedFiles.keys())
        {
            QXmppElement item;
            item.setTagName("item");
            item.setAttribute("jid", ownJid);
            item.setAttribute("node", key);
            item.setAttribute("name", QFileInfo(sharedFiles[key]).fileName());
            items.append(item);
        }

        iq.setQueryItems(items);
        client->sendPacket(iq);
    }
}

void ChatShares::findLocalFiles()
{
    QDir shares(QDir::home().filePath("Public"));
    QByteArray buffer;
    QCryptographicHash hash(QCryptographicHash::Md5);
    foreach (const QFileInfo &info, shares.entryInfoList())
    {
        if (info.isDir())
            continue;

        const QString path = info.absoluteFilePath();
        QFile file(path);
        if (file.open(QIODevice::ReadOnly))
        {
            while (file.bytesAvailable())
            {
                buffer = file.read(16384);
                hash.addData(buffer);
            }
            const QString key = hash.result().toHex();
            sharedFiles.insert(key, path);
            hash.reset();
        }
    }
}

void ChatShares::findRemoteFiles(const QString &query)
{
    QXmppDiscoveryIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setQueryType(QXmppDiscoveryIq::ItemsQuery);
    iq.setQueryNode(ns_shares);

    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);
    QXmppElement search;
    search.setTagName("search");
    search.setValue(query);
    x.appendChild(search);

    iq.setExtensions(x);
}

void ChatShares::registerWithServer()
{
    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(x);
    client->sendPacket(presence);
}

void ChatShares::unregisterFromServer()
{
    // unregister from server
    QList<QXmppElement> extensions;
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);
    extensions.append(x);

    QXmppPresence presence(QXmppPresence::Unavailable);
    presence.setTo(shareServer);
    presence.setExtensions(extensions);
    client->sendPacket(presence);
}

void ChatShares::setShareServer(const QString &server)
{
    shareServer = server;
    registerWithServer();
}

