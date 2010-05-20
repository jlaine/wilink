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

#ifndef __WILINK_SHARES_H__
#define __WILINK_SHARES_H__

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"

#include "chat_panel.h"

class Chat;
class ChatClient;
class ChatRosterModel;
class ChatSharesDatabase;
class ChatSharesModel;
class ChatSharesTab;
class ChatSharesView;
class ChatTransfers;
class ChatTransfersView;
class QLineEdit;
class QModelIndex;
class QStatusBar;
class QTabWidget;
class QTimer;
class QXmppPacket;
class QXmppPresence;

/** The shares panel.
 */
class ChatShares : public ChatPanel
{
    Q_OBJECT

public:
    ChatShares(Chat *chat, ChatSharesDatabase *sharesDb, QWidget *parent = 0);
    void setClient(ChatClient *client);
    void setRoster(ChatRosterModel *model);
    void setTransfers(ChatTransfers *transfers);

signals:
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void disconnected();
    void transferAbort(QXmppShareItem *item);
    void transferDestroyed(QObject *obj);
    void transferDoubleClicked(const QModelIndex &index);
    void transferProgress(qint64, qint64);
    void transferReceived(QXmppTransferJob *job);
    void transferRemoved();
    void transferStateChanged(QXmppTransferJob::State state);
    void findRemoteFiles();
    void downloadItem();
    void itemContextMenu(const QModelIndex &index, const QPoint &globalPos);
    void itemDoubleClicked(const QModelIndex &index);
    void itemExpandRequested(const QModelIndex &index);
    void presenceReceived(const QXmppPresence &presence);
    void processDownloadQueue();
    void registerWithServer();
    void queryStringChanged();
    void shareFolder();
    void shareGetIqReceived(const QXmppShareGetIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &searchIq);
    void shareServerFound(const QString &server);
    void getFinished(const QXmppShareGetIq &reponseIq, const QXmppShareItem &fileInfo);
    void indexStarted();
    void indexFinished(double elapsed, int updated, int removed);
    void searchFinished(const QXmppShareSearchIq &responseIq);
    void tabChanged(int index);
    void directoryChanged(const QString &path);

private:
    void queueItem(QXmppShareItem *item);

private:
    QString shareServer;

    ChatClient *client;
    Chat *chatWindow;
    ChatRosterModel *rosterModel;
    ChatSharesDatabase *db;
    ChatSharesModel *queueModel;
    QMap<QString, QWidget*> searches;

    QLineEdit *lineEdit;
    QTabWidget *tabWidget;

    ChatSharesView *sharesView;
    ChatSharesTab *sharesWidget;

    ChatSharesView *searchView;
    ChatSharesTab *searchWidget;

    ChatSharesView *downloadsView;
    ChatSharesTab *downloadsWidget;
    QList<QXmppTransferJob*> downloadJobs;

    ChatTransfersView *uploadsView;
    ChatSharesTab *uploadsWidget;

    QPushButton *downloadButton;
    QPushButton *indexButton;
    QPushButton *removeButton;
    QStatusBar *statusBar;
    QTimer *registerTimer;

    QAction *menuAction;
};

class ChatSharesTab : public QWidget
{
public:
    ChatSharesTab(QWidget *parent = 0);
    void addWidget(QWidget *widget);
    void setText(const QString &text);

private:
    QLabel *label;
};

#endif
