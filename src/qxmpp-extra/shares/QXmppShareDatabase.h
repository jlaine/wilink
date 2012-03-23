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

#ifndef QXMPPSHAREDATABASE_H
#define QXMPPSHAREDATABASE_H

#include <QDateTime>
#include <QDir>
#include <QThread>

#include "QXmppLogger.h"
#include "QXmppShareIq.h"

#include "QDjangoModel.h"

class QFileSystemWatcher;
class QTimer;

class QXmppShareThread;
class IndexThread;

class QXmppShareDatabase : public QXmppLoggable
{
    Q_OBJECT

public:
    QXmppShareDatabase(QObject *parent = 0);
    ~QXmppShareDatabase();

    QString directory() const;
    QStringList mappedDirectories() const;

    QString filePath(const QString &node) const;
    QString fileNode(const QString &path) const;

signals:
    void directoryChanged(const QString &path);
    void getFinished(const QXmppShareGetIq &packet, const QXmppShareItem &fileInfo);
    void indexStarted();
    void indexFinished(double elapsed, int updated, int removed);
    void searchFinished(const QXmppShareSearchIq &packet);

public slots:
    void setDirectory(const QString &path);
    void setMappedDirectories(const QStringList &paths);

    // threaded operations
    void get(const QXmppShareGetIq &requestIq);
    void index();
    void search(const QXmppShareSearchIq &requestIq);

private slots:
    void slotDirectoryChanged(const QString &path);
    void slotIndexFinished(double elapsed, int updated, int removed, const QStringList &watchDirs);
    void slotWorkerDestroyed(QObject *object);

private:
    void addWorker(QXmppShareThread* worker);

    QTimer *indexTimer;
    IndexThread *indexThread;
    QFileSystemWatcher *fsWatcher;
    QString sharesPath;
    QMap<QString,QString> mappedPaths;
    QList<QXmppShareThread*> workers;
};

class File : public QDjangoModel
{
    Q_OBJECT
    Q_PROPERTY(QDateTime date READ date WRITE setDate)
    Q_PROPERTY(QByteArray hash READ hash WRITE setHash)
    Q_PROPERTY(QString path READ path WRITE setPath)
    Q_PROPERTY(qint64 popularity READ popularity WRITE setPopularity)
    Q_PROPERTY(qint64 size READ size WRITE setSize)

    Q_CLASSINFO("path", "primary_key=1")
    Q_CLASSINFO("hash", "max_length=16")

public:
    File(QObject *parent = 0);

    QDateTime date() const;
    void setDate(const QDateTime &date);

    QByteArray hash() const;
    void setHash(const QByteArray &hash);

    QString path() const;
    void setPath(const QString &path);

    qint64 popularity() const;
    void setPopularity(qint64 popularity);

    qint64 size() const;
    void setSize(qint64 size);

    bool update(const QFileInfo &info, bool updateHash, bool increasePopularity, bool *stopFlag);

private:
    QDateTime m_date;
    QByteArray m_hash;
    QString m_path;
    qint64 m_popularity;
    qint64 m_size;
};

class Schema : public QDjangoModel
{
    Q_OBJECT
    Q_PROPERTY(QString model READ model WRITE setModel)
    Q_PROPERTY(int version READ version WRITE setVersion)

    Q_CLASSINFO("model", "primary_key=1")

public:
    Schema(QObject *parent = 0);

    QString model() const;
    void setModel(const QString &model);

    int version() const;
    void setVersion(int version);

private:
    QString m_model;
    int m_version;
};

#endif
