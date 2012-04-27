/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#include <QBuffer>
#include <QDesktopServices>
#include <QDir>
#include <QDomDocument>
#include <QImageReader>
#include <QList>
#include <QMenu>
#include <QNetworkCacheMetaData>
#include <QNetworkDiskCache>
#include <QPainter>
#include <QStringList>
#include <QUrl>

#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"
#include "QXmppVCardManager.h"

#include "client.h"
#include "roster.h"

static const QChar sortSeparator('\0');

static VCardCache *vcardCache = 0;

// Try to read an IQ from disk cache.
template <class T>
bool readIq(QAbstractNetworkCache *cache, const QUrl &url, T *iq)
{
    QIODevice *ioDevice = cache->data(url);
    if (!ioDevice)
        return false;

    if (iq) {
        QDomDocument doc;
        doc.setContent(ioDevice);
        iq->parse(doc.documentElement());
    }
    delete ioDevice;
    return true;
}

// Write an IQ to disk cache.
template <class T>
void writeIq(QAbstractNetworkCache *cache, const QUrl &url, const T &iq, int cacheSeconds)
{
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    metaData.setExpirationDate(QDateTime::currentDateTime().addSecs(cacheSeconds));
    QIODevice *ioDevice = cache->prepare(metaData);
    QXmlStreamWriter writer(ioDevice);
    iq.toXml(&writer);
    cache->insert(ioDevice);
}

class RosterItem : public ChatModelItem
{
public:
    RosterItem();
    ~RosterItem();
    QString statusName() const;
    VCard* vcard();

    QString jid;
    int messages;

private:
    VCard *m_vcard;
};

RosterItem::RosterItem()
    : m_vcard(0)
{
}

RosterItem::~RosterItem()
{
    if (m_vcard)
        delete m_vcard;
}

VCard *RosterItem::vcard()
{
    if (!m_vcard) {
        m_vcard = new VCard;
        m_vcard->setJid(jid);
    }
    return m_vcard;
}

QString RosterItem::statusName() const
{
    const QXmppPresence::Status::Type type = VCardCache::instance()->presenceStatus(jid);
    if (type == QXmppPresence::Status::Offline)
        return "offline";
    else if (type == QXmppPresence::Status::Online || type == QXmppPresence::Status::Chat)
        return "available";
    else if (type == QXmppPresence::Status::Away || type == QXmppPresence::Status::XA)
        return "away";
    else
        return "busy";
}

RosterImageProvider::RosterImageProvider()
    : QDeclarativeImageProvider(Pixmap)
{
}

QPixmap RosterImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QPixmap pixmap;

    // read vCard image
    QXmppVCardIq vcard;
    if (VCardCache::instance()->get(id, &vcard)) {
        QBuffer buffer;
        buffer.setData(vcard.photo());
        buffer.open(QIODevice::ReadOnly);
        QImageReader imageReader(&buffer);
        if (requestedSize.isValid())
            imageReader.setScaledSize(requestedSize);
        pixmap = QPixmap::fromImage(imageReader.read());
    }
    if (pixmap.isNull()) {
        qWarning("Could not get roster picture for %s", qPrintable(id));
        pixmap = QPixmap(":/peer.png");
        if (requestedSize.isValid())
            pixmap = pixmap.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);
    }

    if (size)
        *size = pixmap.size();
    return pixmap;
}

class RosterModelPrivate
{
public:
    RosterItem* find(const QString &id, ChatModelItem *parent = 0);

    RosterModel *q;
    ChatClient *client;
};

RosterItem *RosterModelPrivate::find(const QString &id, ChatModelItem *parent)
{
    if (!parent)
        parent = q->rootItem;

    /* look at immediate children */
    foreach (ChatModelItem *it, parent->children) {
        RosterItem *item = static_cast<RosterItem*>(it);
        if (item->jid == id)
            return item;
    }

    /* recurse */
    foreach (ChatModelItem *it, parent->children) {
        RosterItem *item = static_cast<RosterItem*>(it);
        RosterItem *found = find(id, item);
        if (found)
            return found;
    }
    return 0;
}

