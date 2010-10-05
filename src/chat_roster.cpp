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

#include <QAbstractNetworkCache>
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
#include "chat_roster_item.h"

static const QChar sortSeparator('\0');

enum RosterColumns {
    ContactColumn = 0,
    ImageColumn,
    SortingColumn,
    MaxColumn,
};

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
    void fetchInfo(const QString &jid);
    void fetchVCard(const QString &jid);

    inline QModelIndex index(ChatRosterItem *item, int column)
    {
        if (item && item != rootItem)
            return q->createIndex(item->row(), column, item);
        else
            return QModelIndex();
    }

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
    QAbstractNetworkCache *cache;
    QXmppClient *client;
    ChatRosterItem *contactsItem;
    ChatRosterItem *ownItem;
    ChatRosterItem *rootItem;
    bool nickNameReceived;
    QMap<QString, int> clientFeatures;
};

/** Count the current number of pending messages.
 */
int ChatRosterModelPrivate::countPendingMessages()
{
    int pending = 0;
    for (int i = 0; i < rootItem->size(); i++)
    {
        ChatRosterItem *item = rootItem->child(i);
        pending += item->data(ChatRosterModel::MessagesRole).toInt();
    }
    return pending;
}

/** Check whether the discovery info for the given roster item is cached, otherwise
 *  request the information.
 *
 * @param item
 */
