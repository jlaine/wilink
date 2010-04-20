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

#include <QApplication>
#include <QDesktopServices>
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
#include <QUrl>

#include "qxmpp/QXmppShareIq.h"
#include "qxmpp/QXmppUtils.h"

#include "chat.h"
#include "chat_roster.h"
#include "chat_shares.h"
#include "chat_shares_database.h"
#include "chat_transfers.h"
#include "systeminfo.h"

static int parallelDownloadLimit = 2;

enum Columns
{
    NameColumn,
    ProgressColumn,
    SizeColumn,
    MaxColumn,
};

enum DataRoles {
    PacketId = QXmppShareItem::MaxRole,
    PacketStart,
    StreamId,
    TransferStart,
    TransferDone,
    TransferPath,
    TransferTotal,
    TransferError,
    UpdateTime,
};

Q_DECLARE_METATYPE(QXmppShareSearchIq)

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 100
#define REFRESH_INTERVAL 60
#define REGISTER_INTERVAL 60
#define REQUEST_TIMEOUT 60

// common queries
#define Q ChatSharesModelQuery
#define Q_FIND_LOCATIONS(locations)  Q(QXmppShareItem::LocationsRole, Q::Equals, QVariant::fromValue(locations))
#define Q_FIND_TRANSFER(sid) \
    (Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) && \
     Q(StreamId, Q::Equals, sid))

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

ChatShares::ChatShares(ChatClient *xmppClient, QWidget *parent)
    : ChatPanel(parent), baseClient(xmppClient), client(0), db(0), rosterModel(0)
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
    sharesView->setExpandsOnDoubleClick(false);
    sharesView->setModel(sharesModel);
    sharesView->hideColumn(ProgressColumn);
    connect(sharesView, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)), this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    connect(sharesView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    connect(sharesView, SIGNAL(expandRequested(QModelIndex)), this, SLOT(itemExpandRequested(QModelIndex)));
    tabWidget->addTab(sharesView, QIcon(":/album.png"), tr("Shares"));

    /* create search tab */
    ChatSharesModel *searchModel = new ChatSharesModel;
    searchView = new ChatSharesView;
    searchView->setExpandsOnDoubleClick(false);
    searchView->setModel(searchModel);
    searchView->hideColumn(ProgressColumn);
    connect(searchView, SIGNAL(contextMenu(QModelIndex, QPoint)), this, SLOT(itemContextMenu(QModelIndex, QPoint)));
    connect(searchView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
    connect(searchView, SIGNAL(expandRequested(QModelIndex)), this, SLOT(itemExpandRequested(QModelIndex)));
    tabWidget->addTab(searchView, QIcon(":/search.png"), tr("Search"));

    /* create queue tab */
    downloadsWidget = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(0);
    downloadsWidget->setLayout(vbox);

    /* download location label */
    const QString downloadsLink = QString("<a href=\"file://%1\">%2</a>").arg(
        SystemInfo::storageLocation(SystemInfo::DownloadsLocation),
        SystemInfo::displayName(SystemInfo::DownloadsLocation));
    QLabel *downloadsLabel = new QLabel(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(downloadsLink));
    downloadsLabel->setOpenExternalLinks(true);
    downloadsLabel->setWordWrap(true);
    vbox->addWidget(downloadsLabel);

    queueModel = new ChatSharesModel(this);
    downloadsView = new ChatSharesView;
    downloadsView->setModel(queueModel);
    connect(downloadsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(transferDoubleClicked(const QModelIndex&)));
    connect(downloadsView, SIGNAL(expandRequested(QModelIndex)), downloadsView, SLOT(expand(QModelIndex)));
    vbox->addWidget(downloadsView);

    tabWidget->addTab(downloadsWidget, QIcon(":/download.png"), tr("Downloads"));

    /* create uploads tab */
    QWidget *uploadsWidget = new QWidget;
    vbox = new QVBoxLayout;
    vbox->setMargin(0);
    uploadsWidget->setLayout(vbox);

    /* download location label */
    const QString sharesLink = QString("<a href=\"file://%1\">%2</a>").arg(
        SystemInfo::storageLocation(SystemInfo::SharesLocation),
        SystemInfo::displayName(SystemInfo::SharesLocation));
    QLabel *uploadsLabel = new QLabel(tr("To share files with other users, simply place them in your %1 folder.").arg(sharesLink));
    uploadsLabel->setOpenExternalLinks(true);
    uploadsLabel->setWordWrap(true);
    vbox->addWidget(uploadsLabel);

    uploadsView = new ChatTransfersView;
    vbox->addWidget(uploadsView);

    tabWidget->addTab(uploadsWidget, QIcon(":/upload.png"), tr("Uploads"));

    /* button box */
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setIcon(QIcon(":/download.png"));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadItem()));
    buttonBox->addButton(downloadButton, QDialogButtonBox::ActionRole);

    removeButton = new QPushButton(tr("Remove"));
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(transferRemoved()));
    buttonBox->addButton(removeButton, QDialogButtonBox::ActionRole);
    removeButton->hide();

    layout->addWidget(buttonBox);
    setLayout(layout);

    /* connect signals */
    registerTimer = new QTimer(this);
    registerTimer->setInterval(REGISTER_INTERVAL * 1000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(baseClient, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
        baseClient->logger(), SLOT(log(QXmppLogger::MessageType,QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    setClient(baseClient);
}

/** When the main XMPP stream is disconnected, disconnect the shares-specific
 *  stream too.
 */
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

/** Recursively cancel any transfer jobs associated with a download queue item.
 */
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

/** When a transfer job is destroyed, remove it from our list
 *  an process the download queue.
 */
void ChatShares::transferDestroyed(QObject *obj)
{
    downloadJobs.removeAll(static_cast<QXmppTransferJob*>(obj));
    processDownloadQueue();
}

/** When the user double clicks an item in the download queue,
 *  open it if it is fully retrieved.
 */
void ChatShares::transferDoubleClicked(const QModelIndex &index)
{
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!index.isValid() || !item)
        return;

    QString localPath = item->data(TransferPath).toString();
    if (item->type() == QXmppShareItem::FileItem && !localPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
}

/** Update the progress bar for a transfer job.
 */
void ChatShares::transferProgress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job->sid()));
    if (queueItem)
    {
        // update progress
        queueItem->setData(TransferDone, done);
        queueItem->setData(TransferTotal, total);
        queueModel->refreshItem(queueItem);
    }
}

