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

#include <QBuffer>
#include <QDebug>
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

#include "QXmppConstants.h"
#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"
#include "QXmppVCardManager.h"

#include "application.h"
#include "chat_client.h"
#include "chat_roster.h"

static const QChar sortSeparator('\0');

static VCardCache *vcardCache = 0;

enum RosterColumns {
    ContactColumn = 0,
    SortingColumn,
    MaxColumn,
};

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

class ChatRosterItem : public ChatModelItem
{
public:
    QString jid;
    int messages;
};

ChatRosterImageProvider::ChatRosterImageProvider()
    : QDeclarativeImageProvider(Pixmap)
{
}

QPixmap ChatRosterImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
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

static void paintMessages(QPixmap &icon, int messages)
{
    QString pending = QString::number(messages);
    QPainter painter(&icon);
    QFont font = painter.font();
    font.setWeight(QFont::Bold);
    painter.setFont(font);

    // text rectangle
    QRect rect = painter.fontMetrics().boundingRect(pending);
    rect.setWidth(rect.width() + 4);
    if (rect.width() < rect.height())
        rect.setWidth(rect.height());
    else
        rect.setHeight(rect.width());
    rect.moveTop(2);
    rect.moveRight(icon.width() - 2);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(Qt::red);
    painter.setPen(Qt::white);
    painter.drawEllipse(rect);
    painter.drawText(rect, Qt::AlignCenter, pending);
}

class ChatRosterModelPrivate
{
public:
    int countPendingMessages();
    ChatRosterItem* find(const QString &id, ChatModelItem *parent = 0);

    ChatRosterModel *q;
    ChatClient *client;
};

/** Count the current number of pending messages.
 */
int ChatRosterModelPrivate::countPendingMessages()
{
    int pending = 0;
    foreach (ChatModelItem *item, q->rootItem->children) {
        ChatRosterItem *child = static_cast<ChatRosterItem*>(item);
        pending += child->messages;
    }
    return pending;
}

ChatRosterItem *ChatRosterModelPrivate::find(const QString &id, ChatModelItem *parent)
{
    if (!parent)
        parent = q->rootItem;

    /* look at immediate children */
    foreach (ChatModelItem *it, parent->children) {
        ChatRosterItem *item = static_cast<ChatRosterItem*>(it);
        if (item->jid == id)
            return item;
    }

    /* recurse */
    foreach (ChatModelItem *it, parent->children) {
        ChatRosterItem *item = static_cast<ChatRosterItem*>(it);
        ChatRosterItem *found = find(id, item);
        if (found)
            return found;
    }
    return 0;
}

ChatRosterModel::ChatRosterModel(ChatClient *xmppClient, QObject *parent)
    : ChatModel(parent),
    d(new ChatRosterModelPrivate)
{
    d->q = this;

    // set additionals role names
    QHash<int, QByteArray> names = roleNames();
    names.insert(ChatRosterModel::StatusRole, "status");
    setRoleNames(names);

    // get cache
    VCardCache::instance()->addClient(xmppClient);

    d->client = xmppClient;

    bool check;
    check = connect(d->client, SIGNAL(connected()),
                    this, SLOT(_q_connected()));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(disconnected()),
                    this, SLOT(_q_disconnected()));
    Q_ASSERT(check);

    check = connect(d->client->rosterManager(), SIGNAL(itemAdded(QString)),
                    this, SLOT(itemAdded(QString)));
    Q_ASSERT(check);

    check = connect(d->client->rosterManager(), SIGNAL(itemChanged(QString)),
                    this, SLOT(itemChanged(QString)));
    Q_ASSERT(check);

    check = connect(d->client->rosterManager(), SIGNAL(itemRemoved(QString)),
                    this, SLOT(itemRemoved(QString)));
    Q_ASSERT(check);

    check = connect(d->client->rosterManager(), SIGNAL(presenceChanged(QString, QString)),
                    this, SLOT(presenceChanged(QString, QString)));
    Q_ASSERT(check);

    check = connect(d->client->rosterManager(), SIGNAL(rosterReceived()),
                    this, SLOT(rosterReceived()));
    Q_ASSERT(check);
}

ChatRosterModel::~ChatRosterModel()
{
    delete d;
}

int ChatRosterModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MaxColumn;
}

static QString contactStatus(const QModelIndex &index)
{
    const int typeVal = index.data(ChatRosterModel::StatusRole).toInt();
    QXmppPresence::Status::Type type = static_cast<QXmppPresence::Status::Type>(typeVal);
    if (type == QXmppPresence::Status::Offline)
        return "offline";
    else if (type == QXmppPresence::Status::Online || type == QXmppPresence::Status::Chat)
        return "available";
    else if (type == QXmppPresence::Status::Away || type == QXmppPresence::Status::XA)
        return "away";
    else
        return "busy";
}

