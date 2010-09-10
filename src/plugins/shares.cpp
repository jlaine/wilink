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
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QTimer>
#include <QUrl>

#include "QDjango.h"
#include "QXmppShareDatabase.h"
#include "QXmppShareExtension.h"
#include "QXmppShareIq.h"
#include "QXmppUtils.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "transfers.h"
#include "shares.h"
#include "shares/model.h"
#include "shares/view.h"
#include "systeminfo.h"

static int parallelDownloadLimit = 2;

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 100
// display message in statusbar for 10 seconds
#define STATUS_TIMEOUT 10000
// keep directory listings for 10 seconds
#define REFRESH_INTERVAL 10
#define REGISTER_INTERVAL 60

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

ChatShares::ChatShares(Chat *chat, QXmppShareDatabase *sharesDb, QWidget *parent)
    : ChatPanel(parent), chatWindow(chat), client(0), db(sharesDb), rosterModel(0),
    menuAction(0)
{
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("Shares"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);

    // HEADER

    layout->addItem(headerLayout());
    layout->addWidget(new QLabel(tr("Enter the name of the file you are looking for.")));
    layout->addSpacing(10);
    lineEdit = new QLineEdit;
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(findRemoteFiles()));
    connect(lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(queryStringChanged()));
    layout->addWidget(lineEdit);
    layout->addSpacing(10);

    // MAIN

    tabWidget = new QTabWidget;
    layout->addWidget(tabWidget);

    /* create main tab */
    ChatSharesModel *sharesModel = new ChatSharesModel(this);
    sharesView = new ChatSharesView;
    sharesView->setExpandsOnDoubleClick(false);
    sharesView->setModel(sharesModel);
    sharesView->hideColumn(ProgressColumn);
    connect(sharesView, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)), this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    connect(sharesView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(itemDoubleClicked(const QModelIndex&)));
    connect(sharesView, SIGNAL(expandRequested(QModelIndex)), this, SLOT(itemExpandRequested(QModelIndex)));

    sharesWidget = new ChatSharesTab;
    sharesWidget->addWidget(sharesView);
    tabWidget->addTab(sharesWidget, QIcon(":/album.png"), tr("Shares"));

    /* create search tab */
    ChatSharesModel *searchModel = new ChatSharesModel(this);
    searchView = new ChatSharesView;
    searchView->setExpandsOnDoubleClick(false);
    searchView->setModel(searchModel);
    searchView->hideColumn(ProgressColumn);
    connect(searchView, SIGNAL(contextMenu(QModelIndex, QPoint)), this, SLOT(itemContextMenu(QModelIndex, QPoint)));
    connect(searchView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemDoubleClicked(QModelIndex)));
    connect(searchView, SIGNAL(expandRequested(QModelIndex)), this, SLOT(itemExpandRequested(QModelIndex)));

    searchWidget = new ChatSharesTab;
    searchWidget->addWidget(searchView);
    tabWidget->addTab(searchWidget, QIcon(":/search.png"), tr("Search"));

    /* create queue tab */
    queueModel = new ChatSharesModel(this);
    downloadsView = new ChatSharesView;
    downloadsView->setModel(queueModel);
    connect(downloadsView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(transferDoubleClicked(const QModelIndex&)));
    connect(downloadsView, SIGNAL(expandRequested(QModelIndex)), downloadsView, SLOT(expand(QModelIndex)));

    downloadsWidget = new ChatSharesTab;
    downloadsWidget->addWidget(downloadsView);
    tabWidget->addTab(downloadsWidget, QIcon(":/download.png"), tr("Downloads"));

    /* create uploads tab */
    uploadsView = new ChatTransfersView;
    uploadsWidget = new ChatSharesTab;
    uploadsWidget->addWidget(uploadsView);
    tabWidget->addTab(uploadsWidget, QIcon(":/upload.png"), tr("Uploads"));

    // FOOTER

    QHBoxLayout *footerLayout = new QHBoxLayout;
    layout->addItem(footerLayout);

    statusBar = new QStatusBar;
    statusBar->setSizeGripEnabled(false);
    footerLayout->addWidget(statusBar);

    /* download button */
    downloadButton = new QPushButton(tr("Download"));
    downloadButton->setIcon(QIcon(":/download.png"));
    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadItem()));
    footerLayout->addWidget(downloadButton);

    /* rescan button */
    indexButton = new QPushButton(tr("Refresh my shares"));
    indexButton->setIcon(QIcon(":/refresh.png"));
    connect(indexButton, SIGNAL(clicked()), db, SLOT(index()));
    footerLayout->addWidget(indexButton);
    indexButton->hide();

    /* remove button */
    removeButton = new QPushButton(tr("Remove"));
    removeButton->setIcon(QIcon(":/remove.png"));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(transferRemoved()));
    footerLayout->addWidget(removeButton);
    removeButton->hide();

    setLayout(layout);

    /* connect signals */
    ChatClient *baseClient = chatWindow->client();
    registerTimer = new QTimer(this);
    registerTimer->setInterval(REGISTER_INTERVAL * 1000);
    connect(registerTimer, SIGNAL(timeout()), this, SLOT(registerWithServer()));
    connect(this, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
        baseClient, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    setClient(baseClient);

    connect(this, SIGNAL(findPanel()), lineEdit, SLOT(setFocus()));
    setFocusProxy(lineEdit);

    /* database signals */
    connect(db, SIGNAL(directoryChanged(QString)),
        this, SLOT(directoryChanged(QString)));
    connect(db, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
        baseClient, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    connect(db, SIGNAL(getFinished(QXmppShareGetIq, QXmppShareItem)),
        this, SLOT(getFinished(QXmppShareGetIq, QXmppShareItem)));
    connect(db, SIGNAL(indexStarted()),
        this, SLOT(indexStarted()));
    connect(db, SIGNAL(indexFinished(double, int, int)),
        this, SLOT(indexFinished(double, int, int)));
    directoryChanged(db->directory());
}

void ChatShares::directoryChanged(const QString &path)
{
    const QString sharesLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(path).toString(),
        SystemInfo::displayName(SystemInfo::SharesLocation));
    const QString helpText = tr("To share files with other users, simply place them in your %1 folder.").arg(sharesLink);

    sharesWidget->setText(helpText);
    searchWidget->setText(helpText);
    downloadsWidget->setText(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(sharesLink));
    uploadsWidget->setText(helpText);
}

