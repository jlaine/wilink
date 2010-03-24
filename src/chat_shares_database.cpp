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
#define HASH_MAX_SIZE 1024 * 1024

ChatSharesDatabase::ChatSharesDatabase(const QString &path, QObject *parent)
    : QObject(parent), sharesDir(path)
{
    // prepare database
    sharesDb = QSqlDatabase::addDatabase("QSQLITE");
    sharesDb.setDatabaseName(":memory:");
    Q_ASSERT(sharesDb.open());
    sharesDb.exec("CREATE TABLE files (path text, size int, hash varchar(32))");
    sharesDb.exec("CREATE UNIQUE INDEX files_path ON files (path)");

    // start indexing
    QThread *worker = new IndexThread(sharesDb, sharesDir, this);
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    worker->start();
}

QString ChatSharesDatabase::locate(const QString &publishId)
{
    QSqlQuery query("SELECT path FROM files WHERE path = :path", sharesDb);
    query.bindValue(":path", publishId);
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

bool SearchThread::updateFile(QXmppShareItem &file, const QSqlQuery &selectQuery)
{
    const QString path = selectQuery.value(0).toString();
    qint64 cachedSize = selectQuery.value(1).toInt();
    QByteArray cachedHash = QByteArray::fromHex(selectQuery.value(2).toByteArray());

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
        QCryptographicHash hasher(QCryptographicHash::Md5);
        char buffer[16384];
        int hashed = 0;
        while (fp.bytesAvailable() && hashed < HASH_MAX_SIZE)
        {
            int len = fp.read(buffer, sizeof(buffer));
            if (len < 0)
            {
                qWarning() << "Error reading from file" << path;
                return false;
            }
            hasher.addData(buffer, len);
            hashed += len;
        }
        fp.close();
        cachedHash = hasher.result();
        cachedSize = info.size();

        // update database entry
        updateQuery.bindValue(":hash", cachedHash.toHex());
        updateQuery.bindValue(":size", cachedSize);
        updateQuery.bindValue(":path", path);
        updateQuery.exec();
    }

    // fill meta-data
    file.setName(info.fileName());
    file.setFileHash(cachedHash);
    file.setFileSize(cachedSize);

    QXmppShareMirror mirror(requestIq.to());
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
    QXmppShareMirror mirror;
    mirror.setJid(requestIq.to());
    mirror.setPath(requestIq.base());
    responseIq.collection().setMirrors(mirror);

    // clean input
    QString basePrefix = requestIq.base();
    if (!basePrefix.isEmpty() && !basePrefix.endsWith("/"))
        basePrefix += "/";
    const QString queryString = requestIq.search().trimmed();
    if (queryString.contains("\\"))
    {
        qWarning() << "Received an invalid search" << queryString;
        responseIq.collection().clearChildren();
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    // perform query
    search(responseIq.collection(), basePrefix, queryString);

    // send response
    responseIq.setType(QXmppIq::Result);
    emit searchFinished(responseIq);
}

void SearchThread::search(QXmppShareItem &rootCollection, const QString &basePrefix, const QString &queryString)
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

    QString sql("SELECT path, size, hash FROM files");
    if (!like.isEmpty())
        sql += " WHERE path LIKE :search ESCAPE :escape";
    sql += " ORDER BY path";
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
    while (query.next())
    {
        const QString path = query.value(0).toString();

        QString prefix;
        if (!queryString.isEmpty())
        {
            int matchIndex = path.indexOf(queryString, 0, Qt::CaseInsensitive);
            if (matchIndex < 0)
            {
                qWarning() << "Could not find" << queryString << "in" << path;
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
            qWarning("Query returned an empty path");
            continue;
        }
        relativeBits.removeLast();

        const int maxDepth = 1;
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
                QXmppShareMirror mirror(requestIq.to());
                mirror.setPath(prefix + dirPath + "/");
                collection.setMirrors(mirror);
                subDirs[dirPath] = parentCollection->appendChild(collection);
            }
            parentCollection = subDirs[dirPath];
        }

        if (!maxDepth || relativeBits.size() < maxDepth)
        {
            // update file info
            QXmppShareItem file(QXmppShareItem::FileItem);
            if (updateFile(file, query))
            {
                fileCount++;
                parentCollection->appendChild(file);
            }
        }

        if (t.elapsed() > SEARCH_MAX_TIME)
        {
            qWarning() << "Search truncated at" << fileCount << "results after" << double(t.elapsed()) / 1000.0 << "s";
            break;
        }
    }
}

