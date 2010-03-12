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

#ifndef __WDESKTOP_CHAT_SHARES_H__
#define __WDESKTOP_CHAT_SHARES_H__

#include <QDir>
#include <QWidget>
#include <QSqlDatabase>
#include <QThread>

class ChatClient;
class QDir;
class QLineEdit;
class QListWidget;
class QTableWidget;
class QTimer;
class QXmppPacket;
class QXmppShareIq;

class ChatSharesDatabase : public QObject
{
    Q_OBJECT

public:
    ChatSharesDatabase(const QString &path, QObject *parent = 0);
    void search(const QXmppShareIq &requestIq);

signals:
    void searchFinished(const QXmppPacket &packet);

private:
    class IndexThread : public QThread
    {
    public:
        IndexThread(const QSqlDatabase &database, const QDir &dir, QObject *parent = 0);
        void run();

    private:
        void scanDir(const QDir &dir);
        qint64 scanCount;
        QSqlDatabase sharesDb;
        QDir sharesDir;
    };

    QSqlDatabase sharesDb;
    QDir sharesDir;
};

class ChatShares : public QWidget
{
    Q_OBJECT

public:
    ChatShares(ChatClient *client, QWidget *parent = 0);
    void setShareServer(const QString &server);

private slots:
    void findRemoteFiles();
    void registerWithServer();
    void shareIqReceived(const QXmppShareIq &share);

private:
    QString shareServer;
    QDir sharesDir;

    ChatClient *client;
    ChatSharesDatabase *db;
    QLineEdit *lineEdit;
    QTableWidget *tableWidget;
    QTimer *registerTimer;
};

#endif