/** When the main XMPP stream is disconnected, disconnect the shares-specific
 *  stream too.
 */
void ChatShares::disconnected()
{
    ChatClient *baseClient = chatWindow->client();
    if (client && client != baseClient && QObject::sender() == baseClient)
    {
        shareServer = "";
        client->disconnectFromServer();
        client->deleteLater();
        client = baseClient;
    }
    sharesView->setEnabled(false);
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
        qint64 oldDone = queueItem->data(TransferPainted).toLongLong();
        if ((done - oldDone) >= total/100)
        {
            queueItem->setData(TransferPainted, done);
            queueModel->refreshItem(queueItem);
        }
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
    QDir downloadsDir(db->directory());
    if (!subdir.isEmpty())
    {
        if (downloadsDir.exists(subdir) || downloadsDir.mkpath(subdir))
            downloadsDir.setPath(downloadsDir.filePath(subdir));
    }

    // determine file name
    const QString filePath = ChatTransfers::availableFilePath(downloadsDir.path(), job->fileName() + ".part");
    QFile *file = new QFile(filePath, job);
    if (!file->open(QIODevice::WriteOnly))
    {
        logMessage(QXmppLogger::WarningMessage, "Could not open file for writing: " + filePath);
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
    statusBar->showMessage(QString("%1 - %2").arg(tr("Transfer"), queueItem->name()), STATUS_TIMEOUT);
    job->accept(file);
}

/** When the user removes items from the download queue, cancel any jobs
 *  associated with them.
 */
void ChatShares::transferRemoved()
{
    foreach (const QModelIndex &index, downloadsView->selectionModel()->selectedRows())
    {
        QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
        if (index.isValid() && item)
        {
            transferAbort(item);
            queueModel->removeItem(item);
        }
    }
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
                statusBar->showMessage(QString("%1 - %2").arg(tr("Downloaded"), queueItem->name()), STATUS_TIMEOUT);

                // rename file
                QFileInfo tempInfo(localPath);
                QDir tempDir(tempInfo.dir());
                QString finalPath = ChatTransfers::availableFilePath(tempDir.path(), tempInfo.fileName().remove(QRegExp("\\.part$")));
                tempDir.rename(tempInfo.fileName(), QFileInfo(finalPath).fileName());
                queueItem->setData(TransferPath, finalPath);
                queueItem->setData(TransferError, QVariant());

                // store to shares database
                File cached;
                cached.setPath(db->fileNode(finalPath));
                cached.setSize(job->fileSize());
                cached.setHash(job->fileHash());
                cached.setDate(QFileInfo(finalPath).lastModified());
                cached.save();

            } else {
                statusBar->showMessage(QString("%1 - %2").arg(tr("Failed"), queueItem->name()), STATUS_TIMEOUT);
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

    if (!search.isEmpty())
        statusBar->showMessage(tr("Searching for \"%1\"").arg(search), STATUS_TIMEOUT);
}

void ChatShares::getFinished(const QXmppShareGetIq &iq, const QXmppShareItem &shareItem)
{
    // check the IQ the response is for this connection
    if (iq.from() != client->configuration().jid())
        return;

    QXmppShareGetIq responseIq(iq);

    // FIXME: for some reason, random number generation in thread is broken
    if (responseIq.type() != QXmppIq::Error)
        responseIq.setSid(generateStanzaHash());
    client->sendPacket(responseIq);

    // send file
    if (responseIq.type() != QXmppIq::Error)
    {
        QString filePath = db->filePath(shareItem.locations()[0].node());
        QXmppTransferFileInfo fileInfo;
        fileInfo.setName(shareItem.name());
        fileInfo.setDate(shareItem.fileDate());
        fileInfo.setHash(shareItem.fileHash());
        fileInfo.setSize(shareItem.fileSize());

        logMessage(QXmppLogger::InformationMessage, "Sending file: " + filePath);

        QFile *file = new QFile(filePath);
        file->open(QIODevice::ReadOnly);
        QXmppTransferJob *job = client->transferManager().sendFile(responseIq.to(), file, fileInfo, responseIq.sid());
        file->setParent(job);
        connect(job, SIGNAL(finished()), job, SLOT(deleteLater()));
        job->setData(LocalPathRole, filePath);
        uploadsView->addJob(job);
    }
}

void ChatShares::indexFinished(double elapsed, int updated, int removed)
{
    statusBar->showMessage(tr("Indexed %1 files in %2s").arg(updated).arg(elapsed), STATUS_TIMEOUT);
    indexButton->setEnabled(true);
}

void ChatShares::indexStarted()
{
    statusBar->showMessage(tr("Indexing files"), STATUS_TIMEOUT);
    indexButton->setEnabled(false);
}

/** Add the selected items to the download queue.
 */
void ChatShares::downloadItem()
{
    // determine current view
    ChatSharesView *treeWidget;
    if (tabWidget->currentWidget() == sharesWidget)
        treeWidget = sharesView;
    else if (tabWidget->currentWidget() == searchWidget)
        treeWidget = searchView;
    else
        return;

    foreach (const QModelIndex &index, treeWidget->selectionModel()->selectedRows())
    {
        QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
        if (item)
            queueItem(item);
    }
}

void ChatShares::queueItem(QXmppShareItem *item)
{
    // check item is not from local collection
    foreach (const QXmppShareLocation &location, item->locations())
        if (location.jid() == client->configuration().jid())
            return;

    // check item is not already in the queue
    if (queueModel->get(Q_FIND_LOCATIONS(item->locations())))
        return;

    QXmppShareItem *queueItem = queueModel->addItem(*item);
    statusBar->showMessage(QString("%1 - %2").arg(tr("Queued"), queueItem->name()), STATUS_TIMEOUT);

    // if we have at least one empty child, we need to retrieve the children
    // of the item we just queued
    QXmppShareItem *emptyChild = queueModel->get(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::CollectionItem) &&
            Q(QXmppShareItem::SizeRole, Q::Equals, 0),
            ChatSharesModel::QueryOptions(), queueItem);
    if (queueItem->type() == QXmppShareItem::CollectionItem && (!queueItem->size() || emptyChild))
    {
        if (queueItem->locations().isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "No location for collection: " + item->name());
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

    if (item->type() == QXmppShareItem::CollectionItem)
    {
        if (view->isExpanded(index))
            view->collapse(index);
        else
            itemExpandRequested(index);
    }
    else if (item->type() == QXmppShareItem::FileItem)
    {
        queueItem(item);
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
        logMessage(QXmppLogger::WarningMessage, "No location for collection: " + item->name());
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

    if (presence.type() == QXmppPresence::Available)
    {
        const QString forceProxy = shareExtension.firstChildElement("force-proxy").value();
        if (forceProxy == "1" && !client->transferManager().proxyOnly())
        {
            logMessage(QXmppLogger::InformationMessage, "Forcing SOCKS5 proxy");
            client->transferManager().setProxyOnly(true);
        }

        // add entries to options menu
        if (!menuAction)
        {
            menuAction = chatWindow->optionsMenu()->addAction(tr("Shares folder"));
            connect(menuAction, SIGNAL(triggered(bool)), this, SLOT(shareFolder()));
        }

        // activate the shares view
        sharesView->setEnabled(true);

        emit registerPanel();
    }
    else if (presence.type() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString domain = shareExtension.firstChildElement("domain").value();
        const QString server = shareExtension.firstChildElement("server").value();

        logMessage(QXmppLogger::InformationMessage, "Redirecting to " + domain + "," + server);

        // reconnect to another server
        registerTimer->stop();

        ChatClient *baseClient = chatWindow->client();
        QXmppConfiguration config = baseClient->configuration();
        config.setDomain(domain);
        config.setHost(server);

        ChatClient *newClient = new ChatClient(this);
        connect(&newClient->transferManager(), SIGNAL(fileReceived(QXmppTransferJob*)),
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

    const QDateTime currentTime = QDateTime::currentDateTime();
    foreach (QXmppShareItem *queueItem, active)
    {
        QDateTime timeoutTime = queueItem->data(PacketTimeout).toDateTime();
        if (timeoutTime.isValid() && timeoutTime <= currentTime)
        {
            logMessage(QXmppLogger::WarningMessage, "Request timed out for file: " + queueItem->name());
            queueItem->setData(PacketId, QVariant());
            queueItem->setData(PacketTimeout, QVariant());
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
                location.jid() != client->configuration().jid() &&
                !location.node().isEmpty())
            {
                locationFound = true;
                break;
            }
        }
        if (!locationFound)
        {
            logMessage(QXmppLogger::WarningMessage, "No location found for file: " + file->name());
            queueModel->removeItem(file);
            continue;
        }

        // request file
        QXmppShareGetIq iq;
        iq.setTo(location.jid());
        iq.setType(QXmppIq::Get);
        iq.setNode(location.node());
        client->sendPacket(iq);

        // allow hashing speeds as low as 10MB/s
        const int maxSeconds = qMax(60, int(file->fileSize() / 10000000));
        file->setData(PacketId, iq.id());
        file->setData(PacketTimeout, QDateTime::currentDateTime().addSecs(maxSeconds));
        QTimer::singleShot(maxSeconds * 1000, this, SLOT(processDownloadQueue()));

        queueModel->refreshItem(file);

        activeDownloads++;
    }
}

/** When the user changes the query string, switch to the appropriate tab.
 */
void ChatShares::queryStringChanged()
{
    if (lineEdit->text().isEmpty())
        tabWidget->setCurrentWidget(sharesWidget);
    else
        tabWidget->setCurrentWidget(searchWidget);
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
    bool check = connect(client, SIGNAL(disconnected()),
        this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
        this, SLOT(presenceReceived(const QXmppPresence&)));
    Q_ASSERT(check);

    // add shares extension
    QXmppShareExtension *extension = new QXmppShareExtension(db);
    client->addExtension(extension);

    check = connect(extension, SIGNAL(shareGetIqReceived(const QXmppShareGetIq&)),
        this, SLOT(shareGetIqReceived(const QXmppShareGetIq&)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(shareSearchIqReceived(const QXmppShareSearchIq&)),
        this, SLOT(shareSearchIqReceived(const QXmppShareSearchIq&)));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(shareServerFound(const QString&)),
        this, SLOT(shareServerFound(const QString&)));
    Q_ASSERT(check);
}

void ChatShares::setRoster(ChatRosterModel *roster)
{
    rosterModel = roster;
}

void ChatShares::shareFolder()
{
    QFileDialog *dialog = new QFileDialog;
    dialog->setDirectory(db->directory());
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setWindowTitle(tr("Shares folder"));
    dialog->show();

    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(deleteLater()));
    connect(dialog, SIGNAL(fileSelected(QString)), this, SLOT(shareFolderSelected(QString)));
}

void ChatShares::shareFolderSelected(const QString &path)
{
    // remember directory
    QSettings settings;
    settings.setValue("SharesLocation", path);

    db->setDirectory(path);
}

void ChatShares::shareGetIqReceived(const QXmppShareGetIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
    {
        db->get(shareIq);
        return;
    }

    QXmppShareItem *queueItem = queueModel->get(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) &&
            Q(PacketId, Q::Equals, shareIq.id()));
    if (!queueItem)
        return;

    if (shareIq.type() == QXmppIq::Result)
    {
        queueItem->setData(PacketTimeout, QVariant());
        queueItem->setData(StreamId, shareIq.sid());
    }
    else if (shareIq.type() == QXmppIq::Error)
    {
        logMessage(QXmppLogger::WarningMessage, "Error requesting file " + queueItem->name() + " from " + shareIq.from());
        queueItem->setData(PacketId, QVariant());
        queueItem->setData(PacketTimeout, QVariant());
        queueItem->setData(TransferError, QXmppTransferJob::ProtocolError);
        queueModel->refreshItem(queueItem);
    }
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
        return;

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
            if (newItem->size())
                statusBar->clearMessage();
            else
                statusBar->showMessage(tr("No files found"), 3000);
            if (view == mainView)
            {
                // when the search view receives results and there are less than 10 results
                // expand one level of folders
                if (view == searchView && newItem->size() < 10)
                    view->expandToDepth(1);
                // otherwise just expand the added item
                else
                    view->setExpanded(index, true);
            }
        }
    }

    // if we retrieved the contents of a download queue item, process queue
    if (mainView == downloadsView)
        processDownloadQueue();
}

