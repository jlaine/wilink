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
#include <QContextMenuEvent>
#include <QDebug>
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
#include <QSortFilterProxyModel>
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

#ifdef WILINK_EMBEDDED
#define FLAT_CONTACTS
#define ICON_SIZE 32
#else
#define ICON_SIZE 24
#endif

static const QChar sortSeparator('\0');

enum RosterColumns {
    ContactColumn = 0,
    StatusColumn,
    SortingColumn,
    MaxColumn,
};

class ChatRosterItem : public ChatModelItem
{
public:
    ChatRosterItem(ChatRosterModel::Type type);

    QVariant data(int role) const;
    void setData(int role, const QVariant &value);

    QString id() const;
    void setId(const QString &id);

    ChatRosterModel::Type type() const;

private:
    QString itemId;
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

QString ChatRosterItem::id() const
{
    return itemId;
}

void ChatRosterItem::setId(const QString &id)
{
    itemId = id;
}

void ChatRosterItem::setData(int role, const QVariant &value)
{
    itemData.insert(role, value);
}

enum ChatRosterModel::Type ChatRosterItem::type() const
{
    return itemType;
}

ChatRosterImageProvider::ChatRosterImageProvider()
    : QDeclarativeImageProvider(Pixmap),
    m_rosterModel(0)
{
}

QPixmap ChatRosterImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_ASSERT(m_rosterModel);

    // exact roster entry or by bare jid
    QModelIndex index = m_rosterModel->findItem(id);
    if (!index.isValid())
        index = m_rosterModel->findItem(jidToBareJid(id));
    QPixmap pixmap;
    if (index.isValid()) {
        ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
        pixmap = item->data(Qt::DecorationRole).value<QPixmap>();
    }
    if (pixmap.isNull()) {
        qWarning("Could not get roster picture for %s", qPrintable(id));
        pixmap = QPixmap(":/peer.png");
    }

    // scale
    if (size)
        *size = pixmap.size();
    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);
    else
        return pixmap;
}

void ChatRosterImageProvider::setRosterModel(ChatRosterModel *rosterModel)
{
    m_rosterModel = rosterModel;
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
    ChatRosterItem* find(const QString &id, ChatRosterItem *parent = 0);

    // Try to read an IQ to disk cache.
    template <class T>
    bool readIq(const QUrl &url, T &iq)
    {
        QIODevice *ioDevice = cache->data(url);
        if (!ioDevice)
            return false;

        QDomDocument doc;
        doc.setContent(ioDevice);
        iq.parse(doc.documentElement());
        delete ioDevice;
        return true;
    }

    // Write an IQ to disk cache.
    template <class T>
    void writeIq(const QUrl &url, const T &iq, int cacheSeconds)
    {
        QNetworkCacheMetaData metaData;
        metaData.setUrl(url);
        metaData.setExpirationDate(QDateTime::currentDateTime().addSecs(cacheSeconds));
        QIODevice *ioDevice = cache->prepare(metaData);
        QXmlStreamWriter writer(ioDevice);
        iq.toXml(&writer);
        cache->insert(ioDevice);
    }

    ChatRosterModel *q;
    QNetworkDiskCache *cache;
    QXmppClient *client;
    ChatRosterItem *contactsItem;
    ChatRosterItem *ownItem;
    ChatRosterItem *roomsItem;
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
    if (readIq(QString("xmpp:%1?vcard").arg(jid), vCard))
        q->vCardFound(vCard);
    else
        client->vCardManager().requestVCard(jid);
}

