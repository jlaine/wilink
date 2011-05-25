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

#include <QDesktopServices>
#include <QSettings>

#include "QDjango.h"
#include "QXmppClient.h"
#include "QXmppShareExtension.h"
#include "QXmppShareDatabase.h"

#include "model.h"
#include "chat_client.h"
#include "chat_utils.h"

// common queries
#define Q ShareModelQuery
#define Q_FIND_LOCATIONS(locations)  Q(QXmppShareItem::LocationsRole, Q::Equals, QVariant::fromValue(locations))

static QXmppShareDatabase *globalDatabase = 0;
static int globalDatabaseRefs = 0;

static QXmppShareDatabase *refDatabase()
{
    /* initialise database */
    if (!globalDatabase)
    {
        const QString databaseName = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).filePath("database.sqlite");
        QSqlDatabase sharesDb = QSqlDatabase::addDatabase("QSQLITE");
        sharesDb.setDatabaseName(databaseName);
        Q_ASSERT(sharesDb.open());
        QDjango::setDatabase(sharesDb);
        // drop wiLink <= 0.9.4 table
        sharesDb.exec("DROP TABLE files");

        // sanitize settings
        QSettings settings;
        QString sharesDirectory = settings.value("SharesLocation",  QDir::home().filePath("Public")).toString();
        if (sharesDirectory.endsWith("/"))
            sharesDirectory.chop(1);
        QStringList mappedDirectories = settings.value("SharesDirectories").toStringList();

        // create shares database
        globalDatabase = new QXmppShareDatabase;
        globalDatabase->setDirectory(sharesDirectory);
        globalDatabase->setMappedDirectories(mappedDirectories);
    }
    globalDatabaseRefs++;
}

static void unrefDatabase()
{
    if (!(--globalDatabaseRefs) && globalDatabase) {
        delete globalDatabase;
        globalDatabase = 0; 
    }
}

/** Update collection timestamps.
 */
static void updateTime(QXmppShareItem *oldItem, const QDateTime &stamp)
{
    if (oldItem->type() == QXmppShareItem::CollectionItem && oldItem->size() > 0)
    {
        oldItem->setData(UpdateTime, QDateTime::currentDateTime());
        for (int i = 0; i < oldItem->size(); i++)
            updateTime(oldItem->child(i), stamp);
    }
}

ShareModel::ShareModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new QXmppShareItem(QXmppShareItem::CollectionItem);

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(Qt::DisplayRole, "name");
    roleNames.insert(SizeRole, "size");
    setRoleNames(roleNames);

    /* load icons */
    collectionIcon = QIcon(":/album.png");
    fileIcon = QIcon(":/file.png");
    peerIcon = QIcon(":/peer.png");
}

ShareModel::~ShareModel()
{
    delete rootItem;
}

QXmppShareItem *ShareModel::addItem(const QXmppShareItem &item)
{
   beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
   QXmppShareItem *child = rootItem->appendChild(item);
   endInsertRows();
   return child;
}

ChatClient *ShareModel::client() const
{
    return m_client;
}