void ChatShares::shareServerFound(const QString &server)
{
    // register with server
    shareServer = server;
    registerWithServer();
    registerTimer->start();
}

void ChatShares::tabChanged(int index)
{
    QWidget *tab = tabWidget->widget(index);
    if (tab == sharesWidget || tab == searchWidget)
        downloadButton->show();
    else
        downloadButton->hide();

    if (tab == downloadsWidget)
        removeButton->show();
    else
        removeButton->hide();

    if (tab == uploadsWidget)
        indexButton->show();
    else
        indexButton->hide();
}

ChatSharesTab::ChatSharesTab(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin(5);

    label = new QLabel;
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);
    vbox->addWidget(label);

    setLayout(vbox);
}

void ChatSharesTab::addWidget(QWidget *widget)
{
    layout()->addWidget(widget);
}

void ChatSharesTab::setText(const QString &text)
{
    label->setText(text);
}

// PLUGIN

class SharesPlugin : public ChatPlugin
{
public:
    SharesPlugin();
    bool initialize(Chat *chat);
    void finalize(Chat *chat);

private:
    QXmppShareDatabase *db;
    QSet<Chat*> chats;
};

SharesPlugin::SharesPlugin()
    : db(0)
{
}

bool SharesPlugin::initialize(Chat *chat)
{
    /* initialise database */
    if (!db)
    {
        const QString databaseName = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).filePath("database.sqlite");
        QSqlDatabase sharesDb = QSqlDatabase::addDatabase("QSQLITE");
        sharesDb.setDatabaseName(databaseName);
        Q_ASSERT(sharesDb.open());
        QDjango::setDatabase(sharesDb);
        // drop wiLink <= 0.9.4 table
        sharesDb.exec("DROP TABLE files");

        QSettings settings;
        db = new QXmppShareDatabase(this);
        db->setDirectory(settings.value("SharesLocation", SystemInfo::storageLocation(SystemInfo::SharesLocation)).toString());
    }
    chats << chat;

    /* register panel */
    ChatShares *shares = new ChatShares(chat, db);
    shares->setObjectName("shares");
    shares->setRoster(chat->rosterModel());
    chat->addPanel(shares);

    /* register shortcut */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_S), chat);
    connect(shortcut, SIGNAL(activated()), shares, SIGNAL(showPanel()));
    return true;
}

void SharesPlugin::finalize(Chat *chat)
{
    /* cleanup database */
    chats.remove(chat);
    if (db && chats.isEmpty())
    {
        delete db;
        db = 0;
    }
}

Q_EXPORT_STATIC_PLUGIN2(shares, SharesPlugin)

