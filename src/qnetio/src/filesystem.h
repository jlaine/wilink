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

#ifndef __QNETIO_FILESYSTEM_H__
#define __QNETIO_FILESYSTEM_H__

#include <QDateTime>
#include <QIODevice>
#include <QList>
#include <QUrl>
#include <QtPlugin>

class QNetworkAccessManager;
class QNetworkReply;

namespace QNetIO
{

class FileSystemJob;

/** @defgroup FileSystem FileSystem classes
 */

/** The FileSystemNetworkAccessManagerFactory class creates
 *  QNetworkAccessManager instances for use with a FileSystem.
 */
class FileSystemNetworkAccessManagerFactory
{
public:
    virtual ~FileSystemNetworkAccessManagerFactory() {};

    /** Creates a QNetworkAccessManager.
     *
     * @param parent
     */
    virtual QNetworkAccessManager* create(QObject* parent) = 0;
};

/** FileInfo stores information about a file.
 *
 * @ingroup FileSystem
 */
class FileInfo
{
public:
    FileInfo()
        : m_isDir(false)
        , m_size(0)
    {}

    bool isDir() const { return m_isDir; };
    void setDir(bool isDir) { m_isDir = isDir; };

    QDateTime lastModified () const { return m_lastModified; };
    void setLastModified(const QDateTime &lastModified) { m_lastModified = lastModified; };

    /** Returns the file's MIME type.
     */
    QString mimeType() const;

    QString name() const { return m_name; };
    void setName(const QString& name) { m_name = name; };

    qint64 size() const { return m_size; };
    void setSize(qint64 size) { m_size = size; };

    /** Returns the file's url.
     */
    QUrl url() const { return m_url; };

    /** Sets the file's url.
     *
     * @param url
     */
    void setUrl(const QUrl &url) { m_url = url; };

    /** Returns the URL of the file under the given URL.
     *
     * @param baseUrl
     * @param fileName
     */
    static QUrl fileUrl(const QUrl &baseUrl, const QString &fileName);

private:
    bool m_isDir;
    QDateTime m_lastModified;
    QString m_name;
    qint64 m_size;
    QUrl m_url;
};

typedef QList<FileInfo> FileInfoList;

/** FileSystem is the base class for all filesystems.
 *
 * It defines the operations all filesystems must support.
 *
 * All operations are asynchronous and return a FileSystemJob which
 * fires the FileSystemJob::finished() signal upon completion.
 *
 * @ingroup FileSystem
 */
class FileSystem : public QObject
{
    Q_OBJECT
    Q_ENUMS(ImageSize)

public:
    /** Describes an image size. */
    enum ImageSize {
        SmallSize,  /**< Small */
        MediumSize, /**< Medium */
        LargeSize,  /**< Large */
        FullSize,   /**< Full size */
    };

    static FileSystem *create(const QUrl &url, QObject *parent);

    static FileSystemNetworkAccessManagerFactory* networkAccessManagerFactory();
    static void setNetworkAccessManagerFactory(FileSystemNetworkAccessManagerFactory *factory);

    /** Constructs a new FileSystem.
     *
     * @param parent
     */
    FileSystem(QObject *parent = 0);

    virtual FileSystemJob* get(const QUrl &fileUrl, ImageSize type);
    virtual FileSystemJob* list(const QUrl &dirUrl, const QString &filter = QString());
    virtual FileSystemJob* mkdir(const QUrl &dirUrl);
    virtual FileSystemJob* open(const QUrl &url);
    virtual FileSystemJob* put(const QUrl &fileUrl, QIODevice *data);
    virtual FileSystemJob* remove(const QUrl &fileUrl);
    virtual FileSystemJob* rmdir(const QUrl &dirUrl);

signals:
    void directoryChanged(const QUrl &dirUrl);
    void fileChanged(const QUrl &url);

    /** This signal is emitted when a job is finished.
     */
    void jobFinished(FileSystemJob* job);

protected:
    /// \cond
    QNetworkAccessManager *createNetworkAccessManager(QObject *parent);
    /// \endcond
};

/** FileSystemJob is the base class for all FileSystem jobs.
 *
 * @ingroup FileSystem
 */
class FileSystemJob : public QIODevice
{
    Q_OBJECT
    Q_ENUMS(Error)
    Q_FLAGS(Operation Operations)

public:
    /** Describes a job error. */
    enum Error {
        NoError = 0,    /**< No error was encountered */
        UrlError,       /**< An invalid URL was given */
        UnknownError,   /**< An unknown error occured */
    };

    /** Describes a job operation. */
    enum Operation {
        None = 0,       /**< No operation */
        Get = 1,            /**< Get the contents of a file */
        List = 2,           /**< List the contents of a directory */
        Mkdir = 4,          /**< Create a directory */
        Open = 8,           /**< Open the file system */
        Put = 16,            /**< Set the contents of a file */
        Remove = 32,         /**< Remove a file */
        Rmdir = 64,          /**< Remove a directory */
    };
    Q_DECLARE_FLAGS(Operations, Operation)

    FileSystemJob(Operation operation, FileSystem* parent);

    Operation operation() const;
    QString operationName() const;

    Operations allowedOperations() const;
    Error error() const;
    QString errorString() const;
    FileInfoList results() const;

    /// \cond
    qint64 bytesAvailable() const;
    bool isSequential() const;
    qint64 size() const;
    void setAllowedOperations(Operations operations);
    void setData(QIODevice *data);
    void setError(Error error);
    void setErrorString(const QString &error);
    void setResults(const FileInfoList &results);
    void setNetworkReply(QNetworkReply *reply);
    /// \endcond

protected:
    qint64 readData(char* data, qint64 maxSize);
    qint64 writeData(const char* data, qint64 maxSize);

signals:
    /** This signal is emitted to indicate the progress of the download part
     *  of this job.
     */
    void downloadProgress(qint64 done, qint64 total);

    /** This signal is emitted when the job finished.
     */
    void finished();

    /** This signal is emitted to indicate the progress of the upload part
     *  of this job.
     */
    void uploadProgress(qint64 done, qint64 total);

public slots:
    /** Aborts the job.
     */
    virtual void abort();

    /// \cond
    void finishLater(Error error);
    /// \endcond

private slots:
    void _q_finished();
    void _q_finishLater();
    void _q_networkFinished();

protected:
    /// \cond
    virtual void networkFinished(QNetworkReply *reply);
    /// \endcond

private:
    Operations m_allowedOperations;
    QIODevice *m_data;
    Error m_error;
    QString m_errorString;
    FileSystem* m_fileSystem;
    Error m_finishError;
    Operation m_operation;
    QNetworkReply *m_networkReply;
    FileInfoList m_results;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FileSystemJob::Operations)

class FileSystemPluginInterface
{
public:
    virtual FileSystem *create(const QUrl &url, QObject *parent) = 0;
};

}

Q_DECLARE_INTERFACE(QNetIO::FileSystemPluginInterface, "eu.bolloretelecom.FileSystemPlugin/1.0")

namespace QNetIO {

class FileSystemPlugin : public QObject, public FileSystemPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QNetIO::FileSystemPluginInterface)
};

}

#endif

