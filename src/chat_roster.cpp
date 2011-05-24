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

#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppDiscoveryIq.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMessage.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"
#include "QXmppVCardManager.h"

#include "application.h"
#include "chat_roster.h"

static const QChar sortSeparator('\0');

static VCardCache *vcardCache = 0;

enum RosterColumns {
    ContactColumn = 0,
    StatusColumn,
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
    ChatRosterItem(ChatRosterModel::Type type);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QString jid;
    QMap<int, QVariant> itemData;
    ChatRosterModel::Type itemType;
};

ChatRosterItem::ChatRosterItem(enum ChatRosterModel::Type type)
    : itemType(type)
{
}

QVariant ChatRosterItem::data(int role) const
{
    return itemData.value(role);
}

void ChatRosterItem::setData(int role, const QVariant &value)
{
    itemData.insert(role, value);
}

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
    void fetchVCard(const QString &jid);
    ChatRosterItem* find(const QString &id, ChatModelItem *parent = 0);

    ChatRosterModel *q;
    QNetworkDiskCache *cache;
    QXmppClient *client;
    ChatRosterItem *ownItem;
    bool nickNameReceived;
    QMap<QString, int> clientFeatures;
};

/** Count the current number of pending messages.
 */
int ChatRosterModelPrivate::countPendingMessages()
{
    int pending = 0;
    foreach (ChatModelItem *item, q->rootItem->children) {
        ChatRosterItem *child = static_cast<ChatRosterItem*>(item);
        pending += child->data(ChatRosterModel::MessagesRole).toInt();
    }
    return pending;
}

/** Check whether the vCard for the given roster item is cached, otherwise
 *  request the vCard.
 *
 * @param item
 */
