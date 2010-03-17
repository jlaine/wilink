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
#include <QResizeEvent>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QTreeWidget>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_shares.h"
#include "chat_shares_p.h"
#include "chat_transfers.h"

enum Columns
{
    NameColumn,
    SizeColumn,
};

enum Roles
{
    HashRole = Qt::UserRole,
    SizeRole,
    MirrorsRole,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), client(xmppClient), db(0)
{
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("Shares"));

    qRegisterMetaType<QXmppShareSearchIq>("QXmppShareSearchIq");
    sharesDir = QDir(QDir::home().filePath("Public"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addItem(statusBar());
    layout->addWidget(new QLabel(tr("Enter the name of the file you are looking for.")));
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    layout->addWidget(lineEdit);

    treeWidget = new ChatSharesView;
    connect(treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*)));
    clearView();
    layout->addWidget(treeWidget);

    setLayout(layout);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
}

qint64 ChatShares::addCollection(const QXmppShareIq::Collection &collection, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *collectionItem = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(treeWidget);
    collectionItem->setIcon(NameColumn, collectionIcon);
    collectionItem->setText(NameColumn, collection.name());

    qint64 collectionSize = 0;
    foreach (const QXmppShareIq::Collection &child, collection.collections())
        collectionSize += addCollection(child, collectionItem);
    foreach (const QXmppShareIq::File &file, collection)
        collectionSize += addFile(file, collectionItem);
    collectionItem->setText(SizeColumn, ChatTransfers::sizeToString(collectionSize));
    return collectionSize;
}

qint64 ChatShares::addFile(const QXmppShareIq::File &file, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *fileItem = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(treeWidget);
    fileItem->setData(NameColumn, HashRole, file.hash());
    fileItem->setData(NameColumn, SizeRole, file.size());
    fileItem->setData(NameColumn, MirrorsRole, file.mirrors());
    fileItem->setIcon(NameColumn, fileIcon);
    fileItem->setText(NameColumn, file.name());
    fileItem->setText(SizeColumn, ChatTransfers::sizeToString(file.size()));
    return file.size();
}

void ChatShares::clearView()
{
    treeWidget->clear();
    QTreeWidgetItem *headerItem = new QTreeWidgetItem;
    headerItem->setText(NameColumn, tr("Name"));
    headerItem->setText(SizeColumn, tr("Size"));
    treeWidget->setHeaderItem(headerItem);
}

void ChatShares::shareGetIqReceived(const QXmppShareGetIq &shareIq)
{
#if 0
    if (shareIq.from() != shareServer)
        return;
#endif

    if (shareIq.type() == QXmppIq::Get)
    {
        QXmppShareGetIq responseIq;
        responseIq.setId(shareIq.id());
        responseIq.setTo(shareIq.from());
        responseIq.setType(QXmppIq::Result);

        // check path is OK
        QString filePath = db->locate(shareIq.file());
        if (filePath.isEmpty())
        {
            qWarning() << "Could not find file" << shareIq.file().name();
            responseIq.setType(QXmppIq::Error);
            client->sendPacket(responseIq);
            return;
        }
        responseIq.setSid(generateStanzaHash());
        client->sendPacket(responseIq);

        // send files
        qDebug() << "Sending" << shareIq.file().name() << "to" << shareIq.from();
        QXmppTransferJob *job = client->getTransferManager().sendFile(shareIq.from(), filePath, responseIq.sid());
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
    }
    else if (shareIq.type() == QXmppIq::Result)
    {
        // expect file
        qDebug() << "Expecting file transfer" << shareIq.sid();
        emit fileExpected(shareIq.sid());
    }
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
#if 0
    if (shareIq.from() != shareServer)
        return;
#endif

    if (shareIq.type() == QXmppIq::Get)
    {
        db->search(shareIq);
    }
    else if (shareIq.type() == QXmppIq::Result)
    {
        lineEdit->setEnabled(true);
        QFileIconProvider iconProvider;
        foreach (const QXmppShareIq::Collection &collection, shareIq.collection().collections())
            addCollection(collection, 0);
        foreach (const QXmppShareIq::File &file, shareIq.collection())
            addFile(file, 0);
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        lineEdit->setEnabled(true);
    }
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
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

    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
}