QVariant ChatRosterModel::data(const QModelIndex &index, int role) const
{
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == JidRole) {
        return item->jid;
    } else if (role == AvatarRole) {
        return VCardCache::instance()->imageUrl(item->jid);
    } else if (role == MessagesRole) {
        return item->messages;
    } else if (role == NameRole) {
        VCard card;
        card.setJid(item->jid);
        if (index.column() == SortingColumn)
            return contactStatus(index) + sortSeparator + card.name().toLower() + sortSeparator + item->jid.toLower();
        else
            return card.name();
    } else if (role == UrlRole) {
        return VCardCache::instance()->profileUrl(item->jid);
    } else if (role == StatusRole) {
        QXmppPresence::Status::Type statusType = QXmppPresence::Status::Offline;
        // NOTE : we test the connection status, otherwise we encounter a race
        // condition upon disconnection, because the roster has not yet been cleared
        if (!d->client->isConnected())
            return statusType;
        foreach (const QXmppPresence &presence, d->client->rosterManager()->getAllPresencesForBareJid(item->jid)) {
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
        return statusType;
    }

    return QVariant();
}

bool ChatRosterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return false;

    if (role == MessagesRole) {
        item->messages = value.toInt();
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

void ChatRosterModel::_q_connected()
{
    // request own vCard
    VCardCache::instance()->get(d->client->configuration().jidBare());
}

void ChatRosterModel::_q_disconnected()
{
    if (rootItem->children.size() > 0)
    {
        ChatModelItem *first = rootItem->children.first();
        ChatModelItem *last = rootItem->children.last();
        emit dataChanged(createIndex(first, ContactColumn),
                         createIndex(last, SortingColumn));
    }
}

/** Handles an item being added to the roster.
 */
void ChatRosterModel::itemAdded(const QString &jid)
{
    ChatRosterItem *item = d->find(jid);
    if (item)
        return;

    // add a new entry
    const QXmppRosterIq::Item entry = d->client->rosterManager()->getRosterEntry(jid);
    item = new ChatRosterItem;
    item->jid = jid;
    item->messages = 0;
    ChatModel::addItem(item, rootItem);
}

/** Handles an item being changed in the roster.
 */
void ChatRosterModel::itemChanged(const QString &jid)
{
    ChatRosterItem *item = d->find(jid);
    if (!item)
        return;

    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));
}

/** Handles an item being removed from the roster.
 */
void ChatRosterModel::itemRemoved(const QString &jid)
{
    ChatRosterItem *item = d->find(jid);
    if (item)
        removeItem(item);
}

void ChatRosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    Q_UNUSED(resource);
    ChatRosterItem *item = d->find(bareJid);
    if (item)
        emit dataChanged(createIndex(item, ContactColumn),
                         createIndex(item, SortingColumn));
}

void ChatRosterModel::rosterReceived()
{
    // make a note of existing contacts
    QStringList oldJids;
    foreach (ChatModelItem *item, rootItem->children) {
        ChatRosterItem *child = static_cast<ChatRosterItem*>(item);
        oldJids << child->jid;
    }

    // process received entries
    foreach (const QString &jid, d->client->rosterManager()->getRosterBareJids()) {
        itemAdded(jid);
        oldJids.removeAll(jid);
    }

    // remove obsolete entries
    foreach (const QString &jid, oldJids) {
        ChatRosterItem *item = d->find(jid);
        if (item)
            removeItem(item);
    }

    // trigger resize
    emit rosterReady();
}

void ChatRosterModel::addPendingMessage(const QString &bareJid)
{
    ChatRosterItem *item = d->find(bareJid);
    if (item)
    {
        item->messages++;
        emit dataChanged(createIndex(item, ContactColumn),
                         createIndex(item, SortingColumn));
        emit pendingMessages(d->countPendingMessages());
    }
}

void ChatRosterModel::clearPendingMessages(const QString &bareJid)
{
    ChatRosterItem *item = d->find(bareJid);
    if (item && item->messages) {
        item->messages = 0;
        emit dataChanged(createIndex(item, ContactColumn),
                         createIndex(item, SortingColumn));
        emit pendingMessages(d->countPendingMessages());
    }
}

VCard::VCard(QObject *parent)
    : QObject(parent),
    m_cache(0)
{
}

QUrl VCard::avatar() const
{
    return m_avatar;
}

void VCard::cardChanged(const QString &jid)
{
    if (jid == m_jid)
        update();
}

VCard::Features VCard::features() const
{
    return m_features;
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
    if (m_jid.isEmpty() || !m_cache)
        return m_jid;

    if (jidToResource(m_jid).isEmpty()) {
        foreach (const QString &key, m_cache->m_features.keys()) {
            if (jidToBareJid(key) == m_jid)
                return key;
        }
    }
    return m_jid;
}