void ChatRosterModelPrivate::fetchVCard(const QString &jid)
{
    QXmppVCardIq vCard;
    if (VCardCache::instance()->get(jid, &vCard));
        q->vCardReceived(vCard);
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

ChatRosterModel::ChatRosterModel(QXmppClient *xmppClient, QObject *parent)
    : ChatModel(parent),
    d(new ChatRosterModelPrivate)
{
    d->q = this;

    /* get cache */
    VCardCache::instance()->setManager(&xmppClient->vCardManager());
    d->cache = new QNetworkDiskCache(this);
    d->cache->setCacheDirectory(wApp->cacheDirectory());

    d->client = xmppClient;
    d->nickNameReceived = false;
    d->ownItem = new ChatRosterItem(ChatRosterModel::Contact);
    ChatModel::addItem(d->ownItem, rootItem);

    bool check;
    check = connect(d->client, SIGNAL(connected()),
                    this, SLOT(connected()));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(disconnected()),
                    this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(d->client->findExtension<QXmppDiscoveryManager>(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(presenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(itemAdded(QString)),
                    this, SLOT(itemAdded(QString)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(itemChanged(QString)),
                    this, SLOT(itemChanged(QString)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(itemRemoved(QString)),
                    this, SLOT(itemRemoved(QString)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(presenceChanged(QString, QString)),
                    this, SLOT(presenceChanged(QString, QString)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(rosterReceived()),
                    this, SLOT(rosterReceived()));
    Q_ASSERT(check);

    check = connect(&d->client->vCardManager(), SIGNAL(vCardReceived(QXmppVCardIq)),
                    this, SLOT(vCardReceived(QXmppVCardIq)));
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

void ChatRosterModel::connected()
{
    // request own vCard
    d->nickNameReceived = false;
    d->ownItem->jid = d->client->configuration().jidBare();
    d->ownItem->setData(Qt::DisplayRole, d->client->configuration().user());
    d->ownItem->setData(NicknameRole, d->client->configuration().user());
    d->fetchVCard(d->ownItem->jid);
    emit dataChanged(createIndex(d->ownItem), createIndex(d->ownItem));
}

/** Returns the full JID of an online contact which has the requested feature.
 *
 * @param bareJid
 * @param feature
 */
QStringList ChatRosterModel::contactFeaturing(const QString &bareJid, ChatRosterModel::Feature feature) const
{
    QStringList jids;
    if (bareJid.isEmpty())
        return jids;

    if (jidToResource(bareJid).isEmpty())
    {
        const QString sought = bareJid + "/";
        foreach (const QString &key, d->clientFeatures.keys())
            if (key.startsWith(sought) && (d->clientFeatures.value(key) & feature))
                jids << key;
    } else if (d->clientFeatures.value(bareJid) & feature) {
        jids << bareJid;
    }
    return jids;
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

    int messages = item->data(MessagesRole).toInt();

    if (role == JidRole) {
        return item->jid;
    } else if (role == AvatarRole) {
        return VCardCache::instance()->imageUrl(item->jid);
    } else if (role == UrlRole) {
        return VCardCache::instance()->profileUrl(item->jid);
    } else if (role == StatusRole) {
        QXmppPresence::Status::Type statusType = QXmppPresence::Status::Offline;
        // NOTE : we test the connection status, otherwise we encounter a race
        // condition upon disconnection, because the roster has not yet been cleared
        if (!d->client->isConnected())
            return statusType;
        foreach (const QXmppPresence &presence, d->client->rosterManager().getAllPresencesForBareJid(item->jid)) {
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
    } else if (role == Qt::DisplayRole && index.column() == StatusColumn) {
        return QVariant();
    } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
        return contactStatus(index) + sortSeparator + item->data(Qt::DisplayRole).toString().toLower() + sortSeparator + item->jid.toLower();
    }

    return item->data(role);
}

void ChatRosterModel::disconnected()
{
    d->clientFeatures.clear();

    if (rootItem->children.size() > 0)
    {
        ChatModelItem *first = rootItem->children.first();
        ChatModelItem *last = rootItem->children.last();
        emit dataChanged(createIndex(first, ContactColumn),
                         createIndex(last, SortingColumn));
    }
}

void ChatRosterModel::discoveryInfoFound(const QXmppDiscoveryIq &disco)
{
    int features = 0;
    foreach (const QString &var, disco.features())
    {
        if (var == ns_chat_states)
            features |= ChatStatesFeature;
        else if (var == ns_stream_initiation_file_transfer)
            features |= FileTransferFeature;
        else if (var == ns_version)
            features |= VersionFeature;
        else if (var == ns_jingle_rtp_audio)
            features |= VoiceFeature;
        else if (var == ns_jingle_rtp_video)
            features |= VideoFeature;
    }
    ChatRosterItem *item = d->find(disco.from());
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities()) {
        if (id.name() == "iChatAgent")
            features |= ChatStatesFeature;
    }
    d->clientFeatures.insert(disco.from(), features);
}

void ChatRosterModel::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() != QXmppIq::Result)
        return;

    writeIq(d->cache, QString("xmpp:%1?disco;type=get;request=info").arg(disco.from()), disco, 3600);
    discoveryInfoFound(disco);
}

QModelIndex ChatRosterModel::findItem(const QString &bareJid, const QModelIndex &parent) const
{
    ChatRosterItem *parentItem = static_cast<ChatRosterItem*>(parent.isValid() ? parent.internalPointer() : rootItem);
    return createIndex(d->find(bareJid, parentItem), 0);
}

bool ChatRosterModel::isOwnNameReceived() const
{
    return d->nickNameReceived;
}

QString ChatRosterModel::ownName() const
{
    return d->ownItem->data(NicknameRole).toString();
}

/** Handles an item being added to the roster.
 */
void ChatRosterModel::itemAdded(const QString &jid)
{
    ChatRosterItem *item = d->find(jid);
    if (item)
        return;

    // add a new entry
    const QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(jid);
    item = new ChatRosterItem(ChatRosterModel::Contact);
    item->jid = jid;
    if (!entry.name().isEmpty())
        item->setData(Qt::DisplayRole, entry.name());
    else
        item->setData(Qt::DisplayRole, jidToUser(jid));
    ChatModel::addItem(item, rootItem);

    // fetch vCard
    d->fetchVCard(item->jid);
}

/** Handles an item being changed in the roster.
 */
void ChatRosterModel::itemChanged(const QString &jid)
{
    ChatRosterItem *item = d->find(jid);
    if (!item)
        return;

    // update an existing entry
    const QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(jid);
    if (!entry.name().isEmpty())
        item->setData(Qt::DisplayRole, entry.name());
    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));

    // fetch vCard
    d->fetchVCard(item->jid);
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

void ChatRosterModel::presenceReceived(const QXmppPresence &presence)
{
    const QString jid = presence.from();
    const QString bareJid = jidToBareJid(jid);
    const QString resource = jidToResource(jid);

    // handle features discovery
    if (!resource.isEmpty())
    {
        if (presence.type() == QXmppPresence::Unavailable)
            d->clientFeatures.remove(jid);
        else if (presence.type() == QXmppPresence::Available && !d->clientFeatures.contains(jid)) {
            QXmppDiscoveryIq disco;
            if (readIq(d->cache, QString("xmpp:%1?disco;type=get;request=info").arg(jid), &disco) &&
                (presence.capabilityVer().isEmpty() || presence.capabilityVer() == disco.verificationString()))
            {
                discoveryInfoFound(disco);
            } else {
                d->client->findExtension<QXmppDiscoveryManager>()->requestInfo(jid);
            }
        }
    }
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
    foreach (const QString &jid, d->client->rosterManager().getRosterBareJids()) {
        itemAdded(jid);
        oldJids.removeAll(jid);
    }

    // remove obsolete entries
    foreach (const QString &jid, oldJids) {
        ChatRosterItem *item = d->find(jid);
        if (item && item != d->ownItem)
            removeItem(item);
    }

    // trigger resize
    emit rosterReady();
}

bool ChatRosterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    item->setData(role, value);
    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));
    return true;
}

void ChatRosterModel::vCardReceived(const QXmppVCardIq& vcard)
{
    if (vcard.type() != QXmppIq::Result)
        return;

    const QString bareJid = vcard.from();
    ChatRosterItem *item = d->find(bareJid);
    if (!item)
        return;

    // store the nickName
    if (!vcard.nickName().isEmpty())
         item->setData(NicknameRole, vcard.nickName());

    // store the nickName or fullName found in the vCard for display,
    // unless the roster entry has a name
    QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(bareJid);
    if (entry.name().isEmpty()) {
        if (!vcard.nickName().isEmpty())
            item->setData(Qt::DisplayRole, vcard.nickName());
        else if (!vcard.fullName().isEmpty())
            item->setData(Qt::DisplayRole, vcard.fullName());
    }

    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));

    // check if we got our own name
    if (item == d->ownItem) {
        d->nickNameReceived = true;
        emit ownNameReceived();
    }
}

