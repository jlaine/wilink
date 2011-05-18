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
#include <QDeclarativeImageProvider>
#include <QSet>
#include <QUrl>

#include "chat_model.h"

class QNetworkDiskCache;
class QSortFilterProxyModel;
class QXmppClient;
class QXmppDiscoveryIq;
class QXmppPresence;
class QXmppVCardIq;
class QXmppVCardManager;

class ChatRosterModel;
class ChatRosterModelPrivate;

#define ROOMS_ROSTER_ID     "1_rooms"
#define CONTACTS_ROSTER_ID  "2_contacts"

class ChatRosterImageProvider : public QDeclarativeImageProvider
{
public:
    ChatRosterImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
    void setRosterModel(ChatRosterModel *rosterModel);

private:
    ChatRosterModel *m_rosterModel;
};

class ChatRosterModel : public ChatModel
{
    Q_OBJECT

public:
    enum Role {
        TypeRole = ChatModel::UserRole,
        NicknameRole,
        MessagesRole,
        PersistentRole,
        StatusRole
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
    bool setData(const QModelIndex & index, const QVariant &value, int role = Qt::EditRole);

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

    friend class ChatRosterModelPrivate;
    ChatRosterModelPrivate * const d;
};

class ChatRosterProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    ChatRosterProxyModel(QObject *parent = 0);
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
    void setSourceModel(QAbstractItemModel *rosterModel);
    QModelIndex sourceRoot() const;
    void setSourceRoot(const QModelIndex &index);

private slots:
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void onRowsAboutToBeInserted(const QModelIndex &parent, int start, int end);
    void onRowsInserted(const QModelIndex &parent, int start, int end);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    void onRowsRemoved(const QModelIndex &parent, int start, int end);

private:
    QString m_rosterRoot;
    QSet<QString> m_selection;
    QPersistentModelIndex m_sourceRoot;
};

class VCardCache : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QXmppVCardManager* manager READ manager WRITE setManager NOTIFY managerChanged)

public:
    VCardCache(QObject *parent = 0);

    QXmppVCardManager *manager() const;
    void setManager(QXmppVCardManager *manager);

    static VCardCache *instance();

signals:
    void cardChanged(const QString &jid);
    void managerChanged(QXmppVCardManager *manager);

public slots:
    bool get(const QString &jid, QXmppVCardIq *iq = 0);
    QUrl imageUrl(const QString &jid);
    QUrl profileUrl(const QString &jid);

private slots:
    void vCardReceived(const QXmppVCardIq&);

private:
    QNetworkDiskCache *m_cache;
    QXmppVCardManager *m_manager;
    QSet<QString> m_failed;
    QSet<QString> m_queue;
};

#endif