void VCard::setJid(const QString &jid)
{
    if (jid != m_jid) {
        m_jid = jid;
        emit jidChanged(m_jid);

        update();
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

QUrl VCard::url() const
{
    return m_url;
}

void VCard::update()
{
    QUrl newAvatar;
    Features newFeatures = 0;
    QString newName;
    QString newNickName;
    QUrl newUrl;

    // fetch data
    if (!m_jid.isEmpty()) {
        if (!m_cache) {
            m_cache = VCardCache::instance();
            connect(m_cache, SIGNAL(cardChanged(QString)),
                    this, SLOT(cardChanged(QString)));
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
            newAvatar = QUrl("qrc:/peer.png");
        }
        foreach (ChatClient *client, m_cache->m_clients) {
            const QString name = client->rosterManager()->getRosterEntry(jidToBareJid(m_jid)).name();
            if (!name.isEmpty()) {
                newName = name;
                break;
            }
        }
        if (newName.isEmpty())
            newName = jidToUser(m_jid);

        // features
        if (!jidToResource(m_jid).isEmpty()) {
            newFeatures = m_cache->m_features.value(m_jid);
        } else {
            foreach (const QString &jid, m_cache->m_features.keys()) {
                if (jidToBareJid(jid) == m_jid)
                    newFeatures |= m_cache->m_features.value(jid);
            }
        }
    }

    // notify changes
    if (newAvatar != m_avatar) {
        m_avatar = newAvatar;
        emit avatarChanged(m_avatar);
    }
    if (newFeatures != m_features) {
        m_features = newFeatures;
        emit featuresChanged(m_features);
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

VCardCache::VCardCache(QObject *parent)
    : QObject(parent)
{
    m_cache = new QNetworkDiskCache(this);
    m_cache->setCacheDirectory(wApp->cacheDirectory());
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
    if (m_queue.contains(jid) || m_failed.contains(jid))
        return false;

    if (readIq(m_cache, QString("xmpp:%1?vcard").arg(jid), iq))
        return true;
    if (!m_clients.isEmpty()) {
        ChatClient *client = m_clients.first();
        qDebug("requesting vCard %s", qPrintable(jid));
        m_queue.insert(jid);
        client->vCardManager().requestVCard(jid);
    }
    return false;
}

QUrl VCardCache::imageUrl(const QString &jid)
{
    if (get(jid))
        return QUrl("image://roster/" + jid);
    else
        return QUrl("qrc:/peer.png");
}

QUrl VCardCache::profileUrl(const QString &jid)
{
    QXmppVCardIq vcard;
    if (get(jid, &vcard))
        return QUrl(vcard.url());
    else
        return QUrl();
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

    if (!client || m_clients.contains(client))
        return;

    check = connect(client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(presenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(client->rosterManager(), SIGNAL(itemAdded(QString)),
                    this, SIGNAL(cardChanged(QString)));
    Q_ASSERT(check);

    check = connect(client->rosterManager(), SIGNAL(itemChanged(QString)),
                    this, SIGNAL(cardChanged(QString)));
    Q_ASSERT(check);

    check = connect(&client->vCardManager(), SIGNAL(vCardReceived(QXmppVCardIq)),
                    this, SLOT(vCardReceived(QXmppVCardIq)));
    Q_ASSERT(check);

    check = connect(client->discoveryManager(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    m_clients << client;
}

void VCardCache::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() != QXmppIq::Result)
        return;

    const QString jid = disco.from();
    qDebug("received disco %s", qPrintable(jid));
    VCard::Features features = 0;
    foreach (const QString &var, disco.features())
    {
        if (var == ns_chat_states)
            features |= VCard::ChatStatesFeature;
        else if (var == ns_stream_initiation_file_transfer)
            features |= VCard::FileTransferFeature;
        else if (var == ns_version)
            features |= VCard::VersionFeature;
        else if (var == ns_jingle_rtp_audio)
            features |= VCard::VoiceFeature;
        else if (var == ns_jingle_rtp_video)
            features |= VCard::VideoFeature;
    }
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities()) {
        if (id.name() == "iChatAgent")
            features |= VCard::ChatStatesFeature;
    }
    m_features.insert(jid, features);

    emit cardChanged(jid);
}

void VCardCache::presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.from();
    const QString bareJid = jidToBareJid(jid);
    const QString resource = jidToResource(jid);

    // handle features discovery
    if (!resource.isEmpty())
    {
        if (presence.type() == QXmppPresence::Unavailable)
            m_features.remove(jid);
        else if (presence.type() == QXmppPresence::Available && !m_features.contains(jid) && !m_clients.isEmpty()) {
            qDebug("requesting disco %s", qPrintable(jid));
            QXmppDiscoveryIq disco;
            m_clients.first()->discoveryManager()->requestInfo(jid);
        }
    }
}

void VCardCache::vCardReceived(const QXmppVCardIq& vCard)
{
    const QString jid = vCard.from();
    if (!m_queue.remove(jid))
        return;

    if (vCard.type() == QXmppIq::Result) {
        qDebug("received vCard %s", qPrintable(jid));
        m_failed.remove(jid);
        writeIq(m_cache, QString("xmpp:%1?vcard").arg(jid), vCard, 3600);
        emit cardChanged(jid);
    } else if (vCard.type() == QXmppIq::Error) {
        qDebug("failed vCard %s", qPrintable(jid));
        m_failed.insert(jid);
    }
}

