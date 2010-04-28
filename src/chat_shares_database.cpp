/*
 * wiLink
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
#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QTime>
#include <QTimer>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat_shares_database.h"

Q_DECLARE_METATYPE(QXmppShareSearchIq)

#define SEARCH_MAX_TIME 5000

static ChatSharesDatabase::Entry getFile(const QSqlQuery &selectQuery)
{
    ChatSharesDatabase::Entry cached;
    cached.path = selectQuery.value(0).toString();
    cached.size = selectQuery.value(1).toInt();
    cached.hash = QByteArray::fromHex(selectQuery.value(2).toByteArray());
    cached.date = selectQuery.value(3).toDateTime();
    return cached;
}

/** Fingerprint a file.
 */
static QByteArray hashFile(const QString &path)
{
    QFile fp(path);

    if (!fp.open(QIODevice::ReadOnly) || fp.isSequential())
        return QByteArray();

    QCryptographicHash hasher(QCryptographicHash::Md5);
    char buffer[16384];
    int hashed = 0;
    while (fp.bytesAvailable())
    {
        int len = fp.read(buffer, sizeof(buffer));
        if (len < 0)
            return QByteArray();
        hasher.addData(buffer, len);
        hashed += len;
    }
    return hasher.result();
}

ChatSharesDatabase::ChatSharesDatabase(const QString &path, QObject *parent)
    : QObject(parent), sharesDir(path)
{
    // prepare database
    sharesDb = QSqlDatabase::addDatabase("QSQLITE");
    sharesDb.setDatabaseName(":memory:");
    Q_ASSERT(sharesDb.open());
    sharesDb.exec("CREATE TABLE files (path TEXT, date DATETIME, size INT, hash VARCHAR(32))");
    sharesDb.exec("CREATE UNIQUE INDEX files_path ON files (path)");

    // start indexing
    indexTimer = new QTimer(this);
    indexTimer->setInterval(60 * 60 * 1000); // 1 hour
    indexTimer->setSingleShot(true);
    connect(indexTimer, SIGNAL(timeout()), this, SLOT(index()));
}

QSqlDatabase ChatSharesDatabase::database() const
{
    return sharesDb;
}

QDir ChatSharesDatabase::directory() const
{
    return sharesDir;
}

/** Handle a get request.
  */
