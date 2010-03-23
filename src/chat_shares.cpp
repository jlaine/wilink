/*
 * wDesktop
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

#include <QFileIconProvider>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QStringList>
#include <QTimer>
#include <QTreeWidget>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_shares.h"
#include "chat_shares_database.h"
#include "chat_transfers.h"
#include "systeminfo.h"

static int parallelDownloadLimit = 2;

enum Actions
{
    DownloadAction,
};

enum Columns
{
    NameColumn,
    SizeColumn,
    MaxColumn,
};

enum DataRoles {
    PacketId,
    StreamId,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), client(xmppClient), db(0)
{
    queueModel = new ChatSharesModel(this);
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("Shares"));

    qRegisterMetaType<QXmppShareSearchIq>("QXmppShareSearchIq");

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addItem(statusBar());
    layout->addWidget(new QLabel(tr("Enter the name of the file you are looking for.")));
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    connect(lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(queryStringChanged()));
    layout->addWidget(lineEdit);

    /* create model / view */
    model = new ChatSharesModel;
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), model, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
    connect(model, SIGNAL(itemReceived(const QModelIndex&)), this, SLOT(itemReceived(const QModelIndex&)));
    treeWidget = new ChatSharesView;
    treeWidget->setModel(model);
    connect(treeWidget, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)), this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    connect(treeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    layout->addWidget(treeWidget);

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(client, SIGNAL(siPubIqReceived(const QXmppSiPubIq&)), this, SLOT(siPubIqReceived(const QXmppSiPubIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
    connect(&client->getTransferManager(), SIGNAL(finished(QXmppTransferJob*)), this, SLOT(processDownloadQueue()));
}

ChatSharesModel *ChatShares::downloadQueue()
{
    return queueModel;
}

void ChatShares::processDownloadQueue()
{
    // check how many downloads are active
    int activeDownloads = client->getTransferManager().activeJobs(QXmppTransferJob::IncomingDirection);
    while (activeDownloads < parallelDownloadLimit)
    {
        // find next item
        QXmppShareItem *file = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, QVariant());
        if (!file)
            return;

        // pick mirror
        QXmppShareMirror mirror;
        bool mirrorFound = false;
        foreach (mirror, file->mirrors())
        {
            if (!mirror.jid().isEmpty() && !mirror.path().isEmpty())
            {
                mirrorFound = true;
                break;
            }
        }
        if (!mirrorFound)
        {
            qWarning() << "No mirror found for file" << file->name();
            return;
        }

        // request file
        QXmppSiPubIq iq;
        iq.setTo(mirror.jid());
        iq.setType(QXmppIq::Get);
        iq.setPublishId(mirror.path());
        qDebug() << "Requesting file" << file->name() << "from" << iq.to();
        file->setData(PacketId, iq.id());
        client->sendPacket(iq);

        activeDownloads++;
    }
}

void ChatShares::siPubIqReceived(const QXmppSiPubIq &shareIq)
{
#if 0
    if (shareIq.from() != shareServer)
        return;
#endif

    if (shareIq.type() == QXmppIq::Get)
    {
        QXmppSiPubIq responseIq;
        responseIq.setId(shareIq.id());
        responseIq.setTo(shareIq.from());
        responseIq.setType(QXmppIq::Result);

        // check path is OK
        QString filePath = db->locate(shareIq.publishId());
        if (filePath.isEmpty())
        {
            qWarning() << "Could not find file" << shareIq.publishId();
            responseIq.setType(QXmppIq::Error);
            client->sendPacket(responseIq);
            return;
        }
        responseIq.setStreamId(generateStanzaHash());
        client->sendPacket(responseIq);

        // send file
        qDebug() << "Sending" << QFileInfo(filePath).fileName() << "to" << responseIq.to();
        QXmppTransferJob *job = client->getTransferManager().sendFile(responseIq.to(), filePath, responseIq.streamId());
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
        return;
    }

    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, shareIq.id());
    if (!queueItem)
        return;

    if (shareIq.type() == QXmppIq::Result)
    {
        // store full path
        QStringList pathBits;
        QXmppShareItem *parentItem = queueItem->parent();
        while (parentItem && parentItem->parent())
        {
            // sanitize path
            QString dirName = parentItem->name();
            if (dirName != "." && dirName != ".." && !dirName.contains("/") && !dirName.contains("\\"))
                pathBits.prepend(dirName);
            parentItem = parentItem->parent();
        }

        // remove from queue
        //queueModel->removeItem(queueItem);

        emit fileExpected(shareIq.streamId(), pathBits.join("/"));
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        qWarning() << "Error requesting file from" << shareIq.from();
        queueModel->removeItem(queueItem);
    }
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
    {
        db->search(shareIq);
        return;
    }
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();

    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setSearch(search);
    client->sendPacket(iq);
}

void ChatShares::itemAction()
{
    QAction *action = static_cast<QAction*>(sender());
    QModelIndex index = treeWidget->currentIndex();
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!action || !index.isValid() || !item)
        return;

    if (action->data() == DownloadAction)
    {
        // queue file download
        qDebug() << "Adding" << item->name() << "to queue";
        queueModel->addItem(*item);
        queueModel->pruneEmptyChildren();
        processDownloadQueue();
    }
}