RosterModel::RosterModel(QObject *parent)
    : ChatModel(parent),
    d(new RosterModelPrivate)
{
    d->client = 0;
    d->q = this;

    // set additionals role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(RosterModel::StatusRole, "status");
    setRoleNames(names);
}

RosterModel::~RosterModel()
{
    delete d;
}

ChatClient *RosterModel::client() const
{
    return d->client;
}

void RosterModel::setClient(ChatClient *client)
{
    bool check;
    Q_UNUSED(check);

    if (client != d->client) {
        d->client = client;

        VCardCache::instance()->addClient(d->client);

        check = connect(d->client, SIGNAL(connected()),
                        this, SLOT(_q_connected()));
        Q_ASSERT(check);

        check = connect(d->client, SIGNAL(disconnected()),
                        this, SLOT(_q_disconnected()));
        Q_ASSERT(check);

        check = connect(d->client->rosterManager(), SIGNAL(itemAdded(QString)),
                        this, SLOT(_q_itemAdded(QString)));
        Q_ASSERT(check);

        check = connect(d->client->rosterManager(), SIGNAL(itemRemoved(QString)),
                        this, SLOT(_q_itemRemoved(QString)));
        Q_ASSERT(check);

        check = connect(d->client->rosterManager(), SIGNAL(presenceChanged(QString,QString)),
                        this, SLOT(_q_itemChanged(QString)));
        Q_ASSERT(check);

        // use a queued connection so that the VCard gets updated first
        check = connect(VCardCache::instance(), SIGNAL(cardChanged(QString)),
                        this, SLOT(_q_itemChanged(QString)), Qt::QueuedConnection);
        Q_ASSERT(check);

        check = connect(d->client->rosterManager(), SIGNAL(rosterReceived()),
                        this, SLOT(_q_rosterReceived()));
        Q_ASSERT(check);

        if (client->state() == QXmppClient::ConnectedState)
            _q_connected();

        if (client->rosterManager()->isRosterReceived())
            QMetaObject::invokeMethod(this, "_q_rosterReceived", Qt::QueuedConnection);

        emit clientChanged(d->client);
    }
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    RosterItem *item = static_cast<RosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == JidRole) {
        return item->jid;
    } else if (role == AvatarRole) {
        return item->vcard()->avatar();
    } else if (role == MessagesRole) {
        return item->messages;
    } else if (role == NameRole) {
        return item->vcard()->name();
    } else if (role == StatusRole) {
        return VCardCache::instance()->presenceStatus(item->jid);
    } else if (role == StatusFilterRole) {
        return item->statusName();
    } else if (role == StatusSortRole) {
        return item->statusName() + sortSeparator + item->vcard()->name().toLower() + sortSeparator + item->jid.toLower();
    }

    return QVariant();
}

