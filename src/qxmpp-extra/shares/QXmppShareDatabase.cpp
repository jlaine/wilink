/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QTime>
#include <QTimer>

#include "QDjango.h"
#include "QDjangoQuerySet.h"
#include "QDjangoWhere.h"

#include "QXmppShareDatabase.h"
#include "QXmppShareDatabase_p.h"
#include "QXmppShareIq.h"
#include "QXmppUtils.h"

Q_DECLARE_METATYPE(QXmppShareSearchIq)

#define FILE_SCHEMA 1
#define SEARCH_MAX_TIME 5000

#define INDEX_INTERVAL (30 * 60 * 1000) // 30mn

#if defined(Q_OS_SYMBIAN) || defined(Q_OS_WIN)
static const QRegExp driveRegex("[a-zA-Z]:");
#endif

File::File(QObject *parent)
    : QDjangoModel(parent), m_popularity(0), m_size(0)
{
}

QDateTime File::date() const
{
    return m_date;
}

void File::setDate(const QDateTime &date)
{
    m_date = date;
}

QByteArray File::hash() const
{
    return m_hash;
}

void File::setHash(const QByteArray &hash)
{
    m_hash = hash;
}

QString File::path() const
{
    return m_path;
}

void File::setPath(const QString &path)
{
    m_path = path;
}

qint64 File::popularity() const
{
    return m_popularity;
}

void File::setPopularity(qint64 popularity)
{
    m_popularity = popularity;
}

qint64 File::size() const
{
    return m_size;
}

void File::setSize(qint64 size)
{
    m_size = size;
}

/** Update the database entry for a file.
 */
bool File::update(const QFileInfo &info, bool updateHash, bool increasePopularity, bool *stopFlag)
{
    // check file is still readable
    if (info.fileName().endsWith(".part") || !info.isReadable() || !info.size())
    {
        remove();
        return false;
    }

    // check whether we need to calculate checksum
    QByteArray fileHash = m_hash;
    if (m_date != info.lastModified() || m_size != info.size())
        fileHash = QByteArray();
    if (updateHash && fileHash.isEmpty())
    {
        QFile fp(info.filePath());
        if (!fp.open(QIODevice::ReadOnly) || fp.isSequential())
        {
            remove();
            return false;
        }

        QCryptographicHash hasher(QCryptographicHash::Md5);
        char buffer[16384];
        int hashed = 0;
        while (fp.bytesAvailable())
        {
            int len = fp.read(buffer, sizeof(buffer));
            if (len < 0)
            {
                remove();
                return false;
            } else if (*stopFlag) {
                return false;
            }
            hasher.addData(buffer, len);
            hashed += len;
        }

        fileHash = hasher.result();
    }

    // update database entry
    if (m_date != info.lastModified() ||
        m_hash != fileHash ||
        m_size != info.size() ||
        increasePopularity)
    {
        m_date = info.lastModified();
        m_hash = fileHash;
        m_size = info.size();
        if (increasePopularity)
            m_popularity++;
        save();
    }

    return true;
}

Schema::Schema(QObject *parent)
    : QDjangoModel(parent), m_version(0)
{
}

QString Schema::model() const
{
    return m_model;
}

void Schema::setModel(const QString &model)
{
    m_model = model;
}

int Schema::version() const
{
    return m_version;
}

void Schema::setVersion(int version)
{
    m_version = version;
}