ChatRosterItem *ChatRosterModelPrivate::find(const QString &id, ChatRosterItem *parent)
{
    if (!parent)
        parent = static_cast<ChatRosterItem*>(q->rootItem);

    /* look at immediate children */
    foreach (ChatModelItem *it, parent->children) {
        ChatRosterItem *item = static_cast<ChatRosterItem*>(it);
        if (item->id() == id)
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

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(AvatarRole, "avatar");
    roleNames.insert(IdRole, "id");
    roleNames.insert(Qt::DisplayRole, "name");
    roleNames.insert(UrlRole, "url");
    setRoleNames(roleNames);

    /* get cache */
    d->cache = new QNetworkDiskCache(this);
    d->cache->setCacheDirectory(wApp->cacheDirectory());

    d->client = xmppClient;
    d->nickNameReceived = false;
    rootItem = new ChatRosterItem(ChatRosterModel::Other);
    d->ownItem = new ChatRosterItem(ChatRosterModel::Contact);
    ChatModel::addItem(d->ownItem, rootItem);
#ifdef FLAT_CONTACTS
    d->contactsItem = (ChatRosterItem*)rootItem;
#else
    d->contactsItem = new ChatRosterItem(ChatRosterModel::Other);
    d->contactsItem->setId(CONTACTS_ROSTER_ID);
    d->contactsItem->setData(Qt::DisplayRole, tr("My contacts"));
    d->contactsItem->setData(Qt::DecorationRole, QIcon(":/peer.png"));
    ChatModel::addItem(d->contactsItem, rootItem);
#endif
    d->roomsItem = 0;

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
    d->ownItem->setId(d->client->configuration().jidBare());
    d->ownItem->setData(Qt::DisplayRole, d->client->configuration().user());
    d->ownItem->setData(NicknameRole, d->client->configuration().user());
    d->fetchVCard(d->ownItem->id());
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

    ChatRosterItem *item = d->find(bareJid);
    if (item && item->type() != ChatRosterModel::Contact && item->type() != ChatRosterModel::RoomMember)
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

/** Determine the display name for a contact.
 *
 *  If the user has set a name for the roster entry, it will be used,
 *  otherwise we fall back to information from the vCard.
 *
 * @param jid
 */
QString ChatRosterModel::contactName(const QString &jid) const
{
    // exact roster entry
    ChatRosterItem *item = d->find(jid);
    if (item)
        return item->data(Qt::DisplayRole).toString();

    // contact by bare jid
    item = d->find(jidToBareJid(jid));
    if (item)
        return item->type() == ChatRosterModel::Room ? jidToResource(jid) : item->data(Qt::DisplayRole).toString();

    return jid;
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

    QString bareJid = item->id();
    int messages = item->data(MessagesRole).toInt();

    if (role == IdRole) {
        return bareJid;
    } else if (role == TypeRole) {
        return item->type();
    } else if (role == AvatarRole) {
        if (item->data(Qt::DecorationRole).isNull())
            return QUrl("qrc:/peer.png");
        return QUrl("image://roster/" + bareJid);
    } else if (role == StatusRole && item->type() == ChatRosterModel::Contact) {
        QXmppPresence::Status::Type statusType = QXmppPresence::Status::Offline;
        // NOTE : we test the connection status, otherwise we encounter a race
        // condition upon disconnection, because the roster has not yet been cleared
        if (!d->client->isConnected())
            return statusType;
        foreach (const QXmppPresence &presence, d->client->rosterManager().getAllPresencesForBareJid(bareJid))
        {
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
    } else if(role == Qt::FontRole && index.column() == ContactColumn) {
        if (messages)
            return QFont("", -1, QFont::Bold, true);
    } else if(role == Qt::BackgroundRole && index.column() == ContactColumn) {
        if (messages)
        {
            QLinearGradient grad(QPointF(0, 0), QPointF(0.8, 0));
            grad.setColorAt(0, QColor(255, 0, 0, 144));
            grad.setColorAt(1, Qt::transparent);
            grad.setCoordinateMode(QGradient::ObjectBoundingMode);
            return QBrush(grad);
        }
    } else {
        if (item->type() == ChatRosterModel::Contact || item->type() == ChatRosterModel::RoomMember)
        {
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap pixmap = item->data(role).value<QPixmap>();
                if (pixmap.isNull())
                    pixmap = QPixmap(":/peer.png");
                if (messages)
                    paintMessages(pixmap, messages);
                return QIcon(pixmap);
            } else if (role == Qt::DecorationRole && index.column() == StatusColumn) {
                return QIcon(QString(":/contact-%1.png").arg(contactStatus(index)));
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return contactStatus(index) + sortSeparator + item->data(Qt::DisplayRole).toString().toLower() + sortSeparator + bareJid.toLower();
            }
        } else if (item->type() == ChatRosterModel::Room) {
            if (role == Qt::DisplayRole && index.column() == ContactColumn && item->children.size() > 0) {
                return QString("%1 (%2)").arg(item->data(role).toString(), QString::number(item->children.size()));
            } else if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap icon(item->data(role).value<QPixmap>());
                if (messages)
                    paintMessages(icon, messages);
                return QIcon(icon);
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return QLatin1String("chatroom") + QString::number(!item->data(PersistentRole).toInt()) + sortSeparator + bareJid.toLower();
            }
        } else {
            if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return bareJid;
            }
        }
    }
    if (role == Qt::DecorationRole && index.column() == StatusColumn)
        return QVariant();

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
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities())
    {
        if (id.name() == "iChatAgent")
            features |= ChatStatesFeature;
        if (item && item->type() == ChatRosterModel::Room &&
            id.category() == "conference")
        {
            item->setData(Qt::DisplayRole, id.name());
            emit dataChanged(createIndex(item, ContactColumn),
                             createIndex(item, SortingColumn));
        }
    }
    d->clientFeatures.insert(disco.from(), features);
}

