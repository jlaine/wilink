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
#include <QTreeView>

#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppShareIq.h"

#include "chat_panel.h"

class ChatClient;
class ChatSharesDatabase;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTimer;
class QXmppPacket;
class QXmppPresence;

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
    QXmppShareItem *findItemByData(QXmppShareItem::Type type, int role, const QVariant &data, QXmppShareItem *parent = 0);
    void pruneEmptyChildren(QXmppShareItem *parent = 0);
    void removeItem(QXmppShareItem *item);
    void setShareServer(const QString &server);

signals:
    void itemReceived(const QModelIndex &index);

private:
    QXmppShareItem *findItemByMirrors(const QXmppShareMirrorList &mirrors, QXmppShareItem *parent);

public slots:
    void shareSearchIqReceived(const QXmppShareSearchIq &shareIq);

private:
    QXmppShareItem *rootItem;
    QString shareServer;

    QIcon collectionIcon;
    QIcon fileIcon;
    QIcon peerIcon;
};

class ChatSharesView : public QTreeView
{
    Q_OBJECT

public:
    ChatSharesView(QWidget *parent = 0);

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
    ChatSharesModel *downloadQueue();
    void setClient(ChatClient *client);

signals:
    void fileExpected(const QString &sid, const QString path);
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void findRemoteFiles();
    void itemAction();
    void itemContextMenu(const QModelIndex &index, const QPoint &globalPos);
    void itemDoubleClicked(const QModelIndex &index);
    void itemReceived(const QModelIndex &index);
    void presenceReceived(const QXmppPresence &presence);
    void processDownloadQueue();
    void registerWithServer();
    void queryStringChanged();
    void siPubIqReceived(const QXmppSiPubIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &share);
    void shareServerFound(const QString &server);
    void searchFinished(const QXmppShareSearchIq &share);

private:
    QString shareServer;

    ChatClient *client;
    ChatClient *baseClient;
    ChatSharesDatabase *db;
    ChatSharesModel *model;
    ChatSharesModel *queueModel;

    QLineEdit *lineEdit;
    ChatSharesView *treeWidget;
    QTimer *registerTimer;
};

#endif