void ChatSharesDatabase::get(const QXmppShareGetIq &requestIq)
{
    QThread *worker = new GetThread(this, requestIq);
    connect(worker, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(worker, SIGNAL(finished()),
            worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(getFinished(QXmppShareGetIq, QXmppShareItem)),
            this, SIGNAL(getFinished(QXmppShareGetIq, QXmppShareItem)));
    worker->start();
 }

/** Update database of available files.
 */
void ChatSharesDatabase::index()
{
    // start indexing
    QThread *worker = new IndexThread(this);
    connect(worker, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(worker, SIGNAL(finished()),
            worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(finished()),
            indexTimer, SLOT(start()));
    connect(worker, SIGNAL(indexFinished(double, int, int, int)),
            this, SIGNAL(indexFinished(double, int, int, int)));
    emit indexStarted();
    worker->start();
}

QString ChatSharesDatabase::jid() const
{
    return sharesJid;
}

void ChatSharesDatabase::setJid(const QString &jid)
{
    sharesJid = jid;
}

/** Handle a search request.
 */
void ChatSharesDatabase::search(const QXmppShareSearchIq &requestIq)
{
    QThread *worker = new SearchThread(this, requestIq);
    connect(worker, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(worker, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SIGNAL(searchFinished(const QXmppShareSearchIq&)));
    worker->start();
}

bool ChatSharesDatabase::deleteFile(const QString &path)
{
    QSqlQuery deleteQuery("DELETE FROM files WHERE path = :path", sharesDb);
    deleteQuery.bindValue(":path", path);
    return deleteQuery.exec();
}

bool ChatSharesDatabase::saveFile(const ChatSharesDatabase::Entry &entry)
{
    QSqlQuery query("SELECT 1 AS a FROM files WHERE path = :path", sharesDb);
    query.bindValue(":path", entry.path);
    if (query.exec() && query.next())
    {
        QSqlQuery updateQuery("UPDATE files SET date = :date, size = :size, hash = :hash "
                               "WHERE path = :path", sharesDb);
        updateQuery.bindValue(":date", entry.date);
        updateQuery.bindValue(":size", entry.size);
        updateQuery.bindValue(":hash", entry.hash.toHex());
        updateQuery.bindValue(":path", entry.path);
        return updateQuery.exec();
    } else {
        QSqlQuery insertQuery("INSERT INTO files (path, date, size, hash) "
                               "VALUES(:path, :date, :size, :hash)", sharesDb);
        insertQuery.bindValue(":path", entry.path);
        insertQuery.bindValue(":date", entry.date);
        insertQuery.bindValue(":size", entry.size);
        insertQuery.bindValue(":hash", entry.hash.toHex());
        return insertQuery.exec();
    }
}

bool ChatSharesDatabase::updateFile(ChatSharesDatabase::Entry &cached, bool updateHash)
{
    // check file is still readable
    QFileInfo info(sharesDir.filePath(cached.path));
    if (!info.isReadable())
    {
        deleteFile(cached.path);
        return false;
    }

    // check whether we need to calculate checksum
    if (cached.date != info.lastModified() || cached.size != info.size())
        cached.hash = QByteArray();
    if (updateHash && cached.hash.isEmpty())
    {
        cached.hash = hashFile(info.filePath());
        if (cached.hash.isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "Error hashing file " + cached.path);
            deleteFile(cached.path);
            return false;
        }
    }

    // update database entry
    if (cached.date != info.lastModified() || cached.size != info.size())
    {
        cached.date = info.lastModified();
        cached.size = info.size();
        saveFile(cached);
    }

    return true;
}

ChatSharesThread::ChatSharesThread(ChatSharesDatabase *database)
    : QThread(database), sharesDatabase(database)
{
}

GetThread::GetThread(ChatSharesDatabase *database, const QXmppShareGetIq &request)
    : ChatSharesThread(database), requestIq(request)
{
}

void GetThread::run()
{
    QXmppShareGetIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setTo(requestIq.from());
    responseIq.setType(QXmppIq::Result);

    // look for file in database
    QXmppShareItem shareFile(QXmppShareItem::FileItem);
    QSqlQuery query("SELECT path, size, hash, date FROM files WHERE path = :path", sharesDatabase->database());
    query.bindValue(":path", requestIq.node());
    if (query.exec() && query.next())
    {
        ChatSharesDatabase::Entry cached = getFile(query);
        if (sharesDatabase->updateFile(cached, true))
        {
            // fill meta-data
            shareFile.setName(QFileInfo(cached.path).fileName());
            shareFile.setFileDate(cached.date);
            shareFile.setFileHash(cached.hash);
            shareFile.setFileSize(cached.size);

            QXmppShareLocation location(requestIq.to());
            location.setNode(cached.path);
            shareFile.setLocations(location);

            // FIXME : for some reason, random number generation fails
            //responseIq.setSid(generateStanzaHash());
            emit getFinished(responseIq, shareFile);
            return;
        }
    }

    logMessage(QXmppLogger::WarningMessage, "Could not find local file " + requestIq.node());
    QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::ItemNotFound);
    responseIq.setError(error);
    responseIq.setType(QXmppIq::Error);
    emit getFinished(responseIq, shareFile);
}

IndexThread::IndexThread(ChatSharesDatabase *database)
    : ChatSharesThread(database), scanAdded(0), scanUpdated(0)
{
}

void IndexThread::run()
{
    QDir sharesDir = sharesDatabase->directory();
    logMessage(QXmppLogger::DebugMessage, "Scan started for " + sharesDir.path());

    // store existing entries
    QSqlQuery query("SELECT path, size, hash, date FROM files", sharesDatabase->database());
    query.exec();
    while (query.next())
    {
        ChatSharesDatabase::Entry item = getFile(query);
        scanOld.insert(item.path, item);
    }

    // perform scan
    QTime t;
    t.start();
    scanDir(sharesDir);

    // remove obsolete entries
    foreach (const QString &path, scanOld.keys())
        sharesDatabase->deleteFile(path);

    const double elapsed = double(t.elapsed()) / 1000.0;
    logMessage(QXmppLogger::DebugMessage, QString("Scan completed in %1 s ( %2 added, %3 updated, %4 removed )")
               .arg(QString::number(elapsed))
               .arg(scanAdded)
               .arg(scanUpdated)
               .arg(scanOld.size()));
    emit indexFinished(elapsed, scanAdded, scanUpdated, scanOld.size());
}

void IndexThread::scanDir(const QDir &dir)
{
    QDir sharesDir = sharesDatabase->directory();
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable))
    {
        if (info.isDir())
        {
            scanDir(QDir(info.filePath()));
        } else {
            const QString relativePath = sharesDir.relativeFilePath(info.filePath());
            ChatSharesDatabase::Entry cached = scanOld.take(relativePath);

            // database entry is up to date, do nothing
            if (cached.path == relativePath &&
                cached.date == info.lastModified() &&
                cached.size == info.size())
            {
                scanUpdated++;
                continue;
            }

            if (cached.path.isEmpty())
                scanAdded++;
            else
                scanUpdated++;

            // update database entry
            cached.path = relativePath;
            cached.size = info.size();
            cached.date = info.lastModified();
            cached.hash = QByteArray();
            sharesDatabase->saveFile(cached);
        }
    }
}