void ChatShares::itemContextMenu(const QModelIndex &index, const QPoint &globalPos)
{
    QMenu *menu = new QMenu(this);

    QAction *action = menu->addAction(tr("Download"));
    action->setData(DownloadAction);
    connect(action, SIGNAL(triggered()), this, SLOT(itemAction()));

    menu->popup(globalPos);
}

void ChatShares::itemDoubleClicked(const QModelIndex &index)
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return;

    if (item->mirrors().isEmpty())
    {
        qWarning() << "No mirror for item" << item->name();
        return; 
    }
    const QXmppShareMirror mirror = item->mirrors().first();

    if (item->type() == QXmppShareItem::FileItem)
    {
        // queue file download
        queueModel->addItem(*item);
        queueModel->pruneEmptyChildren();
        processDownloadQueue();
    }
    else if (item->type() == QXmppShareItem::CollectionItem && !item->size())
    {
        // browse files
        QXmppShareSearchIq iq;
        iq.setTo(mirror.jid());
        iq.setType(QXmppIq::Get);
        iq.setBase(mirror.path());
        client->sendPacket(iq);
    }
}

void ChatShares::itemReceived(const QModelIndex &index)
{
    treeWidget->setExpanded(index, true);
}

void ChatShares::queryStringChanged()
{
    if (lineEdit->text().isEmpty())
        findRemoteFiles();
}

void ChatShares::registerWithServer()
{
    // register with server
    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(x);
    client->sendPacket(presence);
}

void ChatShares::setShareServer(const QString &server)
{
    shareServer = server;

    if (!db)
    {
        db = new ChatSharesDatabase(SystemInfo::storageLocation(SystemInfo::SharesLocation), this);
        connect(db, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SLOT(searchFinished(const QXmppShareSearchIq&)));
    }
    model->setShareServer(server);

    // register with server
    registerWithServer();
    registerTimer->start();

    // browse peers
    findRemoteFiles();
}

ChatSharesModel::ChatSharesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new QXmppShareItem(QXmppShareItem::CollectionItem);

    /* load icons */
    QFileIconProvider iconProvider;
    collectionIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);
    peerIcon = iconProvider.icon(QFileIconProvider::Network);
}

ChatSharesModel::~ChatSharesModel()
{
    delete rootItem;
}

void ChatSharesModel::addItem(const QXmppShareItem &item)
{
    if (findItemByMirrors(item.mirrors(), rootItem))
        return;

    beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
    rootItem->appendChild(item);
    endInsertRows();
}

int ChatSharesModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant ChatSharesModel::data(const QModelIndex &index, int role) const
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::DisplayRole && index.column() == NameColumn)
        return item->name();
    else if (role == Qt::DisplayRole && index.column() == SizeColumn && item->fileSize())
        return ChatTransfers::sizeToString(item->fileSize());
    else if (role == Qt::DecorationRole && index.column() == NameColumn)
    {
        if (item->type() == QXmppShareItem::CollectionItem)
        {
            if (item->mirrors().size() == 1 && item->mirrors().first().path().isEmpty())
                return peerIcon;
            else
                return collectionIcon;
        }
        else
            return fileIcon;
    }
    return QVariant();
}