void ShareModel::setClient(ChatClient *client)
{
    bool check;

    if (client == m_client)
        return;

    m_client = client;

    check = connect(m_client, SIGNAL(disconnected()),
                    this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(m_client, SIGNAL(presenceReceived(const QXmppPresence&)),
                    this, SLOT(presenceReceived(const QXmppPresence&)));
    Q_ASSERT(check);


    // add shares extension
    QXmppShareExtension *extension = new QXmppShareExtension(m_client, refDatabase());
    m_client->addExtension(extension);

    check = connect(extension, SIGNAL(getFailed(QString)),
                    this, SLOT(getFailed(QString)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(transferFinished(QXmppTransferJob*)),
                    this, SLOT(transferFinished(QXmppTransferJob*)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(transferStarted(QXmppTransferJob*)),
                    this, SLOT(transferStarted(QXmppTransferJob*)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(shareSearchIqReceived(QXmppShareSearchIq)),
                    this, SLOT(shareSearchIqReceived(QXmppShareSearchIq)));
    Q_ASSERT(check);

    check = connect(m_client, SIGNAL(shareServerChanged(QString)),
                    this, SLOT(shareServerChanged(QString)));
    Q_ASSERT(check);

    emit clientChanged(m_client);
}


void ShareModel::clear()
{
    rootItem->clearChildren();
    reset();
}

int ShareModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MaxColumn;
}

QVariant ShareModel::data(const QModelIndex &index, int role) const
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == SizeRole) {
        return item->fileSize();
    }

    if (role == Qt::DisplayRole && index.column() == NameColumn)
        return item->name();
    else if (role == Qt::DisplayRole && index.column() == SizeColumn && item->fileSize())
        return sizeToString(item->fileSize());
    else if (role == Qt::DisplayRole && index.column() == ProgressColumn &&
             item->type() == QXmppShareItem::FileItem)
    {
        const QString localPath = item->data(TransferPath).toString();
        qint64 done = item->data(TransferDone).toLongLong();
        if (!localPath.isEmpty())
            return tr("Downloaded");
        else if (item->data(TransferError).toInt())
            return tr("Failed");
        else if (done > 0)
        {
            qint64 total = index.data(TransferTotal).toLongLong();
            int progress = total ? ((done * 100) / total) : 0;
            return QString::number(progress) + "%";
        }
        else if (!item->data(PacketId).toString().isEmpty())
            return tr("Requested");
        else
            return tr("Queued");
    }
    else if (role == Qt::ToolTipRole && index.column() == ProgressColumn &&
             item->type() == QXmppShareItem::FileItem)
    {
        if (!item->data(TransferError).toInt() &&
            item->data(TransferPath).toString().isEmpty())
        {
            qint64 speed = item->data(TransferSpeed).toLongLong();
            return tr("Downloading at %1").arg(speedToString(speed));
        }
    }
    else if (role == Qt::DecorationRole && index.column() == NameColumn)
    {
        if (item->type() == QXmppShareItem::CollectionItem)
        {
            if (item->locations().size() == 1 && item->locations().first().node().isEmpty())
                return peerIcon;
            else
                return collectionIcon;
        }
        else
            return fileIcon;
    }
    return item->data(role);
}

QList<QXmppShareItem *> ShareModel::filter(const ShareModelQuery &query, const ShareModel::QueryOptions &options, QXmppShareItem *parent, int limit)
{
    if (!parent)
        parent = rootItem;

    QList<QXmppShareItem*> matches;
    QXmppShareItem *child;

    // pre-recurse
    if (options.recurse == PreRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }

    // look at immediate children
    for (int i = 0; i < parent->size(); i++)
    {
        child = parent->child(i);
        if (query.match(child))
            matches.append(child);
        if (limit && matches.size() == limit)
            return matches;
    }

    // post-recurse
    if (options.recurse == PostRecurse)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            int childLimit = limit ? limit - matches.size() : 0;
            matches += filter(query, options, parent->child(i), childLimit);
            if (limit && matches.size() == limit)
                return matches;
        }
    }
    return matches;
}

QXmppShareItem *ShareModel::get(const ShareModelQuery &query, const ShareModel::QueryOptions &options, QXmppShareItem *parent)
{
    QList<QXmppShareItem*> match = filter(query, options, parent, 1);
    if (match.size())
        return match.first();
    return 0;
}

/** Return the title for the given column.
 */
QVariant ShareModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == NameColumn)
            return tr("Name");
        else if (section == ProgressColumn)
            return tr("Progress");
        else if (section == SizeColumn)
            return tr("Size");
    }
    return QVariant();
}

QModelIndex ShareModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());

    QXmppShareItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex ShareModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareItem *childItem = static_cast<QXmppShareItem*>(index.internalPointer());
    QXmppShareItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

void ShareModel::refreshItem(QXmppShareItem *item)
{
    emit dataChanged(createIndex(item->row(), ProgressColumn, item), createIndex(item->row(), ProgressColumn, item));
}

void ShareModel::removeItem(QXmppShareItem *item)
{
    if (!item || item == rootItem)
        return;

    QXmppShareItem *parentItem = item->parent();
    if (parentItem == rootItem)
    {
        beginRemoveRows(QModelIndex(), item->row(), item->row());
        rootItem->removeChild(item);
        endRemoveRows();
    } else {
        beginRemoveRows(createIndex(parentItem->row(), 0, parentItem), item->row(), item->row());
        parentItem->removeChild(item);
        endRemoveRows();
    }
}

int ShareModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());
    return parentItem->size();
}