void ChatRosterModelPrivate::fetchInfo(const QString &jid)
{
    QXmppDiscoveryIq disco;
    if (readIq(QString("xmpp:%1?disco;type=get;request=info").arg(jid), disco))
        q->discoveryInfoFound(disco);
    else
        client->discoveryManager().requestInfo(jid);
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

ChatRosterModel::ChatRosterModel(QXmppClient *xmppClient, QObject *parent)
    : QAbstractItemModel(parent),
    d(new ChatRosterModelPrivate)
{
    d->q = this;

    /* get cache */
    Application *wApp = qobject_cast<Application*>(qApp);
    Q_ASSERT(wApp);
    d->cache = wApp->networkCache();

    d->client = xmppClient;
    d->nickNameReceived = false;
    d->ownItem = new ChatRosterItem(ChatRosterItem::Contact);
    d->rootItem = new ChatRosterItem(ChatRosterItem::Root);
    d->contactsItem = d->rootItem;
#if 0
    d->contactsItem = new ChatRosterItem(ChatRosterItem::Contact);
    d->rootItem->append(d->contactsItem);
#endif

    bool check;
    check = connect(d->client, SIGNAL(connected()),
                    this, SLOT(connected()));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(disconnected()),
                    this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(&d->client->discoveryManager(), SIGNAL(infoReceived(QXmppDiscoveryIq)),
                    this, SLOT(discoveryInfoReceived(QXmppDiscoveryIq)));
    Q_ASSERT(check);

    check = connect(d->client, SIGNAL(presenceReceived(QXmppPresence)),
                    this, SLOT(presenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(presenceChanged(QString, QString)),
                    this, SLOT(presenceChanged(QString, QString)));
    Q_ASSERT(check);

    check = connect(&d->client->rosterManager(), SIGNAL(rosterChanged(QString)),
                    this, SLOT(rosterChanged(QString)));
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
    delete d->rootItem;
    delete d;
}

int ChatRosterModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MaxColumn;
}

void ChatRosterModel::connected()
{
    /* request own vCard */
    d->nickNameReceived = false;
    d->ownItem->setId(d->client->configuration().jidBare());
    d->ownItem->setData(NicknameRole, d->client->configuration().user());
    d->fetchVCard(d->ownItem->id());
}

QPixmap ChatRosterModel::contactAvatar(const QString &bareJid) const
{
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
        return item->data(AvatarRole).value<QPixmap>();
    return QPixmap();
}

/** Returns the full JID of an online contact which has the requested feature.
 *
 * @param bareJid
 * @param feature
 */
QStringList ChatRosterModel::contactFeaturing(const QString &bareJid, ChatRosterModel::Feature feature) const
{
    QStringList jids;
    const QString sought = bareJid + "/";
    foreach (const QString &key, d->clientFeatures.keys())
        if (key.startsWith(sought) && (d->clientFeatures.value(key) & feature))
            jids << key;
    return jids;
}

/** Determine extra information for a contact.
 *
 * @param bareJid
 */
QString ChatRosterModel::contactExtra(const QString &bareJid) const
{
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (!item)
        return QString();

    const QString remoteDomain = jidToDomain(bareJid);
    if (d->client->configuration().domain() == "wifirst.net" &&
        remoteDomain == "wifirst.net")
    {
        // for wifirst accounts, return the wifirst nickname if it is
        // different from the display name
        const QString nickName = item->data(NicknameRole).toString();
        if (nickName != item->data(Qt::DisplayRole).toString())
            return nickName;
        else
            return QString();
    } else {
        // for other accounts, return the JID
        return bareJid;
    }
}

/** Determine the display name for a contact.
 *
 *  If the user has set a name for the roster entry, it will be used,
 *  otherwise we fall back to information from the vCard.
 *
 * @param bareJid
 */
QString ChatRosterModel::contactName(const QString &bareJid) const
{
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
        return item->data(Qt::DisplayRole).toString();
    return jidToUser(bareJid);
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
    } else if (role == StatusRole && item->type() == ChatRosterItem::Contact) {
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
    } else if (role == Qt::DisplayRole && index.column() == ImageColumn) {
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
        if (item->type() == ChatRosterItem::Contact)
        {
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap icon(QString(":/contact-%1.png").arg(contactStatus(index)));
                if (messages)
                    paintMessages(icon, messages);
                return icon;
            } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
                return QIcon(contactAvatar(bareJid));
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return contactStatus(index) + sortSeparator + item->data(Qt::DisplayRole).toString().toLower() + sortSeparator + bareJid.toLower();
            }
        } else if (item->type() == ChatRosterItem::Room) {
            if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                QPixmap icon(":/chat.png");
                if (messages)
                    paintMessages(icon, messages);
                return icon;
            } else if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return QLatin1String("chatroom") + sortSeparator + bareJid.toLower();
            }
        } else if (item->type() == ChatRosterItem::RoomMember) {
            if (role == Qt::DisplayRole && index.column() == SortingColumn) {
                return QLatin1String("chatuser") + sortSeparator + contactStatus(index) + sortSeparator + bareJid.toLower();
            } else if (role == Qt::DecorationRole && index.column() == ContactColumn) {
                return QIcon(QString(":/contact-%1.png").arg(contactStatus(index)));
            } else if (role == Qt::DecorationRole && index.column() == ImageColumn) {
                return QIcon(contactAvatar(bareJid));
            }
        }
    }
    if (role == Qt::DecorationRole && index.column() == ImageColumn)
        return QVariant();

    return item->data(role);
}

void ChatRosterModel::disconnected()
{
    d->clientFeatures.clear();

    if (d->rootItem->size() > 0)
    {
        ChatRosterItem *first = d->rootItem->child(0);
        ChatRosterItem *last = d->rootItem->child(d->rootItem->size() - 1);
        emit dataChanged(createIndex(first->row(), ContactColumn, first),
                         createIndex(last->row(), SortingColumn, last));
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
    }
    ChatRosterItem *item = d->rootItem->find(disco.from());
    foreach (const QXmppDiscoveryIq::Identity& id, disco.identities())
    {
        if (id.name() == "iChatAgent")
            features |= ChatStatesFeature;
        if (item && item->type() == ChatRosterItem::Room &&
            id.category() == "conference")
        {
            item->setData(Qt::DisplayRole, id.name());
            emit dataChanged(createIndex(item->row(), ContactColumn, item),
                             createIndex(item->row(), SortingColumn, item));
        }
    }
    if (d->clientFeatures.contains(disco.from()))
        d->clientFeatures.insert(disco.from(), features);
}