SearchThread::SearchThread(ChatSharesDatabase *database, const QXmppShareSearchIq &request)
    : ChatSharesThread(database), requestIq(request)
{
};

void SearchThread::run()
{
    QXmppShareSearchIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setTo(requestIq.from());
    responseIq.setTag(requestIq.tag());

    // determine query type
    QXmppShareLocation location;
    location.setJid(sharesDatabase->jid());
    location.setNode(requestIq.node());
    responseIq.collection().setLocations(location);

    // clean input
    QString basePrefix = requestIq.node();
    if (!basePrefix.isEmpty() && !basePrefix.endsWith("/"))
        basePrefix += "/";
    int queryDepth = requestIq.depth();
    const QString queryString = requestIq.search().trimmed();
    if (queryString.contains("\\"))
    {
        logMessage(QXmppLogger::WarningMessage, "Received an invalid search: " + queryString);
        QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::BadRequest);
        responseIq.setError(error);
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    // check the base path exists
    QDir sharesDir = sharesDatabase->directory();
    QFileInfo info(sharesDir.filePath(basePrefix));
    if (!basePrefix.isEmpty() && !info.exists())
    {
        logMessage(QXmppLogger::WarningMessage, "Base path no longer exists: " + basePrefix);

        // remove obsolete DB entries
        QString like = basePrefix;
        like.replace("%", "\\%");
        like.replace("_", "\\_");
        like += "%";
        QSqlDatabase sharesDb = sharesDatabase->database();
        QSqlQuery query("DELETE FROM files WHERE PATH LIKE :search ESCAPE :escape", sharesDb);
        query.bindValue(":search", like);
        query.bindValue(":escape", "\\");
        query.exec();

        QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::ItemNotFound);
        responseIq.setError(error);
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    // perform query
    search(responseIq.collection(), basePrefix, queryString, queryDepth);

    // send response
    responseIq.setType(QXmppIq::Result);
    emit searchFinished(responseIq);
}