QModelIndex ShareModel::updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem)
{
    QDateTime stamp = QDateTime::currentDateTime();

    QModelIndex oldIndex;
    if (!oldItem)
        oldItem = rootItem;
    if (oldItem != rootItem)
        oldIndex = createIndex(oldItem->row(), 0, oldItem);

    // update own data
    oldItem->setFileHash(newItem->fileHash());
    oldItem->setFileSize(newItem->fileSize());
    oldItem->setLocations(newItem->locations());
    // root collections have empty names
    if (!newItem->name().isEmpty())
        oldItem->setName(newItem->name());
    oldItem->setType(newItem->type());
    if (oldItem != rootItem)
        emit dataChanged(createIndex(oldItem->row(), NameColumn, oldItem),
                         createIndex(oldItem->row(), SizeColumn, oldItem));

    // if we received an empty collection, stop here
    if (newItem->type() == QXmppShareItem::CollectionItem && !newItem->size())
        return oldIndex;

    // if we received a file, clear any children and stop here
    if (newItem->type() == QXmppShareItem::FileItem)
    {
        if (oldItem->size())
        {
            beginRemoveRows(oldIndex, 0, oldItem->size());
            oldItem->clearChildren();
            endRemoveRows();
        }
        return oldIndex;
    }

    // update own timestamp
    oldItem->setData(UpdateTime, stamp);

    // update existing children
    QList<QXmppShareItem*> removed = oldItem->children();
    for (int newRow = 0; newRow < newItem->size(); newRow++)
    {
        QXmppShareItem *newChild = newItem->child(newRow);
        QXmppShareItem *oldChild = get(Q_FIND_LOCATIONS(newChild->locations()), QueryOptions(DontRecurse), oldItem);
        if (oldChild)
        {
            // update existing child
            const int oldRow = oldChild->row();
            if (oldRow != newRow)
            {
                beginMoveRows(oldIndex, oldRow, oldRow, oldIndex, newRow);
                oldItem->moveChild(oldRow, newRow);
                endMoveRows();
            }
            updateItem(oldChild, newChild);
            removed.removeAll(oldChild);
        } else {
            // insert new child
            beginInsertRows(oldIndex, newRow, newRow);
            QXmppShareItem *item = oldItem->insertChild(newRow, *newChild);
            updateTime(item, stamp);
            endInsertRows();
        }
    }

    // remove old children
    foreach (QXmppShareItem *oldChild, removed)
    {
        beginRemoveRows(oldIndex, oldChild->row(), oldChild->row());
        oldItem->removeChild(oldChild);
        endRemoveRows();
    }

    return oldIndex;
}

ShareModelQuery::ShareModelQuery()
    :  m_role(0), m_operation(None), m_combine(NoCombine)
{
}

ShareModelQuery::ShareModelQuery(int role, ShareModelQuery::Operation operation, QVariant data)
    :  m_role(role), m_operation(operation), m_data(data), m_combine(NoCombine)
{
}

bool ShareModelQuery::match(QXmppShareItem *item) const
{
    if (m_operation == Equals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return item->locations() == m_data.value<QXmppShareLocationList>();
        return item->data(m_role) == m_data;
    }
    else if (m_operation == NotEquals)
    {
        if (m_role == QXmppShareItem::LocationsRole)
            return !(item->locations() == m_data.value<QXmppShareLocationList>());
        return (item->data(m_role) != m_data);
    }
    else if (m_combine == AndCombine)
    {
        foreach (const ShareModelQuery &child, m_children)
            if (!child.match(item))
                return false;
        return true;
    }
    else if (m_combine == OrCombine)
    {
        foreach (const ShareModelQuery &child, m_children)
            if (child.match(item))
                return true;
        return false;
    }
    // empty query matches all
    return true;
}

ShareModelQuery ShareModelQuery::operator&&(const ShareModelQuery &other) const
{
    ShareModelQuery result;
    result.m_combine = AndCombine;
    result.m_children << *this << other;
    return result;
}

ShareModelQuery ShareModelQuery::operator||(const ShareModelQuery &other) const
{
    ShareModelQuery result;
    result.m_combine = OrCombine;
    result.m_children << *this << other;
    return result;
}

ShareModel::QueryOptions::QueryOptions(Recurse recurse_)
    : recurse(recurse_)
{
}


