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
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>

#include "qxmpp/QXmppShareIq.h"

#include "chat_client.h"
#include "chat_shares.h"

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : QWidget(parent), client(xmppClient)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Enter the name of the chat room you want to join.")));
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    layout->addWidget(lineEdit);

    listWidget = new QListWidget;
    listWidget->setIconSize(QSize(32, 32));
    listWidget->setSortingEnabled(true);
    //connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
    layout->addWidget(listWidget);

    setLayout(layout);

    // FIXME : find shared files in a thread
    findLocalFiles();

    /* connect signals */
    connect(client, SIGNAL(shareIqReceived(const QXmppShareIq&)), this, SLOT(shareIqReceived(const QXmppShareIq&)));
}

ChatShares::~ChatShares()
{
//    unregisterFromServer();
}

void ChatShares::shareIqReceived(const QXmppShareIq &share)
{
    if (share.from() != shareServer)
        return;

    if (share.type() == QXmppIq::Get)
    {
        // perform search
        QList<QXmppShareIq::File> files;
        foreach (const QByteArray &key, sharedFiles.keys())
        {
            QXmppShareIq::File file;
            file.setName(sharedFiles[key].name);
            file.setSize(sharedFiles[key].size);
            file.setHash(key);
            files.append(file);
        }

        // send response
        QXmppShareIq response;
        response.setId(share.id());
        response.setTo(share.from());
        response.setType(QXmppIq::Result);
        response.setFiles(files);
        client->sendPacket(response);
    }
    else if (share.type() == QXmppIq::Result)
    {
        lineEdit->setEnabled(true);
        foreach (const QXmppShareIq::File &file, share.files())
        {
            listWidget->insertItem(0, file.name());
        }
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

        File f;
        f.name = info.fileName();
        f.path = info.absoluteFilePath();
        f.size = info.size();
        QFile file(f.path);
        if (file.open(QIODevice::ReadOnly))
        {
            while (file.bytesAvailable())
            {
                buffer = file.read(16384);
                hash.addData(buffer);
            }
            const QByteArray key = hash.result();
            sharedFiles.insert(key, f);
            hash.reset();
        }
    }
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();
    if (search.isEmpty())
        return;

    lineEdit->setEnabled(false);
    listWidget->clear();

    QXmppShareIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
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