bool RosterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    RosterItem *item = static_cast<RosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return false;

    if (role == MessagesRole) {
        item->messages = value.toInt();
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

void RosterModel::_q_connected()
{
    // request own vCard
    VCardCache::instance()->get(d->client->configuration().jidBare());
}

void RosterModel::_q_disconnected()
{
    if (rootItem->children.size() > 0)
    {
        ChatModelItem *first = rootItem->children.first();
        ChatModelItem *last = rootItem->children.last();
        emit dataChanged(createIndex(first), createIndex(last));
    }
}

/** Handles an item being added to the roster.
 */
void RosterModel::_q_itemAdded(const QString &jid)
{
    RosterItem *item = d->find(jid);
    if (item)
        return;

    // add a new entry
    const QXmppRosterIq::Item entry = d->client->rosterManager()->getRosterEntry(jid);
    item = new RosterItem;
    item->jid = jid;
    item->messages = 0;
    ChatModel::addItem(item, rootItem);
}

/** Handles an item being changed in the roster.
 */
void RosterModel::_q_itemChanged(const QString &jid)
{
    RosterItem *item = d->find(jid);
    if (item)
        emit dataChanged(createIndex(item), createIndex(item));
}

/** Handles an item being removed from the roster.
 */
void RosterModel::_q_itemRemoved(const QString &jid)
{
    RosterItem *item = d->find(jid);
    if (item)
        removeItem(item);
}

void RosterModel::_q_rosterReceived()
{
    // make a note of existing contacts
    QStringList oldJids;
    foreach (ChatModelItem *item, rootItem->children) {
        RosterItem *child = static_cast<RosterItem*>(item);
        oldJids << child->jid;
    }

    // process received entries
    foreach (const QString &jid, d->client->rosterManager()->getRosterBareJids()) {
        _q_itemAdded(jid);
        oldJids.removeAll(jid);
    }

    // remove obsolete entries
    foreach (const QString &jid, oldJids) {
        RosterItem *item = d->find(jid);
        if (item)
            removeItem(item);
    }
}

void RosterModel::addPendingMessage(const QString &bareJid)
{
    RosterItem *item = d->find(bareJid);
    if (item)
    {
        item->messages++;
        emit dataChanged(createIndex(item), createIndex(item));
        emit pendingMessagesChanged();
    }
}

void RosterModel::clearPendingMessages(const QString &bareJid)
{
    RosterItem *item = d->find(bareJid);
    if (item && item->messages) {
        item->messages = 0;
        emit dataChanged(createIndex(item), createIndex(item));
        emit pendingMessagesChanged();
    }
}

/** Returns the total number of pending messages.
 */
int RosterModel::pendingMessages() const
{
    int pending = 0;
    foreach (ChatModelItem *item, rootItem->children) {
        RosterItem *child = static_cast<RosterItem*>(item);
        pending += child->messages;
    }
    return pending;
}

class VCardCachePrivate
{
public:
    QNetworkDiskCache *cache;
    QList<ChatClient*> clients;
    QSet<QString> discoQueue;
    QMap<QString, VCard::Features> features;
    QSet<QString> vcardFailed;
    QSet<QString> vcardQueue;
};

VCard::VCard(QObject *parent)
    : QObject(parent),
    m_cache(0)
{
#ifdef DEBUG_ROSTER
    qDebug("creating vcard %s", qPrintable(jid));
#endif
}

QUrl VCard::avatar() const
{
    return m_avatar;
}

VCard::Features VCard::features() const
{
    if (!m_jid.isEmpty() && m_cache)
        return m_cache->features(m_jid);
    return 0;
}

QString VCard::jid() const
{
    return m_jid;
}

/** Returns the full JID of an online contact which has the requested feature.
 *
 * @param feature
 */
QString VCard::jidForFeature(Feature feature) const
{
    if (m_jid.isEmpty() || !m_cache || !QXmppUtils::jidToResource(m_jid).isEmpty())
        return QString();

    foreach (const QString &key, m_cache->d->features.keys()) {
        if (QXmppUtils::jidToBareJid(key) == m_jid && (m_cache->d->features.value(key) & feature))
            return key;
    }
    return QString();
}

void VCard::setJid(const QString &jid)
{
    if (jid != m_jid) {
        m_jid = jid;
        emit jidChanged(m_jid);

        update();

        // reset features
        emit featuresChanged();
        emit statusChanged();
    }
}

QString VCard::name() const
{
    return m_name;
}

QString VCard::nickName() const
{
    return m_nickName;
}

int VCard::status() const
{
    if (!m_jid.isEmpty())
        return VCardCache::instance()->presenceStatus(m_jid);
    else
        return QXmppPresence::Status::Offline;
}

QUrl VCard::url() const
{
    return m_url;
}

void VCard::update()
{
    QUrl newAvatar;
    QString newName;
    QString newNickName;
    QUrl newUrl;

    // fetch data
    if (!m_jid.isEmpty()) {
        if (!m_cache) {
            m_cache = VCardCache::instance();
            connect(m_cache, SIGNAL(cardChanged(QString)),
                    this, SLOT(_q_cardChanged(QString)));
            connect(m_cache, SIGNAL(discoChanged(QString)),
                    this, SLOT(_q_discoChanged(QString)));
            connect(m_cache, SIGNAL(presenceChanged(QString)),
                    this, SLOT(_q_presenceChanged(QString)));
        }

        QXmppVCardIq vcard;
        if (m_cache->get(m_jid, &vcard)) {
            newAvatar = QUrl("image://roster/" + m_jid);
            newName = vcard.nickName();
            if (newName.isEmpty())
                newName = vcard.fullName();
            newNickName = vcard.nickName();
            newUrl = QUrl(vcard.url());
        } else {
            newAvatar = QUrl("image://icon/peer");
        }
        foreach (ChatClient *client, m_cache->d->clients) {
            const QString name = client->rosterManager()->getRosterEntry(QXmppUtils::jidToBareJid(m_jid)).name();
            if (!name.isEmpty()) {
                newName = name;
                break;
            }
        }
        if (newName.isEmpty())
            newName = QXmppUtils::jidToUser(m_jid);
    }

    // notify changes
    if (newAvatar != m_avatar) {
        m_avatar = newAvatar;
        emit avatarChanged(m_avatar);
    }
    if (newName != m_name) {
        m_name = newName;
        emit nameChanged(m_name);
    }
    if (newNickName != m_nickName) {
        m_nickName = newNickName;
        emit nickNameChanged(m_nickName);
    }
    if (newUrl != m_url) {
        m_url = newUrl;
        emit urlChanged(m_url);
    }
}

void VCard::_q_cardChanged(const QString &jid)
{
    if (jid == m_jid)
        update();
}

void VCard::_q_discoChanged(const QString &jid)
{
    if (jid == m_jid)
        emit featuresChanged();
}

void VCard::_q_presenceChanged(const QString &jid)
{
    if (jid.isEmpty() || jid == m_jid)
        emit statusChanged();
}

VCardCache::VCardCache(QObject *parent)
    : QObject(parent)
{
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);

    d = new VCardCachePrivate;
    d->cache = new QNetworkDiskCache(this);
    d->cache->setCacheDirectory(QDir(dataPath).filePath("cache"));
}

