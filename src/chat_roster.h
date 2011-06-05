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

#include <QDeclarativeImageProvider>
#include <QSet>
#include <QUrl>

#include "chat_model.h"

class QNetworkDiskCache;
class QXmppDiscoveryIq;
class QXmppPresence;
class QXmppVCardIq;
class QXmppVCardManager;

class ChatClient;
class ChatRosterModel;
class ChatRosterModelPrivate;
class VCardCache;

class ChatRosterImageProvider : public QDeclarativeImageProvider
{
public:
    ChatRosterImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);
};

class ChatRosterModel : public ChatModel
{
    Q_OBJECT

public:
    enum Role {
        StatusRole = ChatModel::UserRole,
    };

    ChatRosterModel(ChatClient *client, QObject *parent = 0);
    ~ChatRosterModel();

    // QAbstractItemModel interface
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    QModelIndex findItem(const QString &jid, const QModelIndex &parent = QModelIndex()) const;

signals:
    void pendingMessages(int messages);
    void rosterReady();

public slots:
    void addPendingMessage(const QString &bareJid);
    void clearPendingMessages(const QString &bareJid);

private slots:
    void _q_connected();
    void _q_disconnected();
    void itemAdded(const QString &jid);
    void itemChanged(const QString &jid);
    void itemRemoved(const QString &jid);
    void presenceChanged(const QString& bareJid, const QString& resource);
    void rosterReceived();

private:
    friend class ChatRosterModelPrivate;
    ChatRosterModelPrivate * const d;
};

class VCard : public QObject
{
    Q_OBJECT
    Q_FLAGS(Feature Features)
    Q_PROPERTY(QUrl avatar READ avatar NOTIFY avatarChanged)
    Q_PROPERTY(Features features READ features NOTIFY featuresChanged)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString nickName READ nickName NOTIFY nickNameChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)

public:
    enum Feature {
        ChatStatesFeature = 1,
        FileTransferFeature = 2,
        VersionFeature = 4,
        VoiceFeature = 8,
        VideoFeature = 16,
    };
    Q_DECLARE_FLAGS(Features, Feature)

    VCard(QObject *parent = 0);

    QUrl avatar() const;
    Features features() const;
    QString jid() const;
    void setJid(const QString &jid);
    QString name() const;
    QString nickName() const;
    QUrl url() const;

signals:
    void avatarChanged(const QUrl &avatar);
    void featuresChanged(Features features);
    void jidChanged(const QString &jid);
    void nameChanged(const QString &name);
    void nickNameChanged(const QString &nickName);
    void urlChanged(const QUrl &url);

public slots:
    QString jidForFeature(Feature feature) const;

private slots:
    void cardChanged(const QString &jid);

private:
    void update();

    VCardCache *m_cache;
    QUrl m_avatar;
    Features m_features;
    QString m_jid;
    QString m_name;
    QString m_nickName;
    QUrl m_url;
};

class VCardCache : public QObject
{
    Q_OBJECT

public:
    static VCardCache *instance();

signals:
    void cardChanged(const QString &jid);

public slots:
    void addClient(ChatClient *client);
    bool get(const QString &jid, QXmppVCardIq *iq = 0);
    QUrl imageUrl(const QString &jid);
    QUrl profileUrl(const QString &jid);

private slots:
    void discoveryInfoReceived(const QXmppDiscoveryIq &disco);
    void presenceReceived(const QXmppPresence &presence);
    void vCardReceived(const QXmppVCardIq&);

private:
    VCardCache(QObject *parent = 0);

    QNetworkDiskCache *m_cache;
    QList<ChatClient*> m_clients;
    QMap<QString, VCard::Features> m_features;
    QSet<QString> m_failed;
    QSet<QString> m_queue;

    friend class VCard;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(VCard::Features)

#endif