QXmppShareDatabase::QXmppShareDatabase(QObject *parent)
    : QXmppLoggable(parent), indexThread(0)
{
    // register meta types
    qRegisterMetaType<QXmppShareItem>("QXmppShareItem");
    qRegisterMetaType<QXmppShareGetIq>("QXmppShareGetIq");
    qRegisterMetaType<QXmppShareSearchIq>("QXmppShareSearchIq");
    qRegisterMetaType<QXmppLogger::MessageType>("QXmppLogger::MessageType");

    // prepare database
    QDjangoMetaModel fileMeta = QDjango::registerModel<File>();
    QDjango::registerModel<Schema>().createTable();

    // check schema version
    Schema *schema = QDjangoQuerySet<Schema>().get(QDjangoWhere("model", QDjangoWhere::Equals, "File"));
    if (!schema || schema->version() != FILE_SCHEMA)
    {
        fileMeta.dropTable();
        fileMeta.createTable();
        if (!schema)
        {
            schema = new Schema;
            schema->setModel("File");
        }
        schema->setVersion(FILE_SCHEMA);
        schema->save();
    }
    if (schema)
        delete schema;

    // prepare file system watcher
    fsWatcher = new QFileSystemWatcher(this);
    connect(fsWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(slotDirectoryChanged(QString)));

    // prepare indexing
    indexTimer = new QTimer(this);
    indexTimer->setInterval(INDEX_INTERVAL);
    indexTimer->setSingleShot(true);
    connect(indexTimer, SIGNAL(timeout()), this, SLOT(index()));
}

QXmppShareDatabase::~QXmppShareDatabase()
{
    // stop all workers
    foreach (QXmppShareThread *worker, workers)
    {
        if (worker->isRunning())
        {
            worker->stop();
            worker->wait();
        }
    }
}

void QXmppShareDatabase::addWorker(QXmppShareThread *worker)
{
    connect(worker, SIGNAL(destroyed(QObject*)),
            this, SLOT(slotWorkerDestroyed(QObject*)));
    connect(worker, SIGNAL(finished()),
            worker, SLOT(deleteLater()));
    workers << worker;
    worker->start();
}

QString QXmppShareDatabase::directory() const
{
    return sharesPath;
}

void QXmppShareDatabase::setDirectory(const QString &path)
{
    if (path == sharesPath)
        return;

    // abort index thread
    if (indexThread && indexThread->isRunning())
    {
        indexThread->stop();
        indexThread->wait();
    }

    // purge database and stop watching old directories
    QDjangoQuerySet<File>().remove();
    QStringList oldPaths = fsWatcher->directories();
    if (!oldPaths.isEmpty())
        fsWatcher->removePaths(oldPaths);

    // create directory if needed
    QFileInfo info(path);
    if (!info.exists() && !info.dir().mkdir(info.fileName()))
       warning("Could not create shares directory: " + path);
    sharesPath = path;

    // schedule index
    indexTimer->setInterval(1000);
    indexTimer->start();

    emit directoryChanged(sharesPath);
}

QStringList QXmppShareDatabase::mappedDirectories() const
{
    return mappedPaths.values();
}

void QXmppShareDatabase::setMappedDirectories(const QStringList &paths)
{
    // check the list actually changed
    QStringList sortedNew = paths;
    qSort(sortedNew);
    QStringList sortedOld = mappedPaths.values();
    qSort(sortedOld);
    if (sortedNew == sortedOld)
        return;

    // abort index thread
    if (indexThread && indexThread->isRunning())
    {
        indexThread->stop();
        indexThread->wait();
    }

    // purge database and stop watching old directories
    QDjangoQuerySet<File>().remove();
    QStringList oldPaths = fsWatcher->directories();
    if (!oldPaths.isEmpty())
        fsWatcher->removePaths(oldPaths);

    // add mapped directories
    mappedPaths.clear();
    foreach (const QString &path, paths)
    {
        QCryptographicHash hasher(QCryptographicHash::Sha1);
        hasher.addData(path.toUtf8());
        QString hash = hasher.result().toHex();
        mappedPaths[hash] = path;
    }

    // schedule index
    indexTimer->setInterval(1000);
    indexTimer->start();
}

QString QXmppShareDatabase::filePath(const QString &node) const
{
    Q_ASSERT(!sharesPath.isEmpty());
    QRegExp rex("^([0-9a-f]+)(/(.*))?$");
    if (rex.exactMatch(node) && mappedPaths.contains(rex.cap(1)))
        return QDir(mappedPaths.value(rex.cap(1))).filePath(rex.cap(3));
    return QDir(sharesPath).filePath(node);
}

