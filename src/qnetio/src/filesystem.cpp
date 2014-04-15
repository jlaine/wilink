/*
 * QNetIO
 * Copyright (C) 2008-2012 Bolloré telecom
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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPluginLoader>
#include <QTimer>

#include "filesystem.h"
#include "mime.h"
#include "wallet.h"

Q_IMPORT_PLUGIN(local_filesystem)
Q_IMPORT_PLUGIN(ftp_filesystem)
Q_IMPORT_PLUGIN(http_filesystem)
#ifdef USE_GPHOTO
Q_IMPORT_PLUGIN(gphoto_filesystem)
#endif
#ifdef USE_FACEBOOK
Q_IMPORT_PLUGIN(facebook_filesystem)
#endif
Q_IMPORT_PLUGIN(picasa_filesystem)
Q_IMPORT_PLUGIN(wifirst_filesystem)

using namespace QNetIO;

static FileSystemNetworkAccessManagerFactory *networkFactory = 0;

QUrl FileInfo::fileUrl(const QUrl &baseUrl, const QString &fileName)
{
    QUrl url(baseUrl);
    if (url.path().endsWith("/"))
        url.setPath(url.path() + fileName);
    else
        url.setPath(url.path() + "/" + fileName);
    return url;
}

QString FileInfo::mimeType() const
{
    if (isDir()) {
        return "application/x-directory";
    } else {
        return MimeTypes().guessType(m_url.path());
    }
}

FileSystem::FileSystem(QObject *parent)
    : QObject(parent)
{
}

/** Retrieve data from a remote file.
 *
 * @param fileUrl
 * @param type
 */