/** When we receive a file, check it is in the download queue and accept it.
 */
void ChatShares::transferReceived(QXmppTransferJob *job)
{
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job->sid()));
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

/** When the user removes an item from the download queue, cancel any jobs
 *  associated with it.
 */
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
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job->sid()));

    if (state == QXmppTransferJob::TransferState && queueItem)
    {
        QTime t;
        t.start();
        queueItem->setData(TransferStart, t);
    }
    else if (state == QXmppTransferJob::FinishedState)
    {
        const QString localPath = job->data(LocalPathRole).toString();
        if (queueItem)
        {
            queueItem->setData(PacketId, QVariant());
            queueItem->setData(StreamId, QVariant());
            queueItem->setData(TransferStart, QVariant());
            if (job->error() == QXmppTransferJob::NoError)
            {
                queueItem->setData(TransferPath, localPath);
                queueItem->setData(TransferError, QVariant());
            } else {
                queueItem->setData(TransferPath, QVariant());
                queueItem->setData(TransferError, job->error());
            }
            queueModel->refreshItem(queueItem);
        }

        // if the transfer failed, delete the local file
        if (job->error() != QXmppTransferJob::NoError)
            QFile(localPath).remove();

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

void ChatShares::downloadItem()
{
    ChatSharesView *treeWidget = qobject_cast<ChatSharesView*>(tabWidget->currentWidget());
    if (!treeWidget)
        return;

    QXmppShareItem *item = static_cast<QXmppShareItem*>(treeWidget->currentIndex().internalPointer());
    if (!item)
        return;

    QXmppShareItem *queueItem = queueModel->addItem(*item);
    QXmppShareItem *emptyChild = queueModel->get(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::CollectionItem) &&
            Q(QXmppShareItem::SizeRole, Q::Equals, 0),
            ChatSharesModel::QueryOptions(), queueItem);

    // if we have at least one empty child, we need to retrieve the children
    // of the item we just queued
    if (queueItem->type() == QXmppShareItem::CollectionItem && (!queueItem->size() || emptyChild))
    {
        if (queueItem->locations().isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "No location for collection " + item->name());
            return;
        }
        const QXmppShareLocation location = queueItem->locations().first();

        // retrieve full tree
        QXmppShareSearchIq iq;
        iq.setTo(location.jid());
        iq.setType(QXmppIq::Get);
        iq.setDepth(0);
        iq.setNode(location.node());
        searches.insert(iq.tag(), downloadsView);
        client->sendPacket(iq);
    }

    processDownloadQueue();
}

