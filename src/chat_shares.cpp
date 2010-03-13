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
#include <QFileIconProvider>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QSqlError>
#include <QSqlQuery>
#include <QTableWidget>
#include <QTime>
#include <QTimer>

#include "qxmpp/QXmppShareIq.h"

#include "chat.h"
#include "chat_shares.h"
#include "chat_transfers.h"

enum Columns
{
    NameColumn,
    SizeColumn,
};

class IndexThread : public QThread
{
public:
    IndexThread(const QSqlDatabase &database, const QDir &dir, QObject *parent = 0);
    void run();

private:
    void scanDir(const QDir &dir);
    qint64 scanCount;
    QSqlDatabase sharesDb;
    QDir sharesDir;
};

class SearchThread : public QThread
{
public:
    SearchThread(const QSqlDatabase &database, const QDir &dir, QObject *parent = 0);
    void run();

private:
    QSqlDatabase sharesDb;
    QDir sharesDir;
};


ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : QWidget(parent), client(xmppClient), db(0)
{
    sharesDir = QDir(QDir::home().filePath("Public"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(tr("Enter the name of the chat room you want to join.")));
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    layout->addWidget(lineEdit);

    tableWidget = new QTableWidget;
    tableWidget->setColumnCount(2);
    tableWidget->setHorizontalHeaderItem(NameColumn, new QTableWidgetItem(tr("Name")));
    tableWidget->setHorizontalHeaderItem(SizeColumn, new QTableWidgetItem(tr("Size")));
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->horizontalHeader()->setResizeMode(NameColumn, QHeaderView::Stretch);
    tableWidget->setSortingEnabled(true);
    //connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(itemClicked(QListWidgetItem*)));
    layout->addWidget(tableWidget);

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
        db->search(shareIq);
    }
    else if (shareIq.type() == QXmppIq::Result)
    {
        QFileIconProvider iconProvider;
        lineEdit->setEnabled(true);
        foreach (const QXmppShareIq::Collection &collection, shareIq.collections())
        {
            foreach (const QXmppShareIq::File &file, collection)
            {
                QTableWidgetItem *nameItem = new QTableWidgetItem(file.name());
                nameItem->setIcon(iconProvider.icon(QFileIconProvider::File));
                QTableWidgetItem *sizeItem = new QTableWidgetItem(ChatTransfers::sizeToString(file.size()));
                tableWidget->insertRow(0);
                tableWidget->setItem(0, NameColumn, nameItem);
                tableWidget->setItem(0, SizeColumn, sizeItem);
            }
        }
    }
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();
    if (search.isEmpty())
        return;

    lineEdit->setEnabled(false);
    tableWidget->clearContents();

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

    if (!db)
    {
        db = new ChatSharesDatabase(sharesDir.path(), this);
        connect(db, SIGNAL(searchFinished(const QXmppPacket&)), client, SLOT(sendPacket(const QXmppPacket&)));
    }

    // register with server
    registerWithServer();
    registerTimer->start();
}

ChatSharesDatabase::ChatSharesDatabase(const QString &path, QObject *parent)
    : QObject(parent), sharesDir(path)
{
    // prepare database
    sharesDb = QSqlDatabase::addDatabase("QSQLITE");
    sharesDb.setDatabaseName("/tmp/shares.db");
    Q_ASSERT(sharesDb.open());
    sharesDb.exec("CREATE TABLE files (path text, size int, hash varchar(32))");
    sharesDb.exec("CREATE UNIQUE INDEX files_path ON files (path)");

    IndexThread *indexer = new IndexThread(sharesDb, sharesDir, this);
    indexer->start();
}

void ChatSharesDatabase::search(const QXmppShareIq &requestIq)
{
    QXmppShareIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setTo(requestIq.from());
    responseIq.setTag(requestIq.tag());

    // refuse to perform search without criteria
    if (requestIq.search().isEmpty())
    {
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    QCryptographicHash hasher(QCryptographicHash::Md5);
    QTime t;
    t.start();

    // perform search
    QSqlQuery query("SELECT path, size, hash FROM files WHERE path LIKE :search", sharesDb);
    query.bindValue(":search", "%" + requestIq.search() + "%");
    query.exec();

    // prepare update queries
    QSqlQuery deleteQuery("DELETE FROM files WHERE path = :path", sharesDb);
    QSqlQuery updateQuery("UPDATE files SET hash = :hash, size = :size WHERE path = :path", sharesDb);

    QXmppShareIq::Collection collection;
    while (query.next())
    {
        const QString path = query.value(0).toString();
        const qint64 size = query.value(1).toInt();
        QByteArray hash = QByteArray::fromHex(query.value(2).toByteArray());
        QFileInfo info(sharesDir.filePath(path));

        // check file is still readable
        if (!info.isReadable())
        {
            deleteQuery.bindValue(":path", path);
            deleteQuery.exec();
            continue;
        }

        // check whether we need to calculate checksum
        if (hash.isEmpty() || info.size() != size)
        {
            QFile file(info.filePath());

            // if we cannot open the file, remove it from database
            if (!file.open(QIODevice::ReadOnly))
            {
                qWarning() << "Failed to open file" << path;
                deleteQuery.bindValue(":path", path);
                deleteQuery.exec();
            }

            while (file.bytesAvailable())
                hasher.addData(file.read(16384));
            hash = hasher.result();
            hasher.reset();

            // update database entry
            updateQuery.bindValue(":hash", hash.toHex());
            updateQuery.bindValue(":size", info.size());
            updateQuery.bindValue(":path", path);
            updateQuery.exec();
        }

        QXmppShareIq::File file;
        file.setName(info.fileName());
        file.setSize(size);
        file.setHash(hash);
        collection.append(file);
    }
    QList<QXmppShareIq::Collection> collections;
    collections.append(collection);

    // send response
    qDebug() << "Found" << collection.size() << "files in" << double(t.elapsed()) / 1000.0 << "s";
    responseIq.setType(QXmppIq::Result);
    responseIq.setCollections(collections);
    emit searchFinished(responseIq);
}

IndexThread::IndexThread(const QSqlDatabase &database, const QDir &dir, QObject *parent)
    : QThread(parent), scanCount(0), sharesDb(database), sharesDir(dir)
{
};

void IndexThread::run()
{
    QTime t;
    t.start();
    scanDir(sharesDir);
    qDebug() << "Scanned" << scanCount << "files in" << double(t.elapsed()) / 1000.0 << "s";
}

void IndexThread::scanDir(const QDir &dir)
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