void ChatRosterModel::discoveryInfoReceived(const QXmppDiscoveryIq &disco)
{
    if (disco.type() != QXmppIq::Result)
        return;

    d->writeIq(QString("xmpp:%1?disco;type=get;request=info").arg(disco.from()), disco, 600);
    discoveryInfoFound(disco);
}

QModelIndex ChatRosterModel::findItem(const QString &bareJid) const
{
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
        return createIndex(item->row(), 0, item);
    else
        return QModelIndex();
}

Qt::ItemFlags ChatRosterModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (!index.isValid())
        return defaultFlags;

    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    if (item->type() == ChatRosterItem::Contact)
        return Qt::ItemIsDragEnabled | defaultFlags;
    else
        return defaultFlags;
}

QModelIndex ChatRosterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());

    ChatRosterItem *childItem = parentItem->child(row);
    return d->index(childItem, column);
}

bool ChatRosterModel::isOwnNameReceived() const
{
    return d->nickNameReceived;
}

QString ChatRosterModel::ownName() const
{
    return d->ownItem->data(NicknameRole).toString();
}

QModelIndex ChatRosterModel::parent(const QModelIndex & index) const
{
    if (!index.isValid())
        return QModelIndex();

    ChatRosterItem *childItem = static_cast<ChatRosterItem*>(index.internalPointer());
    return d->index(childItem->parent(), 0);
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

void ChatRosterModel::presenceChanged(const QString& bareJid, const QString& resource)
{
    Q_UNUSED(resource);
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
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
        else if (presence.type() == QXmppPresence::Available && !d->clientFeatures.contains(jid))
        {
            // discover remote party features
            d->clientFeatures.insert(jid, 0);
            d->fetchInfo(jid);
        }
    }

    // handle chat rooms
    ChatRosterItem *roomItem = d->rootItem->find(bareJid);
    if (!roomItem || roomItem->type() != ChatRosterItem::Room)
        return;

    ChatRosterItem *memberItem = roomItem->find(jid);
    if (presence.type() == QXmppPresence::Available)
    {
        if (!memberItem)
        {
            // create roster entry
            memberItem = new ChatRosterItem(ChatRosterItem::RoomMember);
            memberItem->setId(jid);
            memberItem->setData(StatusRole, presence.status().type());
            beginInsertRows(d->index(roomItem, 0), roomItem->size(), roomItem->size());
            roomItem->append(memberItem);
            endInsertRows();

            // fetch vCard
            d->fetchVCard(memberItem->id());
        } else {
            // update roster entry
            memberItem->setData(StatusRole, presence.status().type());
            emit dataChanged(createIndex(memberItem->row(), ContactColumn, memberItem),
                             createIndex(memberItem->row(), SortingColumn, memberItem));
        }

        // check whether we own the room
        foreach (const QXmppElement &x, presence.extensions())
        {
            if (x.tagName() == "x" && x.attribute("xmlns") == ns_muc_user)
            {
                QXmppElement item = x.firstChildElement("item");
                if (item.attribute("jid") == d->client->configuration().jid())
                {
                    int flags = 0;
                    // role
                    if (item.attribute("role") == "moderator")
                        flags |= (KickFlag | SubjectFlag);
                    // affiliation
                    if (item.attribute("affiliation") == "owner")
                        flags |= (OptionsFlag | MembersFlag | SubjectFlag);
                    else if (item.attribute("affiliation") == "admin")
                        flags |= (MembersFlag | SubjectFlag);
                    roomItem->setData(FlagsRole, flags);
                }
            }
        }
    }
    else if (presence.type() == QXmppPresence::Unavailable && memberItem)
    {
        beginRemoveRows(d->index(roomItem, 0), memberItem->row(), memberItem->row());
        roomItem->remove(memberItem);
        endRemoveRows();
    }
}