void ChatShares::itemContextMenu(const QModelIndex &index, const QPoint &globalPos)
{
    QMenu *menu = new QMenu(this);

    QAction *action = menu->addAction(tr("Download"));
    action->setIcon(QIcon(":/download.png"));
    connect(action, SIGNAL(triggered()), this, SLOT(downloadItem()));

    menu->popup(globalPos);
}

void ChatShares::itemDoubleClicked(const QModelIndex &index)
{
    ChatSharesView *view = qobject_cast<ChatSharesView*>(sender());
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!view || !index.isValid() || !item)
        return;

    if (item->type() == QXmppShareItem::FileItem)
    {
        // download item
        queueModel->addItem(*item);
        processDownloadQueue();
    }
    else if (item->type() == QXmppShareItem::CollectionItem)
    {
        if (view->isExpanded(index))
            view->collapse(index);
        else
            itemExpandRequested(index);
    }
}

/** When the user asks to expand a node, check whether we need to refresh
 *  its contents.
 */
void ChatShares::itemExpandRequested(const QModelIndex &index)
{
    ChatSharesView *view = qobject_cast<ChatSharesView*>(sender());
    QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
    if (!view || !index.isValid() || !item || !item->type() == QXmppShareItem::CollectionItem)
        return;

    // determine whether we need a refresh
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-REFRESH_INTERVAL);
    QDateTime updateTime = item->data(UpdateTime).toDateTime();
    if (updateTime.isValid() && (view == searchView || updateTime >= cutoffTime))
    {
        view->expand(index);
        return;
    }

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
    searches.insert(iq.tag(), view);
    client->sendPacket(iq);
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
        if (forceProxy == "1" && !client->getTransferManager().proxyOnly())
        {
            logMessage(QXmppLogger::InformationMessage, "Forcing SOCKS5 proxy");
            client->getTransferManager().setProxyOnly(true);
        }

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
    QList<QXmppShareItem *> active = queueModel->filter(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) &&
            Q(PacketId, Q::NotEquals, QVariant()));

    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-REQUEST_TIMEOUT);
    foreach (QXmppShareItem *queueItem, active)
    {
        QDateTime startTime = queueItem->data(PacketStart).toDateTime();
        if (startTime.isValid() && startTime <= cutoffTime)
        {
            logMessage(QXmppLogger::WarningMessage, "Request timed out for file " + queueItem->name());
            queueItem->setData(PacketId, QVariant());
            queueItem->setData(PacketStart, QVariant());
            queueItem->setData(TransferError, QXmppTransferJob::ProtocolError);
            queueModel->refreshItem(queueItem);
            active.removeAll(queueItem);
        }
    }

    int activeDownloads = active.size();
    while (activeDownloads < parallelDownloadLimit)
    {
        // find next item
        QXmppShareItem *file = queueModel->get(
                Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) &&
                Q(PacketId, Q::Equals, QVariant()) &&
                Q(TransferPath, Q::Equals, QVariant()) &&
                Q(TransferError, Q::Equals, QVariant()));
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
        client->sendPacket(iq);

        file->setData(PacketId, iq.id());
        file->setData(PacketStart, QDateTime::currentDateTime());
        QTimer::singleShot(REQUEST_TIMEOUT * 1000, this, SLOT(processDownloadQueue()));

        queueModel->refreshItem(file);

        activeDownloads++;
    }
}

/** When the user changes the query string, switch to the appropriate tab.
 */
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

    QXmppElement nickName;
    nickName.setTagName("nickName");
    nickName.setValue(rosterModel->ownName());
    x.appendChild(nickName);

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