void ChatRosterModel::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() != QXmppIq::Result)
        return;

    d->writeIq(QString("xmpp:%1?disco;type=get;request=info").arg(disco.from()), disco, 3600);
    discoveryInfoFound(disco);
}

QModelIndex ChatRosterModel::contactsItem() const
{
    return createIndex(d->contactsItem, 0);
}

QModelIndex ChatRosterModel::findItem(const QString &bareJid, const QModelIndex &parent) const
{
    ChatRosterItem *parentItem = static_cast<ChatRosterItem*>(parent.isValid() ? parent.internalPointer() : rootItem);
    return createIndex(d->find(bareJid, parentItem), 0);
}

Qt::ItemFlags ChatRosterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return defaultFlags;

    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (item->type() == ChatRosterModel::Contact)
        return Qt::ItemIsDragEnabled | defaultFlags;
    else
        return defaultFlags;
}

bool ChatRosterModel::isOwnNameReceived() const
{
    return d->nickNameReceived;
}

QString ChatRosterModel::ownName() const
{
    return d->ownItem->data(NicknameRole).toString();
}

QMimeData *ChatRosterModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> urls;
    foreach (QModelIndex index, indexes)
        if (index.isValid() && index.column() == ContactColumn)
            urls << QUrl(index.data(ChatRosterModel::IdRole).toString());

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    return mimeData;
}

QStringList ChatRosterModel::mimeTypes() const
{
    return QStringList() << "text/uri-list";
}

/** Handles an item being added to the roster.
 */
void ChatRosterModel::itemAdded(const QString &jid)
{
    ChatRosterItem *item = d->find(jid, d->contactsItem);
    if (item)
        return;

    // add a new entry
    const QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(jid);
    item = new ChatRosterItem(ChatRosterModel::Contact);
    item->setId(jid);
    if (!entry.name().isEmpty())
        item->setData(Qt::DisplayRole, entry.name());
    else
        item->setData(Qt::DisplayRole, jidToUser(jid));
    ChatModel::addItem(item, d->contactsItem);

    // fetch vCard
    d->fetchVCard(item->id());
}

/** Handles an item being changed in the roster.
 */
void ChatRosterModel::itemChanged(const QString &jid)
{
    ChatRosterItem *item = d->find(jid, d->contactsItem);
    if (!item)
        return;

    // update an existing entry
    const QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(jid);
    if (!entry.name().isEmpty())
        item->setData(Qt::DisplayRole, entry.name());
    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));

    // fetch vCard
    d->fetchVCard(item->id());
}

/** Handles an item being removed from the roster.
 */