VCardCache::~VCardCache()
{
    delete d;
}

ChatClient *VCardCache::client(const QString &jid) const
{
    const QString domain = QXmppUtils::jidToDomain(jid);
    foreach (ChatClient *client, d->clients) {
        const QString clientDomain = client->configuration().domain();
        if (domain == clientDomain || domain.endsWith("." + clientDomain))
            return client;
    }
    if (!d->clients.isEmpty())
        return d->clients.first();
    return 0;
}

VCard::Features VCardCache::features(const QString &jid) const
{
    VCard::Features features = 0;
    foreach (ChatClient *client, d->clients) {
        foreach (const QXmppPresence &presence, client->rosterManager()->getAllPresencesForBareJid(jid)) {
            const QString fullJid = presence.from();
            if (presence.type() == QXmppPresence::Available) {
                if (d->features.contains(fullJid)) {
                    features |= d->features.value(fullJid);
                } else if (!d->discoQueue.contains(fullJid)) {
#ifdef DEBUG_ROSTER
                    qDebug("requesting disco %s", qPrintable(fullJid));
#endif
                    client->discoveryManager()->requestInfo(fullJid);
                    d->discoQueue.insert(fullJid);
                }
            }
        }
    }
    return features;
}

/** Tries to get the vCard for the given JID.
 *
 *  Returns true if the vCard was found, otherwise requests the card.
 *
 * @param jid
 * @param iq
 */

bool VCardCache::get(const QString &jid, QXmppVCardIq *iq)
{
    if (d->vcardQueue.contains(jid) || d->vcardFailed.contains(jid))
        return false;

    if (readIq(d->cache, QString("xmpp:%1?vcard").arg(jid), iq))
        return true;
    ChatClient *client = this->client(jid);
    if (client) {
#ifdef DEBUG_ROSTER
        qDebug("requesting vCard %s", qPrintable(jid));
#endif
        d->vcardQueue.insert(jid);
        client->vCardManager().requestVCard(jid);
    }
    return false;
}

QUrl VCardCache::imageUrl(const QString &jid)
{
    if (get(jid))
        return QUrl("image://roster/" + jid);
    else
        return QUrl("image://icon/peer");
}

VCardCache *VCardCache::instance()
{
    if (!vcardCache)
        vcardCache = new VCardCache;
    return vcardCache;
}