void SearchThread::search(QXmppShareItem &rootCollection, const QString &basePrefix, const QString &queryString, int maxDepth)
{
    QTime t;
    t.start();

    // prepare SQL query
    QString like;
    if (!queryString.isEmpty())
    {
        like = queryString;
        like.replace("%", "\\%");
        like.replace("_", "\\_");
        like = "%" + like + "%";
    }
    else if (!basePrefix.isEmpty())
    {
        like = basePrefix;
        like.replace("%", "\\%");
        like.replace("_", "\\_");
        like += "%";
    }

    QString sql("SELECT path, size, hash, date FROM files");
    if (!like.isEmpty())
        sql += " WHERE path LIKE :search ESCAPE :escape";
    sql += " ORDER BY path";
    QSqlDatabase sharesDb = sharesDatabase->database();
    QSqlQuery query(sql, sharesDb);
    if (!like.isEmpty())
    {
        query.bindValue(":search", like);
        query.bindValue(":escape", "\\");
    }

    // store results
    int fileCount = 0;
    query.exec();
    QMap<QString, QXmppShareItem*> subDirs;
    QList<ChatSharesDatabase::Entry> dbHits;
    while (query.next())
        dbHits << getFile(query);

    for (int hit = 0; hit < dbHits.size(); hit++)
    {
        ChatSharesDatabase::Entry &cached = dbHits[hit];
        const QString path = cached.path;

        QString prefix;
        if (!queryString.isEmpty())
        {
            int matchIndex = path.indexOf(queryString, 0, Qt::CaseInsensitive);
            if (matchIndex < 0)
            {
                logMessage(QXmppLogger::WarningMessage, QString("Could not find %1 in %2").arg(queryString, path));
                continue;
            }
            int slashIndex = path.lastIndexOf("/", matchIndex);
            if (slashIndex >= 0)
                prefix = path.left(slashIndex + 1);
        } else {
            prefix = basePrefix;
        }

        // find the depth at which we matched
        QStringList relativeBits = path.mid(prefix.size()).split("/");
        if (!relativeBits.size())
        {
            logMessage(QXmppLogger::WarningMessage, "Query returned an empty path");
            continue;
        }
        relativeBits.removeLast();

        QXmppShareItem *parentCollection = &rootCollection;
        QString dirPath;
        const int maxBits = maxDepth ? qMin(maxDepth, relativeBits.size()) : relativeBits.size();
        for (int i = 0; i < maxBits; i++)
        {
            const QString dirName = relativeBits[i];
            if (!dirPath.isEmpty())
                dirPath += "/";
            dirPath += dirName;

            if (!subDirs.contains(dirPath))
            {
                QXmppShareItem collection(QXmppShareItem::CollectionItem);
                collection.setName(dirName);
                QXmppShareLocation location;
                location.setJid(sharesDatabase->jid());
                location.setNode(prefix + dirPath + "/");
                collection.setLocations(location);
                subDirs[dirPath] = parentCollection->appendChild(collection);
            }
            parentCollection = subDirs[dirPath];
        }

        if (!maxDepth || relativeBits.size() < maxDepth)
        {
            // update file info
            if (sharesDatabase->updateFile(cached, requestIq.hash()))
            {
                QXmppShareItem shareFile(QXmppShareItem::FileItem);
                shareFile.setName(QFileInfo(cached.path).fileName());
                shareFile.setFileDate(cached.date);
                shareFile.setFileHash(cached.hash);
                shareFile.setFileSize(cached.size);

                QXmppShareLocation location(requestIq.to());
                location.setNode(cached.path);
                shareFile.setLocations(location);

                fileCount++;
                parentCollection->appendChild(shareFile);
            }
        }

        if (t.elapsed() > SEARCH_MAX_TIME)
        {
            logMessage(QXmppLogger::WarningMessage, QString("Search truncated at %1 results after %2 s").arg(fileCount).arg(double(t.elapsed()) / 1000.0));
            break;
        }
    }
}