void ChatRosterModel::itemRemoved(const QString &jid)
{
    ChatRosterItem *item = d->find(jid, d->contactsItem);
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
            if (d->readIq(QString("xmpp:%1?disco;type=get;request=info").arg(jid), disco) &&
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
    foreach (ChatModelItem *item, d->contactsItem->children) {
        ChatRosterItem *child = static_cast<ChatRosterItem*>(item);
        if (child->type() == ChatRosterModel::Contact)
            oldJids << child->id();
    }

    // process received entries
    foreach (const QString &jid, d->client->rosterManager().getRosterBareJids())
    {
        itemAdded(jid);
        oldJids.removeAll(jid);
    }

    // remove obsolete entries
    foreach (const QString &jid, oldJids)
    {
        ChatRosterItem *item = d->find(jid, d->contactsItem);
        if (item)
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

void ChatRosterModel::vCardFound(const QXmppVCardIq& vcard)
{
    const QString bareJid = vcard.from();

    ChatRosterItem *item = d->find(bareJid);
    if (!item)
        return;

    // read vCard image
    QBuffer buffer;
    buffer.setData(vcard.photo());
    buffer.open(QIODevice::ReadOnly);
    QImageReader imageReader(&buffer);
    imageReader.setScaledSize(QSize(32, 32));
    item->setData(Qt::DecorationRole, QPixmap::fromImage(imageReader.read()));

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

    // store vCard URL
    const QString url = vcard.url();
    if (!url.isEmpty())
        item->setData(UrlRole, vcard.url());

    emit dataChanged(createIndex(item, ContactColumn),
                     createIndex(item, SortingColumn));

    // check if we got our own name
    if (item == d->ownItem) {
        d->nickNameReceived = true;
        emit ownNameReceived();
    }
}

void ChatRosterModel::vCardReceived(const QXmppVCardIq& vCard)
{
    if (vCard.type() != QXmppIq::Result)
        return;

    d->writeIq(QString("xmpp:%1?vcard").arg(vCard.from()), vCard, 3600);
    vCardFound(vCard);
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

QModelIndex ChatRosterModel::addItem(ChatRosterModel::Type type, const QString &id, const QString &name, const QPixmap &pixmap, const QModelIndex &reqParent)
{
    ChatModelItem *parentItem;
    if (reqParent.isValid())
        parentItem = static_cast<ChatRosterItem*>(reqParent.internalPointer());
    else
        parentItem = rootItem;

    // check the item does not already exist
    ChatRosterItem *item = d->find(id);
    if (item)
        return createIndex(item, 0);

    // add item
    item = new ChatRosterItem(type);
    item->setId(id);
    if (!name.isEmpty())
        item->setData(Qt::DisplayRole, name);
    if (!pixmap.isNull())
        item->setData(Qt::DecorationRole, pixmap);

    ChatModel::addItem(item, parentItem);

    // fetch vCard
    if (type == ChatRosterModel::Contact || type == ChatRosterModel::RoomMember)
        d->fetchVCard(item->id());

    return createIndex(item, 0);
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

ChatRosterProxyModel::ChatRosterProxyModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

QModelIndex ChatRosterProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (sourceIndex.parent() != sourceRoot())
        return QModelIndex();

    return createIndex(sourceIndex.row(), sourceIndex.column(), 0);
}

QModelIndex ChatRosterProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();

    return sourceModel()->index(proxyIndex.row(), proxyIndex.column(), sourceRoot());
}

int ChatRosterProxyModel::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

QVariant ChatRosterProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::CheckStateRole && index.isValid() && !index.column())
    {
        const QString jid = index.data(ChatRosterModel::IdRole).toString();
        return m_selection.contains(jid) ? Qt::Checked : Qt::Unchecked;
    } else {
        return QAbstractProxyModel::data(index, role);
    }
}

Qt::ItemFlags ChatRosterProxyModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = sourceModel()->flags(mapToSource(index));
    flags |= Qt::ItemIsUserCheckable;
    return flags;
}

QModelIndex ChatRosterProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column, 0);
}

void ChatRosterProxyModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    const QModelIndex root = sourceRoot();
    if (topLeft.parent() == root && bottomRight.parent() == root)
        emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight));
}

void ChatRosterProxyModel::onRowsAboutToBeInserted(const QModelIndex &sourceIndex, int start, int end)
{
    if (sourceIndex == sourceRoot())
        beginInsertRows(QModelIndex(), start, end);
}

void ChatRosterProxyModel::onRowsInserted(const QModelIndex &sourceIndex, int start, int end)
{
    if (sourceIndex == sourceRoot())
        endInsertRows();
}

void ChatRosterProxyModel::onRowsAboutToBeRemoved(const QModelIndex &sourceIndex, int start, int end)
{
    if (sourceIndex == sourceRoot())
        beginRemoveRows(QModelIndex(), start, end);
}

void ChatRosterProxyModel::onRowsRemoved(const QModelIndex &sourceIndex, int start, int end)
{
    if (sourceIndex == sourceRoot())
        endRemoveRows();
}

QModelIndex ChatRosterProxyModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int ChatRosterProxyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return sourceModel()->rowCount(sourceRoot());
}