void ChatShares::setRoster(ChatRosterModel *roster)
{
    rosterModel = roster;
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
            QXmppStanza::Error error(QXmppStanza::Error::Cancel, QXmppStanza::Error::ItemNotFound);
            responseIq.setError(error);
            responseIq.setType(QXmppIq::Error);
            client->sendPacket(responseIq);
            return;
        }
        logMessage(QXmppLogger::InformationMessage, "Sending file " + filePath);
        responseIq.setSid(generateStanzaHash());
        client->sendPacket(responseIq);

        // send file
        QXmppTransferJob *job = client->getTransferManager().sendFile(responseIq.to(), filePath, responseIq.sid());
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
        job->setData(LocalPathRole, filePath);
        uploadsView->addJob(job);
        return;
    }

    QXmppShareItem *queueItem = queueModel->get(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) &&
            Q(PacketId, Q::Equals, shareIq.id()));
    if (!queueItem)
        return;

    if (shareIq.type() == QXmppIq::Result)
    {
        queueItem->setData(PacketStart, QVariant());
        queueItem->setData(StreamId, shareIq.sid());
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        logMessage(QXmppLogger::WarningMessage, "Error requesting file " + queueItem->name() + " from " + shareIq.from());
        queueItem->setData(PacketId, QVariant());
        queueItem->setData(PacketStart, QVariant());
        queueItem->setData(TransferError, QXmppTransferJob::ProtocolError);
        queueModel->refreshItem(queueItem);
    }
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
    {
        db->search(shareIq);
        return;
    }

    // find target view(s)
    ChatSharesView *mainView = 0;
    if (searches.contains(shareIq.tag()))
        mainView = qobject_cast<ChatSharesView*>(searches.take(shareIq.tag()));
    else if (shareIq.type() == QXmppIq::Set && shareIq.from() == shareServer)
        mainView = sharesView;
    else
        return;

    QList<ChatSharesView*> views;
    views << mainView;
    if (mainView == downloadsView)
        views << sharesView;

    // FIXME : we are casting away constness
    QXmppShareItem *newItem = (QXmppShareItem*)&shareIq.collection();

    // update all concerned views
    foreach (ChatSharesView *view, views)
    {
        ChatSharesModel *model = qobject_cast<ChatSharesModel*>(view->model());
        QXmppShareItem *oldItem = model->get(Q_FIND_LOCATIONS(newItem->locations()),
                                             ChatSharesModel::QueryOptions(ChatSharesModel::PostRecurse));

        if (!oldItem && shareIq.from() != shareServer)
        {
            logMessage(QXmppLogger::WarningMessage, "Ignoring unwanted search result");
            continue;
        }

        if (shareIq.type() == QXmppIq::Error)
        {
            if ((shareIq.error().condition() == QXmppStanza::Error::ItemNotFound) && oldItem)
                model->removeItem(oldItem);
        } else {
            QModelIndex index = model->updateItem(oldItem, newItem);
            if (view == mainView)
                view->setExpanded(index, true);
        }
    }

    // if we retrieved the contents of a download queue item, process queue
    if (mainView == downloadsView)
        processDownloadQueue();
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

void ChatShares::tabChanged(int index)
{
    QWidget *tab = tabWidget->widget(index);
    if (tab == sharesView || tab == searchView)
        downloadButton->show();
    else
        downloadButton->hide();

    if (tab == downloadsWidget)
        removeButton->show();
    else
        removeButton->hide();
}

ChatSharesDelegate::ChatSharesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ChatSharesDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int error = index.data(TransferError).toInt();
    int done = index.data(TransferDone).toInt();
    int total = index.data(TransferTotal).toInt();
    QString localPath = index.data(TransferPath).toString();
    if (index.column() == ProgressColumn && done > 0 && total > 0 && !error && localPath.isEmpty())
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

