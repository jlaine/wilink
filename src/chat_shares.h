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

#ifndef __WILINK_CHAT_SHARES_H__
#define __WILINK_CHAT_SHARES_H__

#include <QIcon>
#include <QStyledItemDelegate>
#include <QTreeView>

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"

#include "chat_panel.h"

class ChatClient;
class ChatRosterModel;
class ChatSharesDatabase;
class ChatTransfers;
class ChatTransfersView;
class QDialogButtonBox;
class QLineEdit;
class QListWidget;
class QStatusBar;
class QTabWidget;
class QTimer;
class QXmppPacket;
class QXmppPresence;

class ChatSharesDelegate : public QStyledItemDelegate
{
public:
    ChatSharesDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class ChatSharesModelQuery
{
public:
    enum Operation
    {
        None,
        Equals,
        NotEquals,
        // Contains,
    };

    ChatSharesModelQuery();
    ChatSharesModelQuery(int role, ChatSharesModelQuery::Operation operation, QVariant data);

    bool match(QXmppShareItem *item) const;

    ChatSharesModelQuery operator&&(const ChatSharesModelQuery &other) const;
    ChatSharesModelQuery operator||(const ChatSharesModelQuery &other) const;

private:
    enum Combine
    {
        NoCombine,
        AndCombine,
        OrCombine,
    };

    int m_role;
    ChatSharesModelQuery::Operation m_operation;
    QVariant m_data;

    QList<ChatSharesModelQuery> m_children;
    ChatSharesModelQuery::Combine m_combine;
};

/** Model representing a tree of share items (collections and files).
 */
class ChatSharesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Recurse
    {
        DontRecurse,
        PreRecurse,
        PostRecurse,
    };

    class QueryOptions
    {
    public:
        QueryOptions(Recurse recurse = PreRecurse);
        Recurse recurse;
    };

    ChatSharesModel(QObject *parent = 0);
    ~ChatSharesModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QXmppShareItem *addItem(const QXmppShareItem &item);
    QList<QXmppShareItem*> filter(const ChatSharesModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0, int limit = 0);
    QXmppShareItem *get(const ChatSharesModelQuery &query, const QueryOptions &options = QueryOptions(), QXmppShareItem *parent = 0);
    void refreshItem(QXmppShareItem *item);
    void removeItem(QXmppShareItem *item);
    QModelIndex updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem);

private:
    QXmppShareItem *rootItem;

    // cached icons, to avoid reloading them whenever an item is added
    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

/** View for displaying a tree of share items.
 */
class ChatSharesView : public QTreeView
{
    Q_OBJECT

public:
    ChatSharesView(QWidget *parent = 0);
    void setModel(QAbstractItemModel *model);

signals:
    void contextMenu(const QModelIndex &index, const QPoint &globalPos);
    void expandRequested(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *e);
};

/** The shares panel.
 */
class ChatShares : public ChatPanel
{
    Q_OBJECT

public:
    ChatShares(ChatClient *client, QWidget *parent = 0);
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
    void shareGetIqReceived(const QXmppShareGetIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &searchIq);
    void shareServerFound(const QString &server);
    void getFinished(const QXmppShareGetIq &reponseIq, const QXmppShareItem &fileInfo);
    void indexStarted();
    void indexFinished(double elapsed, int updated, int removed);
    void searchFinished(const QXmppShareSearchIq &responseIq);
    void tabChanged(int index);

    void changeDirectory();
    void directoryChanged(const QString &path);

private:
    void queueItem(QXmppShareItem *item);

private:
    QString shareServer;

    ChatClient *client;
    ChatClient *baseClient;
    ChatRosterModel *rosterModel;
    ChatSharesDatabase *db;
    ChatSharesModel *queueModel;
    QMap<QString, QWidget*> searches;

    QLineEdit *lineEdit;
    QTabWidget *tabWidget;

    QLabel *sharesLabel;
    ChatSharesView *sharesView;
    QWidget *sharesWidget;

    QLabel *searchLabel;
    ChatSharesView *searchView;
    QWidget *searchWidget;

    QLabel *downloadsLabel;
    ChatSharesView *downloadsView;
    QWidget *downloadsWidget;
    QList<QXmppTransferJob*> downloadJobs;

    QLabel *uploadsLabel;
    ChatTransfersView *uploadsView;
    QWidget *uploadsWidget;

    QPushButton *downloadButton;
    QPushButton *directoryButton;
    QPushButton *indexButton;
    QPushButton *removeButton;
    QStatusBar *statusBar;
    QTimer *registerTimer;
};

#endif