bool ChatRosterProxyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.isValid() && !index.column())
    {
        const QString jid = index.data(ChatRosterModel::IdRole).toString();
        if (value.toInt() == Qt::Checked)
            m_selection += jid;
        else
            m_selection -= jid;
        return true;
    } else {
        return QAbstractProxyModel::setData(index, value, role);
    }
}

QModelIndex ChatRosterProxyModel::sourceRoot() const
{
    return sourceModel()->index(m_sourceRoot.row(), m_sourceRoot.column(), m_sourceRoot.parent());
}

void ChatRosterProxyModel::setSourceModel(QAbstractItemModel *rosterModel)
{
    bool check;
    check = connect(rosterModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                    this, SLOT(onDataChanged(QModelIndex,QModelIndex)));
    Q_ASSERT(check);
    check = connect(rosterModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)),
                    this, SLOT(onRowsAboutToBeInserted(QModelIndex,int,int)));
    Q_ASSERT(check);
    check = connect(rosterModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                    this, SLOT(onRowsInserted(QModelIndex,int,int)));
    Q_ASSERT(check);
    check = connect(rosterModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                    this, SLOT(onRowsAboutToBeRemoved(QModelIndex,int,int)));
    Q_ASSERT(check);
    check = connect(rosterModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                    this, SLOT(onRowsRemoved(QModelIndex,int,int)));
    Q_ASSERT(check);

    QAbstractProxyModel::setSourceModel(rosterModel);
}

void ChatRosterProxyModel::setSourceRoot(const QModelIndex &index)
{
    m_sourceRoot = index;
}

QStringList ChatRosterProxyModel::selectedJids() const
{
    return m_selection.toList();
}

ChatRosterView::ChatRosterView(ChatRosterModel *model, QWidget *parent)
    : QTreeView(parent), rosterModel(model)
{
    sortedModel = new QSortFilterProxyModel(this);
    sortedModel->setSourceModel(model);
    sortedModel->setDynamicSortFilter(true);
    sortedModel->setFilterKeyColumn(SortingColumn);
    setModel(sortedModel);

    setAlternatingRowColors(true);
    setColumnHidden(SortingColumn, true);
    setColumnWidth(StatusColumn, ICON_SIZE + 8);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setAcceptDrops(true);
    setAnimated(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setDropIndicatorShown(false);
    setHeaderHidden(true);
    setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    setMinimumHeight(400);
#ifdef FLAT_CONTACTS
    setMinimumWidth(200);
    setRootIsDecorated(false);
#else
    setMinimumWidth(250);
#endif
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(SortingColumn, Qt::AscendingOrder);

    // expand contacts
    setExpanded(CONTACTS_ROSTER_ID, true);
}

void ChatRosterView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    // allow plugins to populate menu
    QMenu *menu = new QMenu(this);
    emit itemMenu(menu, index);

    // FIXME : is there a better way to test if a menu is empty?
    if (menu->sizeHint().height() > 4)
        menu->popup(event->globalPos());
    else
        delete menu;
}

/** Map an index from the ChatRosterModel to the sorted / filtered model.
 *
 * @param index
 */
QModelIndex ChatRosterView::mapFromRoster(const QModelIndex &index)
{
    return sortedModel->mapFromSource(index);
}

void ChatRosterView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    setColumnWidth(ContactColumn, e->size().width() - ICON_SIZE - 8);
}

void ChatRosterView::setShowOfflineContacts(bool show)
{
    if (show)
        sortedModel->setFilterRegExp(QRegExp());
    else
        sortedModel->setFilterRegExp(QRegExp("^(?!offline).+"));
}

void ChatRosterView::setExpanded(const QString &id, bool expanded)
{
    QModelIndex rosterIndex = rosterModel->findItem(id);
    if (rosterIndex.isValid())
        QTreeView::setExpanded(sortedModel->mapFromSource(rosterIndex), expanded);
}

QSize ChatRosterView::sizeHint () const
{
    if (!model()->rowCount())
        return QTreeView::sizeHint();

    QSize hint(minimumWidth(), minimumHeight());
    int rowCount = sortedModel->rowCount();
#ifndef FLAT_CONTACTS
    rowCount += sortedModel->rowCount(sortedModel->mapFromSource(rosterModel->contactsItem()));
#endif
    int rowHeight = rowCount * sizeHintForRow(0);
    if (rowHeight > hint.height())
        hint.setHeight(rowHeight);
    return hint;
}

