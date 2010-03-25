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

#include <QApplication>
#include <QDialogButtonBox>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QStringList>
#include <QTabWidget>
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
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

enum DataRoles {
    PacketId = Qt::UserRole,
    StreamId,
    TransferStart,
    TransferDone,
    TransferTotal,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 80

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), baseClient(xmppClient), client(0), db(0), chatTransfers(0)
{
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

    tabWidget = new QTabWidget;
    layout->addWidget(tabWidget);

    /* create main tab */
    ChatSharesModel *sharesModel = new ChatSharesModel;
    sharesView = new ChatSharesView;
    sharesView->setModel(sharesModel);
    sharesView->hideColumn(ProgressColumn);
    connect(sharesView, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)), this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    connect(sharesView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    tabWidget->addTab(sharesView, tr("Shares"));

    /* create search tab */
    ChatSharesModel *searchModel = new ChatSharesModel;
    searchView = new ChatSharesView;
    searchView->setModel(searchModel);
    searchView->hideColumn(ProgressColumn);
    connect(searchView, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)), this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    connect(searchView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    tabWidget->addTab(searchView, tr("Search"));

    /* create queue tab */
    QWidget *downloadsWidget = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(0);
    vbox->setSpacing(0);
    downloadsWidget->setLayout(vbox);

    queueModel = new ChatSharesModel(this);
    downloadsView = new ChatSharesView;
    downloadsView->setModel(queueModel);
    vbox->addWidget(downloadsView);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    QPushButton *removeButton = new QPushButton;
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(transferRemoved()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);
    vbox->addWidget(buttonBox);

    tabWidget->addTab(downloadsWidget, tr("Downloads"));

    /* create uploads tab */
    uploadsView = new ChatTransfersView;
    tabWidget->addTab(uploadsView, tr("Uploads"));

    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(60000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(baseClient, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        baseClient->logger(), SLOT(log(QXmppLogger::MessageType,QString)));
    setClient(baseClient);
}

void ChatShares::disconnected()
{
    if (client && client != baseClient)
    {
        shareServer = "";
        client->disconnect();
        client->deleteLater();
        client = baseClient;
    }
}

void ChatShares::transferAbort(QXmppShareItem *item)
{
    QString sid = item->data(StreamId).toString();
    foreach (QXmppTransferJob *job, downloadJobs)
    {
        if (job->sid() == sid)
        {
            job->abort();
            break;
        }
    }
    foreach (QXmppShareItem *child, item->children())
        transferAbort(child);
}

void ChatShares::transferDestroyed(QObject *obj)
{
    downloadJobs.removeAll(static_cast<QXmppTransferJob*>(obj));
    processDownloadQueue();
}

void ChatShares::transferProgress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, StreamId, job->sid());
    if (queueItem)
        queueModel->setProgress(queueItem, done, total);
}

void ChatShares::transferReceived(QXmppTransferJob *job)
{
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, StreamId, job->sid());
    if (!queueItem)
    {
        job->abort();
        return;
    }

    // determine full path
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
    const QString subdir = pathBits.join("/");

    // create directory
    QDir downloadsDir(SystemInfo::storageLocation(SystemInfo::DownloadsLocation));
    if (!subdir.isEmpty())
    {
        if (downloadsDir.exists(subdir) || downloadsDir.mkpath(subdir))
            downloadsDir.setPath(downloadsDir.filePath(subdir));
    }

    // determine file name
    const QString filePath = ChatTransfers::availableFilePath(downloadsDir.path(), job->fileName());
    QFile *file = new QFile(filePath, job);
    if (!file->open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not write to" << filePath;
        job->abort();
        delete file;
        return;
    }

    // add transfer to list
    job->setData(LocalPathRole, filePath);
    connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(transferDestroyed(QObject*)));
    connect(job, SIGNAL(progress(qint64, qint64)), this, SLOT(transferProgress(qint64,qint64)));
    connect(job, SIGNAL(stateChanged(QXmppTransferJob::State)), this, SLOT(transferStateChanged(QXmppTransferJob::State)));
    downloadJobs.append(job);

    // start transfer
    job->accept(file);
}

void ChatShares::transferRemoved()
{
    const QModelIndex &index = downloadsView->currentIndex();
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return;

    transferAbort(item);
    queueModel->removeItem(item);
}

