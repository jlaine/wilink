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
#include "chat_shares_p.h"
#include "chat_transfers.h"

enum Columns
{
    NameColumn,
    SizeColumn,
};

Q_DECLARE_METATYPE(QXmppShareIq)

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : QWidget(parent), client(xmppClient), db(0)
{
    qRegisterMetaType<QXmppShareIq>("QXmppShareIq");
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
    clearView();
    layout->addWidget(tableWidget);

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareIqReceived(const QXmppShareIq&)), this, SLOT(shareIqReceived(const QXmppShareIq&)));
}

void ChatShares::clearView()
{
    tableWidget->clear();
    tableWidget->setHorizontalHeaderItem(NameColumn, new QTableWidgetItem(tr("Name")));
    tableWidget->setHorizontalHeaderItem(SizeColumn, new QTableWidgetItem(tr("Size")));
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

void ChatShares::searchFinished(const QXmppShareIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();
    if (search.isEmpty())
        return;

    lineEdit->setEnabled(false);
    clearView();

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
        connect(db, SIGNAL(searchFinished(const QXmppShareIq&)), this, SLOT(searchFinished(const QXmppShareIq&)));
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

    // start indexing
    QThread *worker = new IndexThread(sharesDb, sharesDir, this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}

void ChatSharesDatabase::search(const QXmppShareIq &requestIq)
{
    QThread *worker = new SearchThread(sharesDb, sharesDir, requestIq, this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(searchFinished(const QXmppShareIq&)), this, SIGNAL(searchFinished(const QXmppShareIq&)));
    worker->start();
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

    foreach (const QFileInfo &info, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable))
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

SearchThread::SearchThread(const QSqlDatabase &database, const QDir &dir, const QXmppShareIq &request, QObject *parent)
    : QThread(parent), requestIq(request), sharesDb(database), sharesDir(dir)
{
};

bool SearchThread::updateFile(QXmppShareIq::File &file)
{
    QCryptographicHash hasher(QCryptographicHash::Md5);
    QSqlQuery deleteQuery("DELETE FROM files WHERE path = :path", sharesDb);
    QSqlQuery updateQuery("UPDATE files SET hash = :hash, size = :size WHERE path = :path", sharesDb);

    // check file is still readable
    QFileInfo info(sharesDir.filePath(file.name()));
    if (!info.isReadable())
    {
        deleteQuery.bindValue(":path", file.name());
        deleteQuery.exec();
        return false;
    }

    // check whether we need to calculate checksum
    if (file.hash().isEmpty() || info.size() != file.size())
    {
        QFile fp(info.filePath());

        // if we cannot open the file, remove it from database
        if (!fp.open(QIODevice::ReadOnly))
        {
            qWarning() << "Failed to open file" << file.name();
            deleteQuery.bindValue(":path", file.name());
            deleteQuery.exec();
            return false;
        }

        // update file object
        while (fp.bytesAvailable())
            hasher.addData(fp.read(16384));
        fp.close();
        file.setHash(hasher.result());
        file.setSize(info.size());

        // update database entry
        updateQuery.bindValue(":hash", file.hash().toHex());
        updateQuery.bindValue(":size", file.size());
        updateQuery.bindValue(":path", file.name());
        updateQuery.exec();
    }
    return true;
}

void SearchThread::run()
{
    QXmppShareIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setTo(requestIq.from());
    responseIq.setTag(requestIq.tag());

    // validate search
    const QString queryString = requestIq.search();
    if (queryString.isEmpty() || queryString.trimmed().isEmpty() ||
        queryString.contains("/") ||
        queryString.contains("\\"))
    {
        qWarning() << "Received an invalid search" << queryString;
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    QCryptographicHash hasher(QCryptographicHash::Md5);
    QTime t;
    t.start();

    // perform search
    QString like = queryString;
    like.replace("%", "\\%");
    like.replace("_", "\\_");
    like = "%" + like + "%";
    QSqlQuery query("SELECT path, size, hash FROM files WHERE path LIKE :search ESCAPE :escape ORDER BY path", sharesDb);
    query.bindValue(":search", like);
    query.bindValue(":escape", "\\");
    query.exec();

    QList<QXmppShareIq::Collection> collections;
    QXmppShareIq::Collection baseCollection;
    QXmppShareIq::Collection currentCollection;
    int searchCount = 0;
    int lastDepth = -1;
    QString lastString;
    while (query.next())
    {
        QXmppShareIq::File file;
        file.setName(query.value(0).toString());
        file.setSize(query.value(1).toInt());
        file.setHash(QByteArray::fromHex(query.value(2).toByteArray()));

        // find the depth at which we matched
        int matchDepth;
        QString matchString;
        for (matchDepth = 2; matchDepth >= 0; matchDepth--)
        {
            QString reString(".*([^/]+)");
            for (int i = 0; i < matchDepth; ++i) reString += "/[^/]+";
            QRegExp re(reString);
            if (re.exactMatch(file.name()) && re.cap(1).contains(queryString, Qt::CaseInsensitive))
            {
                //qDebug() << "Matched at depth" << matchDepth << file.name() << re.cap(1);
                matchString = re.cap(1); 
                break;
            }
        }
        if (matchDepth < 0 || !updateFile(file))
            continue;

        // add file to the appropriate collection
        searchCount++;
        file.setName(QFileInfo(file.name()).fileName());
        if (!matchDepth)
            baseCollection.append(file);
        else if (matchDepth == lastDepth && matchString == lastString)
        {
            currentCollection.append(file);
        }
        else
        {
            if (!currentCollection.isEmpty())
            {
                collections.append(currentCollection);
                // qDebug() << "Finished collection" << currentCollection.name();
                currentCollection.clear();
            }
            currentCollection.setName(matchString);
            // qDebug() << "Starting collection" << currentCollection.name();
        }
        lastDepth = matchDepth;
        lastString = matchString;
    }
    if (!currentCollection.isEmpty())
    {
        collections.append(currentCollection);
        // qDebug() << "Finished collection" << currentCollection.name();
    }
    collections.append(baseCollection);

    // send response
    qDebug() << "Found" << searchCount << "files in" << double(t.elapsed()) / 1000.0 << "s";
    responseIq.setType(QXmppIq::Result);
    responseIq.setCollections(collections);
    emit searchFinished(responseIq);
}
