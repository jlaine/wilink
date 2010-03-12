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
#include <QSqlError>
#include <QSqlQuery>
#include <QTime>
#include <QTimer>

#include "qxmpp/QXmppShareIq.h"

#include "chat.h"
#include "chat_shares.h"

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : QWidget(parent), client(xmppClient), indexer(0)
{
    sharesDir = QDir(QDir::home().filePath("Public"));

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

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareIqReceived(const QXmppShareIq&)), this, SLOT(shareIqReceived(const QXmppShareIq&)));
}

void ChatShares::shareIqReceived(const QXmppShareIq &shareIq)
{
    if (shareIq.from() != shareServer)
        return;

    if (shareIq.type() == QXmppIq::Get)
    {
        // refuse to perform search without criteria
        if (shareIq.search().isEmpty())
        {
            QXmppShareIq response;
            response.setId(shareIq.id());
            response.setTo(shareIq.from());
            response.setType(QXmppIq::Error);
            response.setTag(shareIq.tag());
            client->sendPacket(response);
            return;
        }

        // perform search
        QSqlQuery query("SELECT path, size, hash FROM files WHERE path LIKE :search", sharesDb);
        query.bindValue(":search", "%" + shareIq.search() + "%");
        query.exec();

        // prepare update queries
        QSqlQuery deleteQuery("DELETE FROM files WHERE path = :path", sharesDb);
        QSqlQuery updateQuery("UPDATE files SET hash = :hash WHERE path = :path", sharesDb);

        QCryptographicHash hasher(QCryptographicHash::Md5);

        QList<QXmppShareIq::File> files;
        while (query.next())
        {
            const QString path = query.value(0).toString();
            const qint64 size = query.value(1).toInt();
            QByteArray hash = QByteArray::fromHex(query.value(2).toByteArray());

            if (hash.isEmpty())
            {
                QFile file(sharesDir.filePath(path));
                // if we cannot open the file, remove it from database
                if (!file.open(QIODevice::ReadOnly))
                {
                    qWarning() << "Removing file" << path;
                    deleteQuery.bindValue(":path", path);
                    deleteQuery.exec();
                }

                while (file.bytesAvailable())
                    hasher.addData(file.read(16384));
                hash = hasher.result();
                hasher.reset();

                // add hash to database
                updateQuery.bindValue(":hash", hash.toHex());
                updateQuery.bindValue(":path", path);
                updateQuery.exec();
            }

            QXmppShareIq::File file;
            file.setName(QFileInfo(path).fileName());
            file.setSize(size);
            file.setHash(hash);
            files.append(file);
        }

        // send response
        QXmppShareIq response;
        response.setId(shareIq.id());
        response.setTo(shareIq.from());
        response.setType(QXmppIq::Result);
        response.setTag(shareIq.tag());
        response.setFiles(files);
        client->sendPacket(response);
    }
    else if (shareIq.type() == QXmppIq::Result)
    {
        lineEdit->setEnabled(true);
        foreach (const QXmppShareIq::File &file, shareIq.files())
        {
            listWidget->insertItem(0, file.name());
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

void ChatShares::setShareServer(const QString &server)
{
    shareServer = server;

    if (!indexer)
    {
        // prepare database
        sharesDb = QSqlDatabase::addDatabase("QSQLITE");
        sharesDb.setDatabaseName("/tmp/shares.db");
        Q_ASSERT(sharesDb.open());
        sharesDb.exec("CREATE TABLE files (path text, size int, hash varchar(32))");
        sharesDb.exec("CREATE UNIQUE INDEX files_path ON files (path)");

        // run indexer
        indexer = new ChatSharesIndexer(sharesDb, sharesDir, this);
        indexer->start();
    }

    // register with server
    registerWithServer();
    registerTimer->start();
}

ChatSharesIndexer::ChatSharesIndexer(const QSqlDatabase &database, const QDir &dir, QObject *parent)
    : QThread(parent), sharesDb(database), sharesDir(dir)
{
};

void ChatSharesIndexer::run()
{
    scanCount = 0;
    QTime t;
    t.start();
    scanDir(sharesDir);
    qDebug() << "Found" << scanCount << "files in" << double(t.elapsed()) / 1000.0 << "s";
}

void ChatSharesIndexer::scanDir(const QDir &dir)
{
    QSqlQuery query("INSERT INTO files (path, size) "
                    "VALUES(:path, :size)", sharesDb);

    foreach (const QFileInfo &info, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Readable))
    {
        if (info.isDir())
        {
            scanDir(QDir(info.filePath()));
        } else {
            query.bindValue(":path", sharesDir.relativeFilePath(info.filePath()));
            query.bindValue(":size", info.size());
            query.exec();
            scanCount++;
        }
    }
}