void ChatRosterModel::rosterChanged(const QString &jid)
{
    ChatRosterItem *item = d->contactsItem->find(jid);
    QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(jid);

    // remove an existing entry
    QModelIndex contactsIndex = d->index(d->contactsItem, 0);
    if (entry.subscriptionType() == QXmppRosterIq::Item::Remove)
    {
        if (item)
        {
            beginRemoveRows(contactsIndex, item->row(), item->row());
            d->contactsItem->remove(item);
            endRemoveRows();
        }
        return;
    }

    if (item)
    {
        // update an existing entry
        if (!entry.name().isEmpty())
            item->setData(Qt::DisplayRole, entry.name());
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    } else {
        // add a new entry
        item = new ChatRosterItem(ChatRosterItem::Contact);
        item->setId(jid);
        if (!entry.name().isEmpty())
            item->setData(Qt::DisplayRole, entry.name());
        beginInsertRows(contactsIndex, d->contactsItem->size(), d->contactsItem->size());
        d->contactsItem->append(item);
        endInsertRows();
    }

    // fetch vCard
    d->fetchVCard(item->id());
}

void ChatRosterModel::rosterReceived()
{
    // make a note of existing contacts
    QStringList oldJids;
    for (int i = 0; i < d->contactsItem->size(); i++)
    {
        ChatRosterItem *child = d->contactsItem->child(i);
        if (child->type() == ChatRosterItem::Contact)
            oldJids << child->id();
    }

    // process received entries
    foreach (const QString &jid, d->client->rosterManager().getRosterBareJids())
    {
        rosterChanged(jid);
        oldJids.removeAll(jid);
    }

    // remove obsolete entries
    QModelIndex contactsIndex = d->index(d->contactsItem, 0);
    foreach (const QString &jid, oldJids)
    {
        ChatRosterItem *item = d->contactsItem->find(jid);
        if (item)
        {
            beginRemoveRows(contactsIndex, item->row(), item->row());
            d->rootItem->remove(item);
            endRemoveRows();
        }
    }

    // trigger resize
    emit rosterReady();
}

bool ChatRosterModel::removeRows(int row, int count, const QModelIndex &parent)
{
    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());

    const int minIndex = qMax(0, row);
    const int maxIndex = qMin(row + count, parentItem->size()) - 1;
    beginRemoveRows(parent, minIndex, maxIndex);
    for (int i = maxIndex; i >= minIndex; --i)
        parentItem->removeAt(i);
    endRemoveRows();
    return true;
}

int ChatRosterModel::rowCount(const QModelIndex &parent) const
{
    ChatRosterItem *parentItem;
    if (!parent.isValid())
        parentItem = d->rootItem;
    else
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());
    return parentItem->size();
}

bool ChatRosterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());
    item->setData(role, value);
    emit dataChanged(createIndex(item->row(), ContactColumn, item),
                     createIndex(item->row(), SortingColumn, item));
    return true;
}