void ChatShares::transferStateChanged(QXmppTransferJob::State state)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, StreamId, job->sid());

    if (state == QXmppTransferJob::TransferState && queueItem)
    {
        QTime t;
        t.start();
        queueItem->setData(TransferStart, t);
    }
    else if (state == QXmppTransferJob::FinishedState)
    {
        if (queueItem)
            queueItem->setData(TransferStart, QVariant());

        // if the transfer failed, delete the local file
        if (job->error() != QXmppTransferJob::NoError)
            QFile(job->data(LocalPathRole).toString()).remove();
        job->deleteLater();
    }
}

void ChatShares::findRemoteFiles()
{
    const QString search = lineEdit->text();

    // search for files
    QXmppShareSearchIq iq;
    iq.setTo(shareServer);
    iq.setType(QXmppIq::Get);
    iq.setDepth(3);
    iq.setSearch(search);
    searches.insert(iq.tag(), search.isEmpty() ? sharesView : searchView);
    client->sendPacket(iq);
}

void ChatShares::itemAction()
{
    QAction *action = static_cast<QAction*>(sender());
    ChatSharesView *treeWidget = qobject_cast<ChatSharesView*>(tabWidget->currentWidget());
    if (!action || !treeWidget)
        return;
    QXmppShareItem *item = static_cast<QXmppShareItem*>(treeWidget->currentIndex().internalPointer());
    if (!item)
        return;

    if (action && action->data() == DownloadAction)
    {
        queueModel->addItem(*item);
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

    if (item->type() == QXmppShareItem::FileItem)
    {
        // download item
        queueModel->addItem(*item);
        processDownloadQueue();
    }
    else if (item->type() == QXmppShareItem::CollectionItem && !item->size())
    {
        if (item->locations().isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "No location for collection " + item->name());
            return; 
        }
        const QXmppShareLocation location = item->locations().first();

        // browse files
        QXmppShareSearchIq iq;
        iq.setTo(location.jid());
        iq.setType(QXmppIq::Get);
        iq.setDepth(1);
        iq.setNode(location.node());
        searches.insert(iq.tag(), tabWidget->currentWidget());
        client->sendPacket(iq);
    }
}

void ChatShares::presenceReceived(const QXmppPresence &presence)
{
    if (presence.from() != shareServer)
        return;

    // find shares extension
    QXmppElement shareExtension;
    foreach (const QXmppElement &extension, presence.extensions())
    {
        if (extension.attribute("xmlns") == ns_shares)
        {
            shareExtension = extension;
            break;
        }
    }
    if (shareExtension.attribute("xmlns") != ns_shares)
        return;

    if (presence.getType() == QXmppPresence::Available)
    {
        const QString forceProxy = shareExtension.firstChildElement("force-proxy").value();
        if (forceProxy == "1")
        {
            logMessage(QXmppLogger::InformationMessage, "Forcing SOCKS5 proxy");
            client->getTransferManager().setProxyOnly(true);
        }

        // browse peers
        findRemoteFiles();
        emit registerTab();
    }
    else if (presence.getType() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString domain = shareExtension.firstChildElement("domain").value();
        const QString server = shareExtension.firstChildElement("server").value();

        logMessage(QXmppLogger::InformationMessage, "Redirecting to " + domain + "," + server);

        // reconnect to another server
        registerTimer->stop();

        QXmppConfiguration config = baseClient->getConfiguration();
        config.setDomain(domain);
        config.setHost(server);
        config.setIgnoreSslErrors(true);

        ChatClient *newClient = new ChatClient(this);
        connect(&newClient->getTransferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
            this, SLOT(transferReceived(QXmppTransferJob*)));
        newClient->setLogger(baseClient->logger());

        /* replace client */
        if (client && client != baseClient)
            client->deleteLater();
        setClient(newClient);
        newClient->connectToServer(config);
    }
}