void ChatRosterModel::addPendingMessage(const QString &bareJid)
{
    ChatRosterItem *item = d->find(bareJid);
    if (item)
    {
        item->setData(MessagesRole, item->data(MessagesRole).toInt() + 1);
        emit dataChanged(createIndex(item, ContactColumn),
                         createIndex(item, SortingColumn));
        emit pendingMessages(d->countPendingMessages());
    }
}

void ChatRosterModel::clearPendingMessages(const QString &bareJid)
{
    ChatRosterItem *item = d->find(bareJid);
    if (item && item->data(MessagesRole).toInt())
    {
        item->setData(MessagesRole, 0);
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

QString VCard::jid() const
{
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
        if (newName.isEmpty())
            newName = jidToUser(m_jid);
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

VCardCache::VCardCache(QObject *parent)
    : QObject(parent),
    m_manager(0)
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
    if (m_manager) {
        qDebug("requesting vCard %s", qPrintable(jid));
        m_queue.insert(jid);
        m_manager->requestVCard(jid);
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

QXmppVCardManager *VCardCache::manager() const
{
    return m_manager;
}

void VCardCache::setManager(QXmppVCardManager* manager)
{
    bool check;

    if (manager == m_manager)
        return;

    Q_ASSERT(manager);
    m_manager = manager;

    check = connect(m_manager, SIGNAL(vCardReceived(QXmppVCardIq)),
                    this, SLOT(vCardReceived(QXmppVCardIq)));
    Q_ASSERT(check);

    emit managerChanged(m_manager);
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