void ChatShares::itemDoubleClicked(QTreeWidgetItem *item)
{
    const QStringList mirrors = item->data(NameColumn, MirrorsRole).toStringList();
    if (mirrors.isEmpty())
    {
        qWarning() << "No mirror for file" << item->text(NameColumn);
        return; 
    }

    QXmppShareIq::File file;
    file.setName(item->text(NameColumn));
    file.setHash(item->data(NameColumn, HashRole).toByteArray());
    file.setSize(item->data(NameColumn, SizeRole).toInt());

    // request file
    qDebug() << "Requesting" << file.name() << "from" << mirrors.first();
    QXmppShareGetIq iq;
    iq.setTo(mirrors.first());
    iq.setType(QXmppIq::Get);
    iq.setFile(file);
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
        connect(db, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SLOT(searchFinished(const QXmppShareSearchIq&)));
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

QString ChatSharesDatabase::locate(const QXmppShareIq::File &file)
{
    QSqlQuery query("SELECT path FROM files WHERE hash = :hash AND size = :size", sharesDb);
    query.bindValue(":hash", file.hash().toHex());
    query.bindValue(":size", file.size());
    query.exec();
    if (!query.next())
        return QString();
    return sharesDir.filePath(query.value(0).toString());
}

void ChatSharesDatabase::search(const QXmppShareSearchIq &requestIq)
{
    QThread *worker = new SearchThread(sharesDb, sharesDir, requestIq, this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SIGNAL(searchFinished(const QXmppShareSearchIq&)));
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

SearchThread::SearchThread(const QSqlDatabase &database, const QDir &dir, const QXmppShareSearchIq &request, QObject *parent)
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
    QXmppShareSearchIq responseIq;
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

    QXmppShareIq::Collection rootCollection;
    int searchCount = 0;
    while (query.next())
    {
        QXmppShareIq::File file;
        file.setName(query.value(0).toString());
        file.setSize(query.value(1).toInt());
        file.setHash(QByteArray::fromHex(query.value(2).toByteArray()));
        file.setMirrors(QStringList() << requestIq.to());

        // find the depth at which we matched
        const QString path = file.name();
        QString matchString;
        QRegExp subdirRe(".*(([^/]+)/[^/]+)/[^/]+");
        QRegExp dirRe(".*([^/]+)/[^/]+");
        QRegExp fileRe(".*([^/]+)");
        if (subdirRe.exactMatch(path) && subdirRe.cap(2).contains(queryString, Qt::CaseInsensitive))
            matchString = subdirRe.cap(1);
        else if (dirRe.exactMatch(path) && dirRe.cap(1).contains(queryString, Qt::CaseInsensitive))
            matchString = subdirRe.cap(1); 
        else if (fileRe.exactMatch(path) && fileRe.cap(1).contains(queryString, Qt::CaseInsensitive))
            matchString = "";
        else
            continue;

        // update file info
        if (!updateFile(file))
            continue;
        file.setName(QFileInfo(file.name()).fileName());

        // add file to the appropriate collection
        rootCollection.mkpath(matchString).append(file);

        // limit maximum search time to 15s
        searchCount++;
        if (t.elapsed() > 15000 || searchCount > 250)
            break;
    }

    // send response
    qDebug() << "Found" << searchCount << "files in" << double(t.elapsed()) / 1000.0 << "s";
    responseIq.setType(QXmppIq::Result);
    responseIq.setCollection(rootCollection);
    emit searchFinished(responseIq);
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(2);
    setColumnWidth(SizeColumn, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeWidget::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - 80);
}