void ChatShares::processDownloadQueue()
{
    // check how many downloads are active
    int activeDownloads = downloadJobs.size();
    while (activeDownloads < parallelDownloadLimit)
    {
        // find next item
        QXmppShareItem *file = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, QVariant());
        if (!file)
            return;

        // pick location
        QXmppShareLocation location;
        bool locationFound = false;
        foreach (location, file->locations())
        {
            if (!location.jid().isEmpty() &&
                location.jid() != client->getConfiguration().jid() &&
                !location.node().isEmpty())
            {
                locationFound = true;
                break;
            }
        }
        if (!locationFound)
        {
            logMessage(QXmppLogger::WarningMessage, "No location found for file " + file->name());
            queueModel->removeItem(file);
            continue;
        }

        // request file
        QXmppShareGetIq iq;
        iq.setTo(location.jid());
        iq.setType(QXmppIq::Get);
        iq.setNode(location.node());
        file->setData(PacketId, iq.id());
        client->sendPacket(iq);

        activeDownloads++;
    }
    queueModel->pruneEmptyChildren();
}

void ChatShares::queryStringChanged()
{
    if (lineEdit->text().isEmpty())
        tabWidget->setCurrentWidget(sharesView);
    else
        tabWidget->setCurrentWidget(searchView);
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

void ChatShares::setClient(ChatClient *newClient)
{
    client = newClient;

    connect(client, SIGNAL(presenceReceived(const QXmppPresence&)), this, SLOT(presenceReceived(const QXmppPresence&)));
    connect(client, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)), this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    connect(client, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)), this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
    connect(client, SIGNAL(shareServerFound(const QString&)), this, SLOT(shareServerFound(const QString&)));
}

void ChatShares::setTransfers(ChatTransfers *transfers)
{
    chatTransfers = transfers;
}

void ChatShares::shareGetIqReceived(const QXmppShareGetIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
    {
        QXmppShareGetIq responseIq;
        responseIq.setId(shareIq.id());
        responseIq.setTo(shareIq.from());
        responseIq.setType(QXmppIq::Result);

        // check path is OK
        QString filePath = db->locate(shareIq.node());
        if (filePath.isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "Could not find local file " + shareIq.node());
            responseIq.setType(QXmppIq::Error);
            client->sendPacket(responseIq);
            return;
        }
        responseIq.setSid(generateStanzaHash());
        client->sendPacket(responseIq);

        // send file
        QXmppTransferJob *job = client->getTransferManager().sendFile(responseIq.to(), filePath, responseIq.sid());
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
        job->setData(LocalPathRole, filePath);
        uploadsView->addJob(job);
        return;
    }

    QXmppShareItem *queueItem = queueModel->findItemByData(QXmppShareItem::FileItem, PacketId, shareIq.id());
    if (!queueItem)
        return;

    if (shareIq.type() == QXmppIq::Result)
    {
        queueItem->setData(StreamId, shareIq.sid());
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        logMessage(QXmppLogger::WarningMessage, "Error requesting file from " + shareIq.from());
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
    else if (shareIq.type() == QXmppIq::Error)
    {
        logMessage(QXmppLogger::WarningMessage, "Search failed " + shareIq.search());
        return;
    }

    // process search result
    if (!searches.contains(shareIq.tag()))
        return;
    ChatSharesView *view = qobject_cast<ChatSharesView*>(searches.take(shareIq.tag()));
    ChatSharesModel *model = qobject_cast<ChatSharesModel*>(view->model());
    Q_ASSERT(model);

    QModelIndex index = model->mergeItem(shareIq.collection());
    view->setExpanded(index, true);
}

void ChatShares::searchFinished(const QXmppShareSearchIq &iq)
{
    client->sendPacket(iq);
}

void ChatShares::shareServerFound(const QString &server)
{
    shareServer = server;

    if (!db)
    {
        db = new ChatSharesDatabase(SystemInfo::storageLocation(SystemInfo::SharesLocation), this);
        connect(db, SIGNAL(searchFinished(const QXmppShareSearchIq&)), this, SLOT(searchFinished(const QXmppShareSearchIq&)));
    }

    // register with server
    registerWithServer();
    registerTimer->start();
}