void ChatRosterModel::vCardFound(const QXmppVCardIq& vcard)
{
    const QString bareJid = vcard.from();

    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
    {
        // read vCard image
        QBuffer buffer;
        buffer.setData(vcard.photo());
        buffer.open(QIODevice::ReadOnly);
        QImageReader imageReader(&buffer);
        item->setData(AvatarRole, QPixmap::fromImage(imageReader.read()));

        // store the nickName or fullName found in the vCard for display,
        // unless the roster entry has a name
        if (item->type() == ChatRosterItem::Contact)
        {
            QXmppRosterIq::Item entry = d->client->rosterManager().getRosterEntry(bareJid);
            if (!vcard.nickName().isEmpty())
                item->setData(NicknameRole, vcard.nickName());
            if (entry.name().isEmpty())
            {
                if (!vcard.nickName().isEmpty())
                    item->setData(Qt::DisplayRole, vcard.nickName());
                else if (!vcard.fullName().isEmpty())
                    item->setData(Qt::DisplayRole, vcard.fullName());
            }
        }

        // store vCard URL
        const QString url = vcard.url();
        if (!url.isEmpty())
            item->setData(UrlRole, vcard.url());

        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
    }
    if (bareJid == d->client->configuration().jidBare())
    {
        if (!vcard.nickName().isEmpty())
            d->ownItem->setData(NicknameRole, vcard.nickName());
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
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
    {
        item->setData(MessagesRole, item->data(MessagesRole).toInt() + 1);
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
        emit pendingMessages(d->countPendingMessages());
    }
}

QModelIndex ChatRosterModel::addItem(ChatRosterItem::Type type, const QString &id, const QString &name, const QIcon &icon, const QModelIndex &parent)
{
    ChatRosterItem *parentItem;
    if (parent.isValid())
        parentItem = static_cast<ChatRosterItem*>(parent.internalPointer());
    else
        parentItem = d->rootItem;

    // check the item does not already exist
    ChatRosterItem *item = parentItem->find(id);
    if (item)
        return createIndex(item->row(), 0, item);

    // prepare item
    item = new ChatRosterItem(type);
    item->setId(id);
    if (!name.isEmpty())
        item->setData(Qt::DisplayRole, name);
    if (!icon.isNull())
        item->setData(Qt::DecorationRole, icon);

    // add item
    beginInsertRows(parent, parentItem->size(), parentItem->size());
    parentItem->append(item);
    endInsertRows();
    return createIndex(item->row(), 0, item);
}

void ChatRosterModel::clearPendingMessages(const QString &bareJid)
{
    ChatRosterItem *item = d->rootItem->find(bareJid);
    if (item)
    {
        item->setData(MessagesRole, 0);
        emit dataChanged(createIndex(item->row(), ContactColumn, item),
                         createIndex(item->row(), SortingColumn, item));
        emit pendingMessages(d->countPendingMessages());
    }
}

/** Move an item to the given parent.
 *
 * @param index
 * @param newParent
 */
QModelIndex ChatRosterModel::reparentItem(const QModelIndex &index, const QModelIndex &newParent)
{
    if (!index.isValid())
        return index;
    ChatRosterItem *item = static_cast<ChatRosterItem*>(index.internalPointer());

    /* determine requested parent item */
    ChatRosterItem *newParentItem;
    if (!newParent.isValid())
        newParentItem = d->rootItem;
    else
        newParentItem = static_cast<ChatRosterItem*>(newParent.internalPointer());

    /* check if we need to do anything */
    ChatRosterItem *currentParentItem = item->parent();
    if (currentParentItem == newParentItem)
        return index;

    /* determine current parent index */
    QModelIndex currentParent;
    if (currentParentItem != d->rootItem)
        currentParent = createIndex(currentParentItem->row(), 0, currentParentItem);

    /* move item */
    beginMoveRows(currentParent, item->row(), item->row(),
                  newParent, newParentItem->size());
    item->setParent(newParentItem);
    endMoveRows();

    return createIndex(item->row(), 0, item);
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
    setColumnWidth(ImageColumn, 40);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setAcceptDrops(true);
    setAnimated(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragEnabled(true);
    setDropIndicatorShown(false);
    setHeaderHidden(true);
    setIconSize(QSize(32, 32));
    setMinimumWidth(200);
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(SortingColumn, Qt::AscendingOrder);
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

void ChatRosterView::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void ChatRosterView::dragMoveEvent(QDragMoveEvent *event)
{
    // ignore by default
    event->ignore();

    // let plugins process event
    QModelIndex index = indexAt(event->pos());
    if (index.isValid())
        emit itemDrop(event, index);
}

void ChatRosterView::dropEvent(QDropEvent *event)
{
    // ignore by default
    event->ignore();

    // let plugins process event
    QModelIndex index = indexAt(event->pos());
    if (index.isValid())
        emit itemDrop(event, index);
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
    setColumnWidth(ContactColumn, e->size().width() - 40);
}

void ChatRosterView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    foreach (const QModelIndex &index, selected.indexes())
        expand(index);
}

void ChatRosterView::setShowOfflineContacts(bool show)
{
    if (show)
        sortedModel->setFilterRegExp(QRegExp());
    else
        sortedModel->setFilterRegExp(QRegExp("^(?!offline).+"));
}

QSize ChatRosterView::sizeHint () const
{
    if (!model()->rowCount())
        return QTreeView::sizeHint();

    QSize hint(200, 0);
    hint.setHeight(model()->rowCount() * sizeHintForRow(0));
    return hint;
}

