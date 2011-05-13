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

#ifndef __WILINK_CHAT_ROSTER_H__
#define __WILINK_CHAT_ROSTER_H__

#include <QAbstractProxyModel>
#include <QTreeView>

#include "chat_model.h"

class QContextMenuEvent;
class QMenu;
class QSortFilterProxyModel;
class QXmppClient;
class QXmppDiscoveryIq;
class QXmppPresence;
class QXmppVCardIq;
class QXmppVCardManager;

class ChatRosterModelPrivate;

#define ROOMS_ROSTER_ID     "1_rooms"
#define CONTACTS_ROSTER_ID  "2_contacts"

class ChatRosterModel : public ChatModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole,
        TypeRole,
        AvatarRole,
        NicknameRole,
        MessagesRole,
        PersistentRole,
        StatusRole,
        UrlRole,
    };

    enum Feature {
        ChatStatesFeature = 1,
        FileTransferFeature = 2,
        VersionFeature = 4,
        VoiceFeature = 8,
        VideoFeature = 16,
    };

    enum Type {
        Contact,
        Room,
        RoomMember,
        Other,
    };

    ChatRosterModel(QXmppClient *client, QObject *parent = 0);
    ~ChatRosterModel();

    // QAbstractItemModel interface
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
    bool setData(const QModelIndex & index, const QVariant &value, int role = Qt::EditRole);

    QPixmap contactAvatar(const QString &jid) const;
    QStringList contactFeaturing(const QString &bareJid, ChatRosterModel::Feature) const;
    QString contactName(const QString &jid) const;
    bool isOwnNameReceived() const;
    QString ownName() const;

    QModelIndex addItem(ChatRosterModel::Type type, const QString &id, const QString &name = QString(), const QPixmap &pixmap = QPixmap(), const QModelIndex &parent = QModelIndex());
    QModelIndex contactsItem() const;
    QModelIndex findItem(const QString &jid, const QModelIndex &parent = QModelIndex()) const;

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
    void itemAdded(const QString &jid);
    void itemChanged(const QString &jid);
    void itemRemoved(const QString &jid);
    void presenceChanged(const QString& bareJid, const QString& resource);
    void presenceReceived(const QXmppPresence &presence);
    void rosterReceived();
    void vCardReceived(const QXmppVCardIq&);

private:
    void discoveryInfoFound(const QXmppDiscoveryIq &disco);
    void vCardFound(const QXmppVCardIq&);

    friend class ChatRosterModelPrivate;
    ChatRosterModelPrivate * const d;
};

class ChatRosterProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    ChatRosterProxyModel(ChatRosterModel *rosterModel, const QString &rosterRoot, QObject *parent = 0);
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QStringList selectedJids() const;

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void onRowsRemoved(const QModelIndex &parent, int start, int end);

private:
    bool isRoot(const QModelIndex &sourceIndex) const
    {
        return sourceIndex.isValid() && sourceIndex == m_rosterModel->findItem(m_rosterRoot);
    }
    ChatRosterModel *m_rosterModel;
    QString m_rosterRoot;
    QSet<QString> m_selection;
};

class ChatRosterView : public QTreeView
{
    Q_OBJECT

public:
    ChatRosterView(ChatRosterModel *model, QWidget *parent = NULL);
    QModelIndex mapFromRoster(const QModelIndex &index);
    void setExpanded(const QString &id, bool expanded);
    QSize sizeHint() const;

signals:
    void itemMenu(QMenu *menu, const QModelIndex &index);

public slots:
    void setShowOfflineContacts(bool show);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    ChatRosterModel *rosterModel;
    QSortFilterProxyModel *sortedModel;
};

#endif
