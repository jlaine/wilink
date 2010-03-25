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

#include <QIcon>
#include <QStyledItemDelegate>
#include <QTreeView>

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppTransferManager.h"

#include "chat_panel.h"

class ChatClient;
class ChatSharesDatabase;
class ChatTransfers;
class ChatTransfersView;
class QLineEdit;
class QListWidget;
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

class ChatSharesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    ChatSharesModel(QObject *parent = 0);
    ~ChatSharesModel();
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void addItem(const QXmppShareItem &item);
    QModelIndex mergeItem(const QXmppShareItem &item);
    QXmppShareItem *findItemByData(QXmppShareItem::Type type, int role, const QVariant &data, QXmppShareItem *parent = 0);
    void pruneEmptyChildren(QXmppShareItem *parent = 0);
    void removeItem(QXmppShareItem *item);

    void setProgress(QXmppShareItem *item, qint64 done, qint64 total);

private:
    QXmppShareItem *findItemByLocations(const QXmppShareLocationList &mirrors, QXmppShareItem *parent, bool recurse = true);
    QModelIndex updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem);

private:
    QXmppShareItem *rootItem;

    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

class ChatSharesView : public QTreeView
{
    Q_OBJECT

public:
    ChatSharesView(QWidget *parent = 0);
    void setModel(QAbstractItemModel *model);

signals:
    void contextMenu(const QModelIndex &index, const QPoint &globalPos);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void resizeEvent(QResizeEvent *e);
};

class ChatShares : public ChatPanel
{
    Q_OBJECT

public:
    ChatShares(ChatClient *client, QWidget *parent = 0);
    void setClient(ChatClient *client);
    void setTransfers(ChatTransfers *transfers);

signals:
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void disconnected();
    void transferReceived(QXmppTransferJob *job);
    void transferProgress(qint64, qint64);
    void transferStateChanged(QXmppTransferJob::State state);
    void findRemoteFiles();
    void itemAction();
    void itemContextMenu(const QModelIndex &index, const QPoint &globalPos);
    void itemDoubleClicked(const QModelIndex &index);
    void presenceReceived(const QXmppPresence &presence);
    void processDownloadQueue();
    void registerWithServer();
    void queryStringChanged();
    void shareGetIqReceived(const QXmppShareGetIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &share);
    void shareServerFound(const QString &server);
    void searchFinished(const QXmppShareSearchIq &share);

private:
    QString shareServer;

    ChatClient *client;
    ChatClient *baseClient;
    ChatSharesDatabase *db;
    ChatSharesModel *queueModel;
    ChatTransfers *chatTransfers;
    QMap<QString, QWidget*> searches;

    QLineEdit *lineEdit;
    QTabWidget *tabWidget;
    ChatSharesView *sharesView;
    ChatSharesView *searchView;
    ChatSharesView *downloadsView;
    ChatTransfersView *uploadsView;
    QTimer *registerTimer;
};

#endif
