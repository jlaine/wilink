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

#ifndef __WILINK_SHARES_H__
#define __WILINK_SHARES_H__

#include <QTextDocument>

#include "QXmppLogger.h"
#include "QXmppShareIq.h"
#include "QXmppTransferManager.h"

#include "chat_panel.h"

class Chat;
class ChatRosterModel;
class ShareModel;
class SharesView;
class QLabel;
class QModelIndex;
class QPushButton;
class QStatusBar;
class QXmppPresence;
class QXmppShareDatabase;

/** The shares panel.
 */
class SharePanel : public ChatPanel
{
    Q_OBJECT

public:
    SharePanel(Chat *chat, QXmppShareDatabase *sharesDb, QWidget *parent = 0);
    void setClient(QXmppClient *client);

signals:
    void findFinished(bool found);
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void disconnected();
    void getFailed(const QString &packetId);
    void transferAbort(QXmppShareItem *item);
    void transferDestroyed(QObject *obj);
    void transferDoubleClicked(const QModelIndex &index);
    void transferFinished(QXmppTransferJob *job);
    void transferProgress(qint64, qint64);
    void transferRemoved();
    void transferStarted(QXmppTransferJob *job);
    void find(const QString &needle, QTextDocument::FindFlags flags, bool changed);
    void downloadItem();
    void itemContextMenu(const QModelIndex &index, const QPoint &globalPos);
    void itemDoubleClicked(const QModelIndex &index);
    void itemExpandRequested(const QModelIndex &index);
    void presenceReceived(const QXmppPresence &presence);
    void processDownloadQueue();
    void shareSearchIqReceived(const QXmppShareSearchIq &searchIq);
    void shareServerFound(const QString &server);
    void indexStarted();
    void indexFinished(double elapsed, int updated, int removed);
    void directoryChanged(const QString &path);

private:
    void queueItem(QXmppShareItem *item);

private:
    QString shareServer;

    QAction *action;
    Chat *chatWindow;
    QXmppClient *client;
    QXmppShareDatabase *db;
    ShareModel *queueModel;
    QMap<QString, QWidget*> searches;

    SharesView *sharesView;
    QString sharesFilter;

    QLabel *downloadsHelp;
    SharesView *downloadsView;
    QList<QXmppTransferJob*> downloadJobs;

    QPushButton *downloadButton;
    QPushButton *removeButton;
    QStatusBar *statusBar;
};

#endif
