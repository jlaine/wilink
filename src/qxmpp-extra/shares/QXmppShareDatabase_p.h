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

#include <QDir>
#include <QThread>

#include "QXmppLogger.h"
#include "QXmppShareIq.h"

class File;
class QXmppShareDatabase;

class QXmppShareThread : public QThread
{
    Q_OBJECT

public:
    QXmppShareThread(QXmppShareDatabase *database);
    void stop();

signals:
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

protected:
    void debug(const QString &message)
    {
        emit logMessage(QXmppLogger::DebugMessage, qxmpp_loggable_trace(message));
    }

    void warning(const QString &message)
    {
        emit logMessage(QXmppLogger::WarningMessage, qxmpp_loggable_trace(message));
    }

    QXmppShareDatabase *sharesDatabase;
    bool stopRequested;
};

class GetThread : public QXmppShareThread
{
    Q_OBJECT

public:
    GetThread(QXmppShareDatabase *database, const QXmppShareGetIq &request);
    void run();

signals:
    void getFinished(const QXmppShareGetIq &packet, const QXmppShareItem &fileInfo);

private:
    QXmppShareGetIq requestIq;
};

class IndexThread : public QXmppShareThread
{
    Q_OBJECT

public:
    IndexThread(QXmppShareDatabase *database);
    void run();

signals:
    void indexFinished(double elapsed, int updated, int removed, const QStringList &watchDirs);

private:
    void scanDir(const QDir &dir);

    QHash<QString, File*> scanOld;
    qint64 scanAdded;
    qint64 scanUpdated;
    QStringList watchDirs;
};

class SearchThread : public QXmppShareThread
{
    Q_OBJECT

public:
    SearchThread(QXmppShareDatabase *database, const QXmppShareSearchIq &request);
    void run();

signals:
    void searchFinished(const QXmppShareSearchIq &packet);

private:
    void search(QXmppShareItem &rootCollection, const QString &basePrefix, const QString &queryString, int maxDepth);

    QXmppShareSearchIq requestIq;
};
