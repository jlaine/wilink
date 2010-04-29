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

#include <QDir>
#include <QSqlDatabase>
#include <QThread>

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppShareIq.h"

class QTimer;

class ChatSharesDatabase : public QObject
{
    Q_OBJECT

public:
    struct Entry
    {
        QString path;
        QDateTime date;
        qint64 size;
        QByteArray hash;
    };

    ChatSharesDatabase(QObject *parent = 0);
    QSqlDatabase database() const;

    QString directory() const;

    QString filePath(const QString &node) const;
    QString fileNode(const QString &path) const;

    bool updateFile(QSqlDatabase sharesDb, ChatSharesDatabase::Entry &entry, bool updateHash);

    static ChatSharesDatabase *instance();

signals:
    void directoryChanged(const QString &path);
    void logMessage(QXmppLogger::MessageType type, const QString &msg);
    void getFinished(const QXmppShareGetIq &packet, const QXmppShareItem &fileInfo);
    void indexStarted();
    void indexFinished(double elapsed, int updated, int removed);
    void searchFinished(const QXmppShareSearchIq &packet);

public slots:
    void setDirectory(const QString &path);

    // threaded operations
    void get(const QXmppShareGetIq &requestIq);
    void index();
    void search(const QXmppShareSearchIq &requestIq);

private slots:
    void slotIndexFinished(double elapsed, int updated, int removed);

private:
    QTimer *indexTimer;
    QThread *indexThread;
    QSqlDatabase sharesDb;
    QDir sharesDir;

    static ChatSharesDatabase *globalInstance;
};

class ChatSharesThread : public QThread
{
    Q_OBJECT

public:
    ChatSharesThread(ChatSharesDatabase *database);

signals:
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

protected:
    ChatSharesDatabase *sharesDatabase;
    QSqlDatabase sharesDb;
};

class GetThread : public ChatSharesThread
{
    Q_OBJECT

public:
    GetThread(ChatSharesDatabase *database, const QXmppShareGetIq &request);
    void run();

signals:
    void getFinished(const QXmppShareGetIq &packet, const QXmppShareItem &fileInfo);

private:
    QXmppShareGetIq requestIq;
};

class IndexThread : public ChatSharesThread
{
    Q_OBJECT

public:
    IndexThread(ChatSharesDatabase *database);
    void run();

signals:
    void indexFinished(double elapsed, int updated, int removed);

private:
    void scanDir(const QDir &dir);

    QHash<QString, ChatSharesDatabase::Entry> scanOld;
    qint64 scanAdded;
    qint64 scanUpdated;
};

class SearchThread : public ChatSharesThread
{
    Q_OBJECT

public:
    SearchThread(ChatSharesDatabase *database, const QXmppShareSearchIq &request);
    void run();

signals:
    void searchFinished(const QXmppShareSearchIq &packet);

private:
    void search(QXmppShareItem &rootCollection, const QString &basePrefix, const QString &queryString, int maxDepth);

    QXmppShareSearchIq requestIq;
};