QXmppShareItem *ChatSharesModel::addItem(const QXmppShareItem &item)
{
    QXmppShareItem *child = get(Q_FIND_LOCATIONS(item.locations()), QueryOptions(PostRecurse), rootItem);
    if (!child)
    {
       beginInsertRows(QModelIndex(), rootItem->size(), rootItem->size());
       child = rootItem->appendChild(item);
       endInsertRows();
   }
    return child;
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
    else if (role == Qt::DisplayRole && index.column() == ProgressColumn &&
             item->type() == QXmppShareItem::FileItem)
    {
        const QString localPath = item->data(TransferPath).toString();
        int done = item->data(TransferDone).toInt();
        QTime t = index.data(TransferStart).toTime();
        if (!localPath.isEmpty())
        {
            return tr("Downloaded");
        }
        else if (done > 0 && t.isValid() && t.elapsed())
        {
            int speed = (done * 1000.0) / t.elapsed();
            return ChatTransfers::sizeToString(speed) + "/s";
        }
        else if (!item->data(PacketId).toString().isEmpty())
            return tr("Requested");
        else if (item->data(TransferError).toInt())
            return tr("Failed");
        else
            return tr("Queued");
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

QList<QXmppShareItem *> ChatSharesModel::filter(const ChatSharesModelQuery &query, const ChatSharesModel::QueryOptions &options, QXmppShareItem *parent, int limit)
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

QXmppShareItem *ChatSharesModel::get(const ChatSharesModelQuery &query, const ChatSharesModel::QueryOptions &options, QXmppShareItem *parent)
{
    QList<QXmppShareItem*> match = filter(query, options, parent, 1);
    if (match.size())
        return match.first();
    return 0;
}

/** Return the title for the given column.
 */
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

void ChatSharesModel::refreshItem(QXmppShareItem *item)
{
    emit dataChanged(createIndex(item->row(), ProgressColumn, item), createIndex(item->row(), ProgressColumn, item));
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

QModelIndex ChatSharesModel::updateItem(QXmppShareItem *oldItem, QXmppShareItem *newItem)
{
    QDateTime stamp = QDateTime::currentDateTime();

    QModelIndex oldIndex;
    if (!oldItem)
        oldItem = rootItem;
    if (oldItem != rootItem)
        oldIndex = createIndex(oldItem->row(), 0, oldItem);

    oldItem->setFileHash(newItem->fileHash());
    oldItem->setFileSize(newItem->fileSize());
    oldItem->setLocations(newItem->locations());
    //oldItem->setName(newItem->name());
    oldItem->setType(newItem->type());
    if (oldItem->type() == QXmppShareItem::CollectionItem && oldItem->size() > 0)
        oldItem->setData(UpdateTime, stamp);

    QList<QXmppShareItem*> removed = oldItem->children();
    QList<QXmppShareItem*> added;
    foreach (QXmppShareItem *newChild, newItem->children())
    {
        QXmppShareItem *oldChild = get(Q_FIND_LOCATIONS(newChild->locations()), QueryOptions(DontRecurse), oldItem);
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
        {
            QXmppShareItem *item = oldItem->appendChild(*newChild);
            updateTime(item, stamp);
        }
        endInsertRows();
    }

    return oldIndex;
}

ChatSharesModelQuery::ChatSharesModelQuery()
    :  m_role(0), m_operation(None), m_combine(NoCombine)
{
}

ChatSharesModelQuery::ChatSharesModelQuery(int role, ChatSharesModelQuery::Operation operation, QVariant data)
    :  m_role(role), m_operation(operation), m_data(data), m_combine(NoCombine)
{
}

bool ChatSharesModelQuery::match(QXmppShareItem *item) const
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
        foreach (const ChatSharesModelQuery &child, m_children)
            if (!child.match(item))
                return false;
        return true;
    }
    else if (m_combine == OrCombine)
    {
        foreach (const ChatSharesModelQuery &child, m_children)
            if (child.match(item))
                return true;
        return false;
    }
    // empty query matches all
    return true;
}

ChatSharesModelQuery ChatSharesModelQuery::operator&&(const ChatSharesModelQuery &other) const
{
    ChatSharesModelQuery result;
    result.m_combine = AndCombine;
    result.m_children << *this << other;
    return result;
}

ChatSharesModelQuery ChatSharesModelQuery::operator||(const ChatSharesModelQuery &other) const
{
    ChatSharesModelQuery result;
    result.m_combine = OrCombine;
    result.m_children << *this << other;
    return result;
}

ChatSharesModel::QueryOptions::QueryOptions(Recurse recurse_)
    : recurse(recurse_)
{
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

void ChatSharesView::keyPressEvent(QKeyEvent *event)
{
    QModelIndex current = currentIndex();
    if (current.isValid())
    {
        switch (event->key())
        {
        case Qt::Key_Plus:
        case Qt::Key_Right:
            emit expandRequested(current);
            return;
        case Qt::Key_Minus:
        case Qt::Key_Left:
            collapse(current);
            return;
        }
    }
    QTreeView::keyPressEvent(event);
}

/** When the view is resized, adjust the width of the
 *  "name" column to take all the available space.
 */
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
