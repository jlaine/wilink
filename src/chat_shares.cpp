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
#include <QStackedWidget>
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
    TypeRole,
    MirrorRole,
    PathRole,
};

enum Types
{
    CollectionType = 0,
    FileType = 1,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

#define SEARCH_MAX_TIME 15000

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
    layout->addWidget(treeWidget);

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
}

void ChatShares::goBack()
{
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
    if (shareIq.type() == QXmppIq::Get)
    {
        db->search(shareIq);
        return;
    }

    lineEdit->setEnabled(true);
    if (shareIq.type() == QXmppIq::Result)
    {
        QTreeWidgetItem *parent = treeWidget->findItem(shareIq.collection(), 0);
        if (!parent)
            treeWidget->clear();
        foreach (const QXmppShareIq::Item &item, shareIq.collection().children())
            treeWidget->addItem(item, parent);
        if (parent)
            parent->setExpanded(true);
    }
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();

    lineEdit->setEnabled(false);

    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
}

void ChatShares::itemDoubleClicked(QTreeWidgetItem *item)
{
    const QString jid = item->data(NameColumn, MirrorRole).toString();
    const QString path = item->data(NameColumn, PathRole).toString();
    if (jid.isEmpty())
    {
        qWarning() << "No mirror for item" << item->text(NameColumn);
        return; 
    }

    int type = item->data(NameColumn, TypeRole).toInt();
    if (type == FileType)
    {
        if (path.isEmpty())
        {
            qWarning() << "No path for file" << item->text(NameColumn);
            return;
        }

        QXmppShareIq::Item file(QXmppShareIq::Item::FileItem);
        file.setName(item->text(NameColumn));
        file.setFileHash(item->data(NameColumn, HashRole).toByteArray());
        file.setFileSize(item->data(NameColumn, SizeRole).toInt());

        // request file
        qDebug() << "Requesting" << file.name() << "from" << jid;
        QXmppShareGetIq iq;
        iq.setTo(jid);
        iq.setType(QXmppIq::Get);
        iq.setFile(file);
        client->sendPacket(iq);
    }
    else if (type == CollectionType && !item->childCount())
    {
        lineEdit->setEnabled(false);

        // browse files
        QXmppShareSearchIq iq;
        iq.setTo(jid);
        iq.setType(QXmppIq::Get);
        iq.setBase(path);
        client->sendPacket(iq);
    }
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

QString ChatSharesDatabase::locate(const QXmppShareIq::Item &file)
{
    QSqlQuery query("SELECT path FROM files WHERE hash = :hash AND size = :size", sharesDb);
    query.bindValue(":hash", file.fileHash().toHex());
    query.bindValue(":size", file.fileSize());
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

bool SearchThread::updateFile(QXmppShareIq::Item &file, const QSqlQuery &selectQuery)
{
    const QString path = selectQuery.value(0).toString();
    qint64 cachedSize = selectQuery.value(1).toInt();
    QByteArray cachedHash = QByteArray::fromHex(selectQuery.value(2).toByteArray());

    QCryptographicHash hasher(QCryptographicHash::Md5);
    QSqlQuery deleteQuery("DELETE FROM files WHERE path = :path", sharesDb);
    QSqlQuery updateQuery("UPDATE files SET hash = :hash, size = :size WHERE path = :path", sharesDb);

    // check file is still readable
    QFileInfo info(sharesDir.filePath(path));
    if (!info.isReadable())
    {
        deleteQuery.bindValue(":path", path);
        deleteQuery.exec();
        return false;
    }

    // check whether we need to calculate checksum
    if (cachedHash.isEmpty() || cachedSize == info.size())
    {
        QFile fp(info.filePath());

        // if we cannot open the file, remove it from database
        if (!fp.open(QIODevice::ReadOnly))
        {
            qWarning() << "Failed to open file" << path;
            deleteQuery.bindValue(":path", path);
            deleteQuery.exec();
            return false;
        }

        // update file object
        while (fp.bytesAvailable())
            hasher.addData(fp.read(16384));
        fp.close();
        cachedHash = hasher.result();
        cachedSize = info.size();

        // update database entry
        updateQuery.bindValue(":hash", cachedHash);
        updateQuery.bindValue(":size", cachedSize);
        updateQuery.bindValue(":path", path);
        updateQuery.exec();
    }

    // fill meta-data
    file.setName(info.fileName());
    file.setFileHash(cachedHash);
    file.setFileSize(cachedSize);

    QXmppShareIq::Mirror mirror(requestIq.to());
    mirror.setPath(path);
    file.setMirrors(mirror);

    return true;
}

void SearchThread::run()
{
    QXmppShareSearchIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setTo(requestIq.from());
    responseIq.setTag(requestIq.tag());

    // determine query type
    QXmppShareIq::Item rootCollection(QXmppShareIq::Item::CollectionItem);
    QXmppShareIq::Mirror mirror;
    mirror.setJid(requestIq.to());
    mirror.setPath(requestIq.base());
    rootCollection.setMirrors(mirror);
    const QString queryString = requestIq.search().trimmed();
    if (queryString.isEmpty())
    {
        if (!browse(rootCollection, requestIq.base()))
        {
            qWarning() << "Browse failed";
            responseIq.setType(QXmppIq::Error);
            emit searchFinished(responseIq);
            return;
        }
    } else {
        // perform search
        if (!search(rootCollection, queryString))
        {
            qWarning() << "Search" << queryString << "failed";
            responseIq.setType(QXmppIq::Error);
            emit searchFinished(responseIq);
            return;
        }
    }

    // send response
    responseIq.setType(QXmppIq::Result);
    responseIq.setCollection(rootCollection);
    emit searchFinished(responseIq);
}

bool SearchThread::browse(QXmppShareIq::Item &rootCollection, const QString &base)
{
    QTime t;
    t.start();

    QString prefix = base;
    if (!prefix.isEmpty() && !prefix.endsWith("/"))
        prefix += "/";

    QString sql("SELECT path, size, hash FROM files");
    if (!prefix.isEmpty())
        sql += " WHERE path LIKE :search ESCAPE :escape";
    sql += " ORDER BY path";
    QSqlQuery query(sql, sharesDb);

    if (!prefix.isEmpty())
    {
        QString like = prefix;
        like.replace("%", "\\%");
        like.replace("_", "\\_");
        like += "%";
        query.bindValue(":search", like);
        query.bindValue(":escape", "\\");
    }
    query.exec();

    QStringList subDirs;
    while (query.next())
    {
        const QString path = query.value(0).toString();
        const QString relativePath = path.mid(prefix.size());
        if (relativePath.count("/") == 0)
        {
            QXmppShareIq::Item file(QXmppShareIq::Item::FileItem);
            if (updateFile(file, query))
                rootCollection.appendChild(file);
        }
        else
        {
            const QString dirName = relativePath.split("/").first();
            if (subDirs.contains(dirName))
                continue;
            subDirs.append(dirName);

            QXmppShareIq::Item &collection = rootCollection.mkpath(dirName);
            QXmppShareIq::Mirror mirror(requestIq.to());
            mirror.setPath(prefix + dirName + "/");
            collection.setMirrors(mirror);
        }
        if (t.elapsed() > SEARCH_MAX_TIME)
            break;
    }
    qDebug() << "Browsed" << rootCollection.children().size() << "files in" << double(t.elapsed()) / 1000.0 << "s";
    return true;
}

bool SearchThread::search(QXmppShareIq::Item &rootCollection, const QString &queryString)
{
    if (queryString.contains("/") ||
        queryString.contains("\\"))
    {
        qWarning() << "Received an invalid search" << queryString;
        return false;
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

    int searchCount = 0;
    while (query.next())
    {
        const QString path = query.value(0).toString();

        // find the depth at which we matched
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
        QXmppShareIq::Item file(QXmppShareIq::Item::FileItem);
        if (updateFile(file, query))
        {
            // add file to the appropriate collection
            rootCollection.mkpath(matchString).appendChild(file);
            searchCount++;
        }

        // limit maximum search time to 15s
        if (t.elapsed() > SEARCH_MAX_TIME || searchCount > 250)
            break;
    }

    qDebug() << "Found" << searchCount << "files in" << double(t.elapsed()) / 1000.0 << "s";
    return true;
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(2);
    setColumnWidth(SizeColumn, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);
    peerIcon = iconProvider.icon(QFileIconProvider::Network);

    /* set header names */
    clear();
}

qint64 ChatSharesView::addItem(const QXmppShareIq::Item &file, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *fileItem = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(this);
    fileItem->setData(NameColumn, HashRole, file.fileHash());
    fileItem->setData(NameColumn, SizeRole, file.fileSize());

    /* FIXME : we are only using the first mirror */
    if (!file.mirrors().isEmpty())
    {
        QXmppShareIq::Mirror mirror = file.mirrors().first();
        fileItem->setData(NameColumn, MirrorRole, mirror.jid());
        fileItem->setData(NameColumn, PathRole, mirror.path());
    }

    fileItem->setText(NameColumn, file.name());
    if (file.type() == QXmppShareIq::Item::CollectionItem)
    {
        fileItem->setData(NameColumn, TypeRole, CollectionType);
        if (file.mirrors().first().path().isEmpty())
            fileItem->setIcon(NameColumn, peerIcon);
        else
            fileItem->setIcon(NameColumn, collectionIcon);
    } else {
        fileItem->setData(NameColumn, TypeRole, FileType);
        fileItem->setIcon(NameColumn, fileIcon);
    }

    /* calculate size */
    qint64 collectionSize = file.fileSize();
    foreach (const QXmppShareIq::Item &child, file.children())
        collectionSize += addItem(child, fileItem);
    fileItem->setText(SizeColumn, ChatTransfers::sizeToString(collectionSize));
    return collectionSize;
}

void ChatSharesView::clear()
{
    QTreeWidget::clear();
    QTreeWidgetItem *headerItem = new QTreeWidgetItem;
    headerItem->setText(NameColumn, tr("Name"));
    headerItem->setText(SizeColumn, tr("Size"));
    setHeaderItem(headerItem);
}

QTreeWidgetItem *ChatSharesView::findItem(const QXmppShareIq::Item &collection, QTreeWidgetItem *parent)
{
    if (collection.mirrors().isEmpty())
        return 0;

    /* FIXME : we are only using the first mirror */
    QXmppShareIq::Mirror mirror = collection.mirrors().first();
    QTreeWidgetItem *found = 0;
    if (parent)
    {
        if (mirror.jid() == parent->data(NameColumn, MirrorRole) &&
            mirror.path() == parent->data(NameColumn, PathRole))
            return parent;
        for (int i = 0; i < parent->childCount(); i++)
            if ((found = findItem(collection, parent->child(i))) != 0)
                return found;
    } else {
        for (int i = 0; i < topLevelItemCount(); i++)
            if ((found = findItem(collection, topLevelItem(i))) != 0)
                return found;
    }
    return 0;
}

void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeWidget::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - 80);
}