void VCardCache::addClient(ChatClient *client)
{
    bool check;
    Q_UNUSED(check);

    if (!client || d->clients.contains(client))
        return;

    check = connect(client, SIGNAL(destroyed(QObject*)),
                    this, SLOT(_q_clientDestroyed(QObject*)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(disconnected()),
                    this, SIGNAL(presenceChanged()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(_q_presenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(client->rosterManager(), SIGNAL(itemAdded(QString)),
                    this, SIGNAL(cardChanged(QString)));
    Q_ASSERT(check);

    check = connect(client->rosterManager(), SIGNAL(itemChanged(QString)),
                    this, SIGNAL(cardChanged(QString)));
    Q_ASSERT(check);

    check = connect(&client->vCardManager(), SIGNAL(vCardReceived(QXmppVCardIq)),
                    this, SLOT(_q_vCardReceived(QXmppVCardIq)));
    Q_ASSERT(check);

    check = connect(client->discoveryManager(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(_q_discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    d->clients << client;
}

QXmppPresence::Status::Type VCardCache::presenceStatus(const QString &jid) const
{
    QXmppPresence::Status::Type statusType = QXmppPresence::Status::Offline;

    foreach (ChatClient *client, d->clients) {
        // NOTE : we test the connection status, otherwise we encounter a race
        // condition upon disconnection, because the roster has not yet been cleared
        if (!client->isConnected())
            continue;

        foreach (const QXmppPresence &presence, client->rosterManager()->getAllPresencesForBareJid(jid)) {
            QXmppPresence::Status::Type type = presence.status().type();
            if (type == QXmppPresence::Status::Offline)
                continue;
            // FIXME : we should probably be using the priority rather than
            // stop at the first available contact
            else if (type == QXmppPresence::Status::Online ||
                     type == QXmppPresence::Status::Chat)
                return type;
            else
                statusType = type;
        }
    }
    return statusType;
}

void VCardCache::_q_clientDestroyed(QObject *object)
{
    d->clients.removeAll(static_cast<ChatClient*>(object));
}

void VCardCache::_q_discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    const QString jid = disco.from();
    if (!d->discoQueue.remove(jid) || disco.type() != QXmppIq::Result)
        return;

#ifdef DEBUG_ROSTER
    qDebug("received disco %s", qPrintable(jid));
#endif
    VCard::Features features = 0;
    foreach (const QString &var, disco.features())
    {
        if (var == "http://jabber.org/protocol/chatstates")
            features |= VCard::ChatStatesFeature;
        else if (var == "http://jabber.org/protocol/si/profile/file-transfer")
            features |= VCard::FileTransferFeature;
        else if (var == "jabber:iq:version")
            features |= VCard::VersionFeature;
        else if (var == "urn:xmpp:jingle:apps:rtp:audio")
            features |= VCard::VoiceFeature;
        else if (var == "urn:xmpp:jingle:apps:rtp:video")
            features |= VCard::VideoFeature;
    }
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities()) {
        if (id.name() == QLatin1String("iChatAgent"))
            features |= VCard::ChatStatesFeature;
    }
    d->features.insert(jid, features);

    emit discoChanged(QXmppUtils::jidToBareJid(jid));
}

void VCardCache::_q_presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.from();
    if (QXmppUtils::jidToResource(jid).isEmpty())
        return;

    if ((presence.type() == QXmppPresence::Available && !d->features.contains(jid)) ||
        (presence.type() == QXmppPresence::Unavailable && d->features.remove(jid)))
        emit discoChanged(QXmppUtils::jidToBareJid(jid));

    emit presenceChanged(QXmppUtils::jidToBareJid(jid));
}

void VCardCache::_q_vCardReceived(const QXmppVCardIq& vCard)
{
    const QString jid = vCard.from();
    if (!d->vcardQueue.remove(jid))
        return;

    if (vCard.type() == QXmppIq::Result) {
#ifdef DEBUG_ROSTER
        qDebug("received vCard %s", qPrintable(jid));
#endif
        d->vcardFailed.remove(jid);
        writeIq(d->cache, QString("xmpp:%1?vcard").arg(jid), vCard, 3600);
        emit cardChanged(jid);
    } else if (vCard.type() == QXmppIq::Error) {
#ifdef DEBUG_ROSTER
        qWarning("failed vCard %s", qPrintable(jid));
#endif
        d->vcardFailed.insert(jid);
    }
}

