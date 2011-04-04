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

#define HOME_ROSTER_ID      "1_home"
#define ROOMS_ROSTER_ID     "2_rooms"
#define CONTACTS_ROSTER_ID  "z_1_contacts"

class ChatRosterModel : public ChatModel
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
        PersistentRole,
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

    enum Type {
        Root,
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
    QString contactExtra(const QString &bareJid) const;
    QString contactName(const QString &jid) const;
    bool isOwnNameReceived() const;
    QString ownName() const;

    QModelIndex addItem(ChatRosterModel::Type type, const QString &id, const QString &name = QString(), const QIcon &icon = QIcon(), const QModelIndex &parent = QModelIndex());
    QModelIndex contactsItem() const;
    QModelIndex findItem(const QString &jid) const;

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
    void discoveryInfoFound(const QXmppDiscoveryIq &disco);
    void vCardFound(const QXmppVCardIq&);

    friend class ChatRosterModelPrivate;
    ChatRosterModelPrivate * const d;
};

class ChatRosterView : public QTreeView
{
    Q_OBJECT

public:
    ChatRosterView(ChatRosterModel *model, QWidget *parent = NULL);
    QModelIndex mapFromRoster(const QModelIndex &index);
    QSize sizeHint() const;

signals:
    void itemMenu(QMenu *menu, const QModelIndex &index);
    void itemDrop(QDropEvent *event, const QModelIndex &index);

public slots:
    void setShowOfflineContacts(bool show);

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