ChatSharesDelegate::ChatSharesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ChatSharesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int done = index.data(TransferDone).toInt();
    int total = index.data(TransferTotal).toInt();
    if (index.column() == ProgressColumn && done > 0 && total > 0)
    {
        QStyleOptionProgressBar progressBarOption;
        progressBarOption.rect = option.rect;
        progressBarOption.minimum = 0;
        progressBarOption.maximum = total; 
        progressBarOption.progress = done;
        progressBarOption.state = QStyle::State_Active | QStyle::State_Enabled | QStyle::State_HasFocus | QStyle::State_Selected;
        progressBarOption.text = index.data(Qt::DisplayRole).toString();
        progressBarOption.textVisible = true;

        QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                           &progressBarOption, painter);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
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
    if (findItemByLocations(item.locations(), rootItem))
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
    else if (index.column() == ProgressColumn)
    {
        if (role == Qt::DisplayRole)
        {
            int done = item->data(TransferDone).toInt();
            QTime t = index.data(TransferStart).toTime();
            if (done <= 0 || !t.isValid() || !t.elapsed())
                return QVariant();
            int speed = (done * 1000.0) / t.elapsed();
            return ChatTransfers::sizeToString(speed) + "/s";
        }
        return item->data(role);
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

QXmppShareItem *ChatSharesModel::findItemByLocations(const QXmppShareLocationList &locations, QXmppShareItem *parent, bool recurse)
{
    if (locations.isEmpty())
        return 0;

    // look at immediate children
    QXmppShareItem *child;
    foreach (const QXmppShareLocation &location, locations)
    {
        for (int i = 0; i < parent->size(); i++)
        {
            child = parent->child(i);
            if (child->locations().contains(location))
                return child;
        }
    }

    // recurse
    if (recurse)
    {
        for (int i = 0; i < parent->size(); i++)
            if ((child = findItemByLocations(locations, parent->child(i))) != 0)
                return child;
    }

    return 0;
}

QVariant ChatSharesModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void ChatSharesModel::setProgress(QXmppShareItem *item, qint64 done, qint64 total)
{
    item->setData(TransferDone, done);
    item->setData(TransferTotal, total);
    emit dataChanged(createIndex(item->row(), ProgressColumn, item), createIndex(item->row(), ProgressColumn, item));
}

QModelIndex ChatSharesModel::mergeItem(const QXmppShareItem &item)
{
    // FIXME : we are casting away constness
    QXmppShareItem *newItem = (QXmppShareItem*)&item;
    QXmppShareItem *parentItem = findItemByLocations(newItem->locations(), rootItem);
    if (!parentItem)
        parentItem = rootItem;
    return updateItem(parentItem, newItem);
}

QModelIndex ChatSharesModel::updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem)
{
    QModelIndex oldIndex;
    if (oldItem != rootItem)
        oldIndex = createIndex(oldItem->row(), 0, oldItem);

    oldItem->setFileHash(newItem->fileHash());
    oldItem->setFileSize(newItem->fileSize());
    oldItem->setLocations(newItem->locations());
    //oldItem->setName(newItem->name());
    oldItem->setType(newItem->type());

    QList<QXmppShareItem*> removed = oldItem->children();
    QList<QXmppShareItem*> added;
    foreach (QXmppShareItem *newChild, newItem->children())
    {
        QXmppShareItem *oldChild = findItemByLocations(newChild->locations(), oldItem, false);
        if (oldChild)
        {
            updateItem(oldChild, newChild);
            removed.removeAll(oldChild);
        } else {
            added << newChild;
        }
    }

    // remove old children if we received a file or a non-empty collection
    if (newItem->type() == QXmppShareItem::FileItem || newItem->size())
    {
        foreach (QXmppShareItem *oldChild, removed)
        {
            beginRemoveRows(oldIndex, oldChild->row(), oldChild->row());
            oldItem->removeChild(oldChild);
            endRemoveRows();
        }
    }

    // add new children
    if (added.size())
    {
        beginInsertRows(oldIndex, oldItem->size(), oldItem->size() + added.size());
        foreach (QXmppShareItem *newChild, added)
            oldItem->appendChild(*newChild);
        endInsertRows();
    }

    return oldIndex;
}

ChatSharesView::ChatSharesView(QWidget *parent)
    : QTreeView(parent)
{
    setItemDelegate(new ChatSharesDelegate(this));
    setRootIsDecorated(false);
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
    int nameWidth =  e->size().width();
    if (!isColumnHidden(SizeColumn))
        nameWidth -= SIZE_COLUMN_WIDTH;
    if (!isColumnHidden(ProgressColumn))
        nameWidth -= PROGRESS_COLUMN_WIDTH;
    setColumnWidth(NameColumn, nameWidth);
}

void ChatSharesView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(SizeColumn, SIZE_COLUMN_WIDTH);
    setColumnWidth(ProgressColumn, PROGRESS_COLUMN_WIDTH);
}