QString QXmppShareDatabase::fileNode(const QString &path) const
{
    Q_ASSERT(!sharesPath.isEmpty());
    foreach (const QString &key, mappedPaths.keys())
    {
        const QString dirPath = mappedPaths.value(key);
        if (path == dirPath)
            return key;
        else if (path.startsWith(dirPath + "/"))
            return QString("%1/%2").arg(key, QDir(dirPath).relativeFilePath(path));
    }
    if (path.startsWith(sharesPath + "/"))
        return QDir(sharesPath).relativeFilePath(path);
    return QString();
}

/** Handle a get request.
  */
void QXmppShareDatabase::get(const QXmppShareGetIq &requestIq)
{
    if (sharesPath.isEmpty() || requestIq.type() != QXmppIq::Get)
        return;

    QXmppShareThread *worker = new GetThread(this, requestIq);
    connect(worker, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(worker, SIGNAL(getFinished(QXmppShareGetIq,QXmppShareItem)),
            this, SIGNAL(getFinished(QXmppShareGetIq,QXmppShareItem)));
    addWorker(worker);
 }

/** Update database of available files.
 */
void QXmppShareDatabase::index()
{
    if (sharesPath.isEmpty() || (indexThread && indexThread->isRunning()))
        return;

    // start indexing
    if (!indexThread)
    {
        indexThread = new IndexThread(this);
        connect(indexThread, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
                this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
        connect(indexThread, SIGNAL(indexFinished(double,int,int,QStringList)),
                this, SLOT(slotIndexFinished(double,int,int,QStringList)));
        workers << indexThread;
    }
    indexThread->start(QThread::LowestPriority);
    emit indexStarted();
}

void QXmppShareDatabase::slotDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    indexTimer->setInterval(30 * 1000); // 30 seconds
    indexTimer->start();
}

void QXmppShareDatabase::slotIndexFinished(double elapsed, int updated, int removed, const QStringList &watchPaths)
{
    // update watched directories
    QStringList addPaths;
    QStringList removePaths = fsWatcher->directories();
    foreach (const QString &path, watchPaths)
        if (!removePaths.removeAll(path))
            addPaths << path;
    if (!removePaths.isEmpty())
        fsWatcher->removePaths(removePaths);
    if (!addPaths.isEmpty())
        fsWatcher->addPaths(addPaths);

    // schedule next run
    indexTimer->setInterval(INDEX_INTERVAL);
    indexTimer->start();

    emit indexFinished(elapsed, updated, removed);
}

void QXmppShareDatabase::slotWorkerDestroyed(QObject *object)
{
    workers.removeAll(static_cast<QXmppShareThread*>(object));
}

/** Handle a search request.
 */
void QXmppShareDatabase::search(const QXmppShareSearchIq &requestIq)
{
    if (sharesPath.isEmpty() || requestIq.type() != QXmppIq::Get)
        return;

    QXmppShareThread *worker = new SearchThread(this, requestIq);
    connect(worker, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(worker, SIGNAL(searchFinished(QXmppShareSearchIq)),
            this, SIGNAL(searchFinished(QXmppShareSearchIq)));
    addWorker(worker);
}

QXmppShareThread::QXmppShareThread(QXmppShareDatabase *database)
    : QThread(database), sharesDatabase(database), stopRequested(false)
{
}

void QXmppShareThread::stop()
{
    if (isRunning())
        stopRequested = true;
}

GetThread::GetThread(QXmppShareDatabase *database, const QXmppShareGetIq &request)
    : QXmppShareThread(database), requestIq(request)
{
}

void GetThread::run()
{
    Q_ASSERT(!sharesDatabase->directory().isEmpty());

    QXmppShareGetIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setFrom(requestIq.to());
    responseIq.setTo(requestIq.from());
    responseIq.setType(QXmppIq::Result);

    // look for file in database
    QXmppShareItem shareFile(QXmppShareItem::FileItem);
    File *cached = QDjangoQuerySet<File>().get(QDjangoWhere("path", QDjangoWhere::Equals, requestIq.node()));
    if (cached)
    {
        QFileInfo info(sharesDatabase->filePath(cached->path()));
        if (cached->update(info, true, true, &stopRequested))
        {
            // fill meta-data
            shareFile.setName(info.fileName());
            shareFile.setFileDate(cached->date());
            shareFile.setFileHash(cached->hash());
            shareFile.setFileSize(cached->size());
            shareFile.setPopularity(cached->popularity());

            QXmppShareLocation location(requestIq.to());
            location.setNode(cached->path());
            shareFile.setLocations(location);

            // FIXME : for some reason, random number generation fails
            //responseIq.setSid(generateStanzaHash());
            emit getFinished(responseIq, shareFile);
            delete cached;
            return;
        }
        delete cached;
    }

    warning("Could not find local file " + requestIq.node());
    QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::ItemNotFound);
    responseIq.setError(error);
    responseIq.setType(QXmppIq::Error);
    emit getFinished(responseIq, shareFile);
}

IndexThread::IndexThread(QXmppShareDatabase *database)
    : QXmppShareThread(database), scanUpdated(0)
{
}

void IndexThread::run()
{
    Q_ASSERT(!sharesDatabase->directory().isEmpty());
    QDir sharesDir(sharesDatabase->directory());
    debug("Scan started for " + sharesDir.path());

    // store existing entries
    QDjangoQuerySet<File> qs;
    for (int i = 0; i < qs.size(); i++)
    {
        File *cached = qs.at(i);
        scanOld.insert(cached->path(), cached);
    }

    // perform scan
    QTime t;
    t.start();
    scanDir(sharesDir);
    foreach (const QString &mappedDir, sharesDatabase->mappedDirectories())
        scanDir(mappedDir);

    if (stopRequested)
    {
        debug("Scan aborted for " + sharesDir.path());
    } else {
        // remove obsolete entries
        foreach (File *cached, scanOld.values())
            cached->remove();

        const double elapsed = double(t.elapsed()) / 1000.0;
        debug(QString("Scan completed in %1 s ( %2 updated, %3 removed )")
                   .arg(QString::number(elapsed))
                   .arg(scanUpdated)
                   .arg(scanOld.size()));
        emit indexFinished(elapsed, scanUpdated, scanOld.size(), watchDirs);
    }

    // cleanup
    foreach (File *cached, scanOld.values())
        delete cached;
    scanOld.clear();
    scanAdded = 0;
    scanUpdated = 0;
    stopRequested = false;
    watchDirs.clear();
}

void IndexThread::scanDir(const QDir &dir)
{
    QList<QFileInfo> subdirs;

    if (stopRequested)
        return;

    // check that the shares directory has not changed under our feet
    QString dirNode = sharesDatabase->fileNode(dir.path());
    if (dirNode.isEmpty() && dir.path() != sharesDatabase->directory())
        return;

    // if this is the shares directory or one of its immediate subdirectories,
    // add it to the list of directory to monitor
    if (!dirNode.count("/"))
        watchDirs << dir.path();

    if (!dirNode.isEmpty())
        dirNode += "/";

    // scan files first
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable))
    {
        if (stopRequested)
            return;

        if (info.isDir())
        {
            subdirs << info;
        } else {
            const QString fileNode = dirNode + info.fileName();

            File *cached = scanOld.take(fileNode);
            if (!cached)
            {
                cached = new File;
                cached->setPath(fileNode);
            }
            if (cached->update(info, false, false, &stopRequested))
                scanUpdated++;
            delete cached;
        }
    }

    // recurse into subdirectories
    foreach (const QFileInfo &info, subdirs)
    {
        if (stopRequested)
            return;

        scanDir(QDir(info.filePath()));
    }
}