FileSystemJob* FileSystem::get(const QUrl &fileUrl, ImageSize type)
{
    Q_UNUSED(fileUrl);
    Q_UNUSED(type);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Get, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Retrieve the list of files and subdirectories in the
 *  given directory.
 *
 * @param dirUrl
 * @param filter
 */
FileSystemJob* FileSystem::list(const QUrl &dirUrl, const QString &filter)
{
    Q_UNUSED(dirUrl);
    Q_UNUSED(filter);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::List, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Create a directory.
 *
 * @param dirUrl
 */
FileSystemJob* FileSystem::mkdir(const QUrl &dirUrl)
{
    Q_UNUSED(dirUrl);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Mkdir, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Open the given URL.
 */
FileSystemJob* FileSystem::open(const QUrl &url)
{
    Q_UNUSED(url);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Open, this);
    job->finishLater(FileSystemJob::NoError);
    return job;
}

/** Send data to a remote file.
 *
 * \c data must be open for reading.
 *
 * @param fileUrl
 * @param data
 */
FileSystemJob* FileSystem::put(const QUrl &fileUrl, QIODevice *data)
{
    Q_UNUSED(fileUrl);
    Q_UNUSED(data);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Put, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Delete a remote file.
 *
 * @param fileUrl
 */
FileSystemJob* FileSystem::remove(const QUrl &fileUrl)
{
    Q_UNUSED(fileUrl);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Remove, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Delete a remote folder.
 *
 * @param fileUrl
 */
FileSystemJob* FileSystem::rmdir(const QUrl &dirUrl)
{
    Q_UNUSED(dirUrl);

    FileSystemJob *job = new FileSystemJob(FileSystemJob::Rmdir, this);
    job->finishLater(FileSystemJob::UnknownError);
    return job;
}

/** Creates a FileSystem for the given URL.
 *
 * @param url
 * @param parent
 */
FileSystem *FileSystem::create(const QUrl &url, QObject *parent)
{
    foreach (QObject *obj, pluginInstances("filesystem")) {
        FileSystemPlugin *plugin = qobject_cast<FileSystemPlugin *>(obj);
        if (plugin) {
            FileSystem *fs = plugin->create(url, parent);
            if (fs)
                return fs;
        }
    }
    return 0;
}

QNetworkAccessManager *FileSystem::createNetworkAccessManager(QObject *parent)
{
    bool check;
    Q_UNUSED(check);

    QNetworkAccessManager *network;
    if (networkFactory)
        network = networkFactory->create(parent);
    else
        network = new QNetworkAccessManager(parent);

    check = connect(network, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                    Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    Q_ASSERT(network);

    return network;
}

/** Returns the factory to use for creating QNetworkAccessManager(s).
 */
FileSystemNetworkAccessManagerFactory* FileSystem::networkAccessManagerFactory()
{
    return networkFactory;
}

/** Sets the factory to use for creating QNetworkAccessManager(s).
 *
 * @param factory
 */
void FileSystem::setNetworkAccessManagerFactory(FileSystemNetworkAccessManagerFactory *factory)
{
    networkFactory = factory;
}

/** Creates a new FileSystemJob.
 */
FileSystemJob::FileSystemJob(FileSystemJob::Operation operation, FileSystem *parent)
    : QIODevice(parent),
    m_allowedOperations(FileSystemJob::None),
    m_data(0),
    m_error(FileSystemJob::NoError),
    m_fileSystem(parent),
    m_operation(operation),
    m_networkReply(0)
{
    connect(this, SIGNAL(finished()),
            this, SLOT(_q_finished()));
}

/** Aborts the job.
 */
void FileSystemJob::abort()
{
    if (m_networkReply)
        m_networkReply->abort();
}

FileSystemJob::Operations FileSystemJob::allowedOperations() const
{
    return m_allowedOperations;
}

void FileSystemJob::setAllowedOperations(FileSystemJob::Operations operations)
{
    m_allowedOperations = operations;
}

void FileSystemJob::setData(QIODevice *data)
{
    bool check;
    Q_UNUSED(check);

    if (data != m_data) {
        m_data = data;

        check = connect(data, SIGNAL(readyRead()),
                        this, SIGNAL(readyRead()));
        Q_ASSERT(check);

        // FIXME: is this the right time?
        setOpenMode(data->openMode());
    }
}

/** Returns the error which was encountered while processing
 *  the job.
 */
FileSystemJob::Error FileSystemJob::error() const
{
    return m_error;
}

void FileSystemJob::setError(FileSystemJob::Error error)
{
    m_error = error;
}

/** Returns the error string if an error was encountered.
 *
 * @sa error()
 */
QString FileSystemJob::errorString() const
{
    return m_errorString;
}

void FileSystemJob::setErrorString(const QString &error)
{
    m_errorString = error;
}

void FileSystemJob::finishLater(FileSystemJob::Error error)
{
    m_finishError = error;
    QMetaObject::invokeMethod(this, "_q_finishLater", Qt::QueuedConnection);
}

/** Returns the operation that was posted for this reply.
 */
FileSystemJob::Operation FileSystemJob::operation() const
{
    return m_operation;
}

/** Returns a string representation of the operation that was posted
 *  for this reply.
 */
QString FileSystemJob::operationName() const
{
    switch (m_operation)
    {
        case None: return "none";
        case Get: return "get";
        case Open: return "open";
        case List: return "list";
        case Mkdir: return "mkdir";
        case Put: return "put";
        case Remove: return "remove";
        case Rmdir: return "rmdir";
    }
    return "unknown";
}

void FileSystemJob::networkFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        setError(UnknownError);
        setErrorString(reply->errorString());
    }

    emit finished();
}

void FileSystemJob::setNetworkReply(QNetworkReply *reply)
{
    bool check;
    Q_UNUSED(check);

#ifdef DEBUG_FILESYSTEM
    QByteArray verb;
    switch (reply->operation())
    {
    case QNetworkAccessManager::HeadOperation:
        verb = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        verb = "GET";
        break;
    case QNetworkAccessManager::PutOperation:
        verb = "PUT";
        break;
    case QNetworkAccessManager::PostOperation:
        verb = "POST";
        break;
    case QNetworkAccessManager::DeleteOperation:
        verb = "DELETE";
        break;
    default:
       verb = reply->request().attribute(QNetworkRequest::CustomVerbAttribute).toByteArray();
    }
    qDebug("%s %s", verb.constData(), qPrintable(reply->request().url().toString()));
    foreach (const QByteArray &header, reply->request().rawHeaderList()) {
        qDebug("%s: %s", header.constData(), reply->request().rawHeader(header).constData());
    }
#endif

    m_networkReply = reply;
    m_networkReply->setParent(this);

    check = connect(m_networkReply, SIGNAL(downloadProgress(qint64,qint64)),
                    this, SIGNAL(downloadProgress(qint64,qint64)));
    Q_ASSERT(check);

    check = connect(m_networkReply, SIGNAL(finished()),
                    this, SLOT(_q_networkFinished()));
    Q_ASSERT(check);

    check = connect(m_networkReply, SIGNAL(uploadProgress(qint64,qint64)),
                    this, SIGNAL(uploadProgress(qint64,qint64)));
    Q_ASSERT(check);
}

bool FileSystemJob::isSequential() const
{
    if (m_data)
        return m_data->isSequential();
    else
        return true;
}

qint64 FileSystemJob::bytesAvailable() const
{
    if (m_data)
        return QIODevice::bytesAvailable() + m_data->bytesAvailable();
    else
        return 0;
}

qint64 FileSystemJob::readData(char* data, qint64 maxSize)
{
    if (m_data)
        return m_data->read(data, maxSize);
    else
        return 0;
}

qint64 FileSystemJob::size() const
{
    if (m_data)
        return m_data->size();
    else
        return 0;
}

qint64 FileSystemJob::writeData(const char* data, qint64 maxSize)
{
    Q_UNUSED(data);
    Q_UNUSED(maxSize);
    return -1;
}

/** Returns the list results.
 */
FileInfoList FileSystemJob::results() const
{
    return m_results;
}

void FileSystemJob::setResults(const FileInfoList &results)
{
    m_results = results;
}

void FileSystemJob::_q_finished()
{
    QMetaObject::invokeMethod(m_fileSystem, "jobFinished", Q_ARG(FileSystemJob*, this));
}

void FileSystemJob::_q_finishLater()
{
    setError(m_finishError);
    emit finished();
}

void FileSystemJob::_q_networkFinished()
{
    Q_ASSERT(m_networkReply);
    networkFinished(m_networkReply);
}