QXmppShareItem *ChatSharesModel::findItemByData(QXmppShareItem::Type type, int role, const QVariant &data, QXmppShareItem *parent)
{
    if (!parent)
        parent = rootItem;

    // recurse
    QXmppShareItem *child;
    for (int i = 0; i < parent->size(); i++)
        if ((child = findItemByData(type, role, data, parent->child(i))) != 0)
            return child;

    // look at immediate children
    for (int i = 0; i < parent->size(); i++)
    {
        child = parent->child(i);
        if (child->type() == type && child->data(role) == data)
            return child;
    }
    return 0;
}

QXmppShareItem *ChatSharesModel::findItemByMirrors(const QXmppShareMirrorList &mirrors, QXmppShareItem *parent)
{
    if (mirrors.isEmpty())
        return 0;

    // look at immediate children
    QXmppShareItem *child;
    foreach (const QXmppShareMirror &mirror, mirrors)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            child = parent->child(i);
            if (child->mirrors().contains(mirror))
                return child;
        }
    }

    // recurse
    for (int i = 0; i < parent->size(); i++)
        if ((child = findItemByMirrors(mirrors, parent->child(i))) != 0)
            return child;

    return 0;
}

QVariant ChatSharesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (section == NameColumn)
            return tr("Name");
        else if (section == SizeColumn)
            return tr("Size");
    }
    return QVariant();
}

QModelIndex ChatSharesModel::index(int row, int column, const QModelIndex &parent) const
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

QModelIndex ChatSharesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QXmppShareItem *childItem = static_cast<QXmppShareItem*>(index.internalPointer());
    QXmppShareItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

void ChatSharesModel::pruneEmptyChildren(QXmppShareItem *parent)
{
    if (!parent)
        parent = rootItem;

    // recurse
    for (int i = 0; i < parent->size(); i++)
        pruneEmptyChildren(parent->child(i));

    // look at immediate children
    for (int i = parent->size() - 1; i >= 0; i--)
    {
        QXmppShareItem *child = parent->child(i);
        if (child->type() == QXmppShareItem::CollectionItem && !child->size())
            removeItem(child);
    }
}

void ChatSharesModel::removeItem(QXmppShareItem *item)
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

int ChatSharesModel::rowCount(const QModelIndex &parent) const
{
    QXmppShareItem *parentItem;
    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<QXmppShareItem*>(parent.internalPointer());
    return parentItem->size();
}

void ChatSharesModel::setShareServer(const QString &server)
{
    shareServer = server;
}

void ChatSharesModel::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Result)
    {
        QXmppShareItem *parentItem = findItemByMirrors(shareIq.collection().mirrors(), rootItem);
        if (parentItem)
        {
            if (parentItem->size())
            {
                beginRemoveRows(createIndex(parentItem->row(), 0, parentItem), 0, parentItem->size());
                parentItem->clearChildren();
                endRemoveRows();
            }
            if (shareIq.collection().size())
            {
                beginInsertRows(createIndex(parentItem->row(), 0, parentItem), 0, shareIq.collection().size());
                for (int i = 0; i < shareIq.collection().size(); i++)
                    parentItem->appendChild(*shareIq.collection().child(i));
                endInsertRows();
            }
            emit itemReceived(createIndex(parentItem->row(), 0, parentItem));
        } else if (shareIq.from() == shareServer) {
            rootItem->clearChildren();
            for (int i = 0; i < shareIq.collection().size(); i++)
                rootItem->appendChild(*shareIq.collection().child(i));
            emit reset();
        }
    }
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeView(parent)
{
    setColumnWidth(SizeColumn, 80);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void ChatSharesView::contextMenuEvent(QContextMenuEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (!index.isValid())
        return;

    emit contextMenu(index, event->globalPos());
}

void ChatSharesView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);
    setColumnWidth(NameColumn, e->size().width() - 80);
}