SearchThread::SearchThread(QXmppShareDatabase *database, const QXmppShareSearchIq &request)
    : QXmppShareThread(database), requestIq(request)
{
}

void SearchThread::run()
{
    Q_ASSERT(!sharesDatabase->directory().isEmpty());

    QXmppShareSearchIq responseIq;
    responseIq.setId(requestIq.id());
    responseIq.setFrom(requestIq.to());
    responseIq.setTo(requestIq.from());
    responseIq.setTag(requestIq.tag());

    // determine query type
    QXmppShareLocation location;
    location.setJid(requestIq.to());
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
        warning("Received an invalid search: " + queryString);
        QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::BadRequest);
        responseIq.setError(error);
        responseIq.setType(QXmppIq::Error);
        emit searchFinished(responseIq);
        return;
    }

    // check the base path exists
    QFileInfo info(sharesDatabase->filePath(basePrefix));
    if (!basePrefix.isEmpty() && !info.exists())
    {
        warning("Base path no longer exists: " + basePrefix);

        // remove obsolete DB entries
        QDjangoQuerySet<File>().filter(QDjangoWhere("path", QDjangoWhere::StartsWith, basePrefix)).remove();

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
    QDjangoQuerySet<File> qs;
    if (!basePrefix.isEmpty())
        qs = qs.filter(QDjangoWhere("path", QDjangoWhere::StartsWith, basePrefix));
    if (!queryString.isEmpty())
        qs = qs.filter(QDjangoWhere("path", QDjangoWhere::Contains, queryString));
    qs = qs.orderBy(QStringList() << "path");

    int fileCount = 0;
    QMap<QString, QXmppShareItem*> subDirs;
    for (int hit = 0; hit < qs.size(); hit++)
    {
        File *cached = qs.at(hit);
        const QString path = cached->path();

        // find the depth at which we matched
        QStringList relativeBits = path.mid(basePrefix.size()).split("/");
        if (!relativeBits.size())
        {
            warning("Query returned an empty path");
            delete cached;
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
                const QString dirNode = basePrefix + dirPath;
                const QFileInfo dirInfo(sharesDatabase->filePath(dirNode));
                QXmppShareItem collection(QXmppShareItem::CollectionItem);
#if defined(Q_OS_SYMBIAN) || defined(Q_OS_WIN)
                if (driveRegex.exactMatch(dirInfo.filePath()))
                    collection.setName(dirInfo.filePath().left(1));
                else
                    collection.setName(dirInfo.fileName());
#else
                collection.setName(dirInfo.fileName());
#endif
                QXmppShareLocation location;
                location.setJid(requestIq.to());
                location.setNode(dirNode + "/");
                collection.setLocations(location);
                subDirs[dirPath] = parentCollection->appendChild(collection);
            }
            parentCollection = subDirs[dirPath];
        }

        if (!maxDepth || relativeBits.size() < maxDepth)
        {
            // update file info
            QFileInfo info(sharesDatabase->filePath(cached->path()));
            if (cached->update(info, requestIq.hash(), false, &stopRequested))
            {
                QXmppShareItem shareFile(QXmppShareItem::FileItem);
                shareFile.setName(info.fileName());
                shareFile.setFileDate(cached->date());
                shareFile.setFileHash(cached->hash());
                shareFile.setFileSize(cached->size());
                shareFile.setPopularity(cached->popularity());

                QXmppShareLocation location(requestIq.to());
                location.setNode(cached->path());
                shareFile.setLocations(location);

                fileCount++;
                parentCollection->appendChild(shareFile);
            }
        }
        delete cached;

        if (stopRequested || t.elapsed() > SEARCH_MAX_TIME)
        {
            warning(QString("Search truncated at %1 results after %2 s").arg(
                QString::number(fileCount),
                QString::number(double(t.elapsed()) / 1000.0)));
            break;
        }
    }
}

