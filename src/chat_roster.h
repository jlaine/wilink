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

#ifndef __WILINK_CHAT_ROSTER_H__
#define __WILINK_CHAT_ROSTER_H__

#include <QAbstractItemModel>
#include <QTreeView>

#include "QXmppVCardIq.h"

#include "chat_roster_item.h"

class QContextMenuEvent;
class QMenu;
class QSortFilterProxyModel;
class QXmppClient;
class QXmppDiscoveryIq;
class QXmppPresence;
class QXmppVCardManager;

class ChatRosterModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole,
        TypeRole,
        MessagesRole,
        AvatarRole,
        FlagsRole,
        StatusRole,
        UrlRole,
        NicknameRole,
    };

    enum Feature {
        ChatStatesFeature = 1,
        FileTransferFeature = 2,
        VersionFeature = 4,
        VoiceFeature = 8,
    };

    enum Flag {
        OptionsFlag = 1,
        MembersFlag = 2,
        KickFlag = 4,
        SubjectFlag = 8,
    };

    ChatRosterModel(QXmppClient *client, QObject *parent = 0);
    ~ChatRosterModel();

    // QAbstractItemModel interface
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
    QModelIndex parent(const QModelIndex & index) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QPixmap contactAvatar(const QString &bareJid) const;
    QStringList contactFeaturing(const QString &bareJid, ChatRosterModel::Feature) const;
    QString contactExtra(const QString &bareJid) const;
    QString contactName(const QString &bareJid) const;
    bool isOwnNameReceived() const;
    QString ownName() const;

    QModelIndex addItem(ChatRosterItem::Type type, const QString &id, const QString &name = QString(), const QIcon &icon = QIcon(), const QModelIndex &parent = QModelIndex());
    QModelIndex findItem(const QString &bareJid) const;
    void removeItem(const QString &bareJid);
    QModelIndex reparentItem(const QModelIndex &index, const QModelIndex &parent);

    void addPendingMessage(const QString &bareJid);
    void clearPendingMessages(const QString &bareJid);

signals:
    void ownNameReceived();
    void pendingMessages(int messages);
    void rosterReady();

public slots:
    void connected();
    void disconnected();

protected slots:
    void discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void presenceChanged(const QString& bareJid, const QString& resource);
    void presenceReceived(const QXmppPresence &presence);
    void rosterChanged(const QString &jid);
    void rosterReceived();
    void vCardReceived(const QXmppVCardIq&);

private:
    void countPendingMessages();

    QXmppClient *client;
    ChatRosterItem *rootItem;
    QString nickName;
    bool nickNameReceived;
    QMap<QString, int> clientFeatures;
};

class ChatRosterView : public QTreeView
{
    Q_OBJECT

public:
    ChatRosterView(ChatRosterModel *model, QWidget *parent = NULL);
    void selectContact(const QString &jid);
    QSize sizeHint() const;

signals:
    void itemMenu(QMenu *menu, const QModelIndex &index);
    void itemDrop(QDropEvent *event, const QModelIndex &index);

public slots:
    void setShowOfflineContacts(bool show);

protected slots:
    void selectionChanged(const QItemSelection & selected, const QItemSelection &deselected);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    ChatRosterModel *rosterModel;
    QSortFilterProxyModel *sortedModel;
};

#endif
