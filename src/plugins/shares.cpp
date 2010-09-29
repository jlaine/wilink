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
#include "shares/options.h"
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
#define Q_FIND_TRANSFER(job) \
    (Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) && \
     Q(PacketId, Q::Equals, job->data(QXmppShareExtension::TransactionRole)))

ChatShares::ChatShares(Chat *chat, QXmppShareDatabase *sharesDb, QWidget *parent)
    : ChatPanel(parent), chatWindow(chat), client(0), db(sharesDb), rosterModel(0),
    menuAction(0)
{
    bool check;
    setWindowIcon(QIcon(":/album.png"));
    setWindowTitle(tr("Shares"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);

    // HEADER

    layout->addItem(headerLayout());
    layout->addWidget(new QLabel(tr("Enter the name of the file you are looking for.")));
    layout->addSpacing(4);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->setSpacing(6);

    QLabel *findIcon = new QLabel;
    findIcon->setPixmap(QPixmap(":/search.png").scaled(16, 16));
    hbox->addWidget(findIcon);
    lineEdit = new QLineEdit;
    check = connect(lineEdit, SIGNAL(returnPressed()),
                    this, SLOT(findRemoteFiles()));
    Q_ASSERT(check);
    check = connect(lineEdit, SIGNAL(textChanged(const QString&)),
                    this, SLOT(queryStringChanged()));
    Q_ASSERT(check);
    hbox->addWidget(lineEdit);

    layout->addItem(hbox);
    layout->addSpacing(4);

    // MAIN

    tabWidget = new QTabWidget;
    layout->addWidget(tabWidget);

    /* create main tab */
    ChatSharesModel *sharesModel = new ChatSharesModel(this);
    sharesView = new ChatSharesView;
    sharesView->setExpandsOnDoubleClick(false);
    sharesView->setModel(sharesModel);
    sharesView->hideColumn(ProgressColumn);
    check = connect(sharesView, SIGNAL(contextMenu(const QModelIndex&, const QPoint&)),
                    this, SLOT(itemContextMenu(const QModelIndex&, const QPoint&)));
    Q_ASSERT(check);

    check = connect(sharesView, SIGNAL(doubleClicked(const QModelIndex&)),
                    this, SLOT(itemDoubleClicked(const QModelIndex&)));
    Q_ASSERT(check);

    check = connect(sharesView, SIGNAL(expandRequested(QModelIndex)),
                    this, SLOT(itemExpandRequested(QModelIndex)));
    Q_ASSERT(check);

    sharesWidget = new ChatSharesTab;
    sharesWidget->addWidget(sharesView);
    tabWidget->addTab(sharesWidget, QIcon(":/album.png"), tr("Shares"));

    /* create queue tab */
    queueModel = new ChatSharesModel(this);
    downloadsView = new ChatSharesView;
    downloadsView->setModel(queueModel);
    check = connect(downloadsView, SIGNAL(doubleClicked(const QModelIndex&)),
                    this, SLOT(transferDoubleClicked(const QModelIndex&)));
    Q_ASSERT(check);

    check = connect(downloadsView, SIGNAL(expandRequested(QModelIndex)),
                    downloadsView, SLOT(expand(QModelIndex)));
    Q_ASSERT(check);

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
    check = connect(downloadButton, SIGNAL(clicked()),
                    this, SLOT(downloadItem()));
    Q_ASSERT(check);
    footerLayout->addWidget(downloadButton);

    /* rescan button */
    indexButton = new QPushButton(tr("Refresh my shares"));
    indexButton->setIcon(QIcon(":/refresh.png"));
    check = connect(indexButton, SIGNAL(clicked()),
                    db, SLOT(index()));
    Q_ASSERT(check);
    footerLayout->addWidget(indexButton);
    indexButton->hide();

    /* remove button */
    removeButton = new QPushButton(tr("Remove"));
    removeButton->setIcon(QIcon(":/remove.png"));
    check = connect(removeButton, SIGNAL(clicked()),
                    this, SLOT(transferRemoved()));
    Q_ASSERT(check);
    footerLayout->addWidget(removeButton);
    removeButton->hide();

    setLayout(layout);

    /* connect signals */
    QXmppClient *baseClient = chatWindow->client();
    registerTimer = new QTimer(this);
    registerTimer->setInterval(REGISTER_INTERVAL * 1000);
    check = connect(registerTimer, SIGNAL(timeout()),
                    this, SLOT(registerWithServer()));
    Q_ASSERT(check);

    searchTimer = new QTimer(this);
    searchTimer->setInterval(300);
    searchTimer->setSingleShot(true);
    check = connect(searchTimer, SIGNAL(timeout()),
                    this, SLOT(findRemoteFiles()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    baseClient, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    check = connect(tabWidget, SIGNAL(currentChanged(int)),
                    this, SLOT(tabChanged(int)));
    Q_ASSERT(check);

    setClient(baseClient);

    check = connect(this, SIGNAL(findPanel()),
                    lineEdit, SLOT(setFocus()));
    Q_ASSERT(check);

    setFocusProxy(lineEdit);

    /* database signals */
    check = connect(db, SIGNAL(directoryChanged(QString)),
                    this, SLOT(directoryChanged(QString)));
    Q_ASSERT(check);

    check = connect(db, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    baseClient, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    check = connect(db, SIGNAL(indexStarted()),
                    this, SLOT(indexStarted()));
    Q_ASSERT(check);

    check = connect(db, SIGNAL(indexFinished(double, int, int)),
                    this, SLOT(indexFinished(double, int, int)));
    Q_ASSERT(check);

    directoryChanged(db->directory());
}

void ChatShares::directoryChanged(const QString &path)
{
    const QString sharesLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(path).toString(),
        SystemInfo::displayName(SystemInfo::SharesLocation));
    const QString helpText = tr("To share files with other users, simply place them in your %1 folder.").arg(sharesLink);

    sharesWidget->setText(helpText);
    downloadsWidget->setText(tr("Received files are stored in your %1 folder. Once a file is received, you can double click to open it.").arg(sharesLink));
    uploadsWidget->setText(helpText);
}

/** When the main XMPP stream is disconnected, disconnect the shares-specific
 *  stream too.
 */
void ChatShares::disconnected()
{
    QXmppClient *baseClient = chatWindow->client();
    if (client && client != baseClient && QObject::sender() == baseClient)
    {
        shareServer = "";
        client->disconnectFromServer();
        client->deleteLater();
        client = baseClient;
    }
    sharesView->setEnabled(false);
}

/** When a file get fails, updated the associated download queue item.
 */
void ChatShares::getFailed(const QString &packetId)
{
    QXmppShareItem *queueItem = queueModel->get(
            Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) &&
            Q(PacketId, Q::Equals, packetId));
    if (!queueItem)
        return;

    logMessage(QXmppLogger::WarningMessage, "Error requesting file " + queueItem->name());
    queueItem->setData(PacketId, QVariant());
    queueItem->setData(TransferError, QXmppTransferJob::ProtocolError);
    queueModel->refreshItem(queueItem);

    // process download queue
    processDownloadQueue();
}

/** Recursively cancel any transfer jobs associated with a download queue item.
 */
void ChatShares::transferAbort(QXmppShareItem *item)
{
    const QVariant packetId = item->data(PacketId);
    foreach (QXmppTransferJob *job, downloadJobs)
    {
        if (job->data(QXmppShareExtension::TransactionRole) == packetId)
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

/** When a transfer finishes, update the queue entry.
 *
 * @param job
 */
void ChatShares::transferFinished(QXmppTransferJob *job)
{
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job));
    if (!queueItem)
        return;

    queueItem->setData(PacketId, QVariant());
    if (job->error() == QXmppTransferJob::NoError)
    {
        statusBar->showMessage(QString("%1 - %2").arg(tr("Downloaded"), queueItem->name()), STATUS_TIMEOUT);
        queueItem->setData(TransferPath, job->data(QXmppShareExtension::LocalPathRole));
        queueItem->setData(TransferError, QVariant());
    } else {
        statusBar->showMessage(QString("%1 - %2").arg(tr("Failed"), queueItem->name()), STATUS_TIMEOUT);
        queueItem->setData(TransferPath, QVariant());
        queueItem->setData(TransferError, job->error());
    }
    queueModel->refreshItem(queueItem);

    // process the download queue
    processDownloadQueue();
}

/** Update the progress bar for a transfer job.
 */
void ChatShares::transferProgress(qint64 done, qint64 total)
{
    QXmppTransferJob *job = qobject_cast<QXmppTransferJob*>(sender());
    if (!job)
        return;
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job));
    if (queueItem)
    {
        // update progress
        queueItem->setData(TransferDone, done);
        queueItem->setData(TransferTotal, total);
        queueItem->setData(TransferSpeed, job->speed());
        qint64 oldDone = queueItem->data(TransferPainted).toLongLong();
        if ((done - oldDone) >= total/100)
        {
            queueItem->setData(TransferPainted, done);
            queueModel->refreshItem(queueItem);
        }
    }
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

void ChatShares::findRemoteFiles()
{
    sharesFilter = lineEdit->text();
    qobject_cast<ChatSharesModel*>(sharesView->model())->clear();

    // search for files
    QXmppShareExtension *extension = client->findExtension<QXmppShareExtension*>();
    const QString requestId = extension->search(QXmppShareLocation(shareServer), 1, sharesFilter);
    if (!requestId.isEmpty())
    {
        searches.insert(requestId, sharesView);
        if (!sharesFilter.isEmpty())
            statusBar->showMessage(tr("Searching for \"%1\"").arg(sharesFilter), STATUS_TIMEOUT);
    }
}

void ChatShares::transferStarted(QXmppTransferJob *job)
{
    if (job->direction() == QXmppTransferJob::OutgoingDirection)
        uploadsView->addJob(job);
    else
    {
        QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job));
        if (!queueItem)
            return;

        // add transfer to list
        bool check;
        check = connect(job, SIGNAL(destroyed(QObject*)),
                        this, SLOT(transferDestroyed(QObject*)));
        Q_ASSERT(check);

        check = connect(job, SIGNAL(progress(qint64, qint64)),
                        this, SLOT(transferProgress(qint64,qint64)));
        Q_ASSERT(check);

        downloadJobs.append(job);

        // update status
        statusBar->showMessage(QString("%1 - %2").arg(tr("Transfer"), queueItem->name()), STATUS_TIMEOUT);
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
    if (tabWidget->currentWidget() != sharesWidget)
        return;

    foreach (const QModelIndex &index, sharesView->selectionModel()->selectedRows())
    {
        QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
        if (item)
            queueItem(item);
    }
}

void ChatShares::queueItem(QXmppShareItem *item)
{
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
        QXmppShareExtension *extension = client->findExtension<QXmppShareExtension*>();
        const QString requestId = extension->search(location, 0, sharesFilter);
        if (!requestId.isEmpty())
            searches.insert(requestId, downloadsView);
    }

    // process the download queue
    processDownloadQueue();
}

void ChatShares::itemContextMenu(const QModelIndex &index, const QPoint &globalPos)
{
    QMenu *menu = new QMenu(this);

    QAction *action = menu->addAction(tr("Download"));
    action->setIcon(QIcon(":/download.png"));

    bool check;
    check = connect(action, SIGNAL(triggered()),
                    this, SLOT(downloadItem()));
    Q_ASSERT(check);

    menu->popup(globalPos);
}

/** When the user double clicks on a node, expand it if it's a collection,
 * otherwise addd it to the download queue.
 *
 * @param index
 */
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
 *
 * @param index
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
    if (updateTime.isValid() && updateTime >= cutoffTime)
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
    QXmppShareExtension *extension = client->findExtension<QXmppShareExtension*>();
    const QString requestId = extension->search(location, 1, sharesFilter);
    if (!requestId.isEmpty())
        searches.insert(requestId, view);
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
            menuAction = chatWindow->optionsMenu()->addAction(QIcon(":/album.png"), tr("Shares options"));
            connect(menuAction, SIGNAL(triggered(bool)),
                    this, SLOT(shareFolder()));
        }

        // activate the shares view
        sharesView->setEnabled(true);

        // add roster entry
        rosterModel->addItem(objectType(),
            objectName(),
            windowTitle(),
            windowIcon(), rosterModel->findItem("home"));
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

        QXmppClient *baseClient = chatWindow->client();
        QXmppConfiguration config = baseClient->configuration();
        config.setDomain(domain);
        config.setHost(server);

        QXmppClient *newClient = new ChatClient(this);
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

        // request file
        QXmppShareExtension *extension = client->findExtension<QXmppShareExtension*>();
        const QString id = extension->get(*file);
        if (id.isEmpty())
        {
            queueModel->removeItem(file);
            continue;
        }
        file->setData(PacketId, id);
        queueModel->refreshItem(file);

        activeDownloads++;
    }
}

/** When the user changes the query string, switch to the appropriate tab.
 */
void ChatShares::queryStringChanged()
{
    tabWidget->setCurrentWidget(sharesWidget);
    const QString queryString  = lineEdit->text();
    if (queryString.isEmpty() || queryString.size() >= 3)
        searchTimer->start();
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
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNone);
    client->sendPacket(presence);
}

void ChatShares::setClient(QXmppClient *newClient)
{
    client = newClient;
    bool check = connect(client, SIGNAL(disconnected()),
        this, SLOT(disconnected()));
    Q_ASSERT(check);

    check = connect(client, SIGNAL(presenceReceived(const QXmppPresence&)),
        this, SLOT(presenceReceived(const QXmppPresence&)));
    Q_ASSERT(check);


    // add shares extension
    QXmppShareExtension *extension = new QXmppShareExtension(client, db);
    client->addExtension(extension);

    check = connect(extension, SIGNAL(getFailed(QString)),
                    this, SLOT(getFailed(QString)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(transferFinished(QXmppTransferJob*)),
                    this, SLOT(transferFinished(QXmppTransferJob*)));
    Q_ASSERT(check);

    check = connect(extension, SIGNAL(transferStarted(QXmppTransferJob*)),
                    this, SLOT(transferStarted(QXmppTransferJob*)));
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
    ChatSharesOptions *dialog = new ChatSharesOptions(db);

    bool check;
    check = connect(dialog, SIGNAL(finished(int)),
                    dialog, SLOT(deleteLater()));
    Q_ASSERT(check);
    Q_UNUSED(check);

#ifdef WILINK_EMBEDDED
    dialog->showMaximized();
#else
    dialog->show();
#endif
}

void ChatShares::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
        return;

    // find target view(s)
    ChatSharesView *view = 0;
    const QString requestId = shareIq.id();
    if ((shareIq.type() == QXmppIq::Result || shareIq.type() == QXmppIq::Error) && searches.contains(requestId))
        view = qobject_cast<ChatSharesView*>(searches.take(requestId));
    else if (shareIq.type() == QXmppIq::Set && shareIq.from() == shareServer && sharesFilter.isEmpty())
        view = sharesView;
    else
        return;

    // FIXME : we are casting away constness
    QXmppShareItem *newItem = (QXmppShareItem*)&shareIq.collection();

    // update all concerned views
    ChatSharesModel *model = qobject_cast<ChatSharesModel*>(view->model());
    QXmppShareItem *oldItem = model->get(Q_FIND_LOCATIONS(newItem->locations()),
                                         ChatSharesModel::QueryOptions(ChatSharesModel::PostRecurse));

    if (!oldItem && shareIq.from() != shareServer)
    {
        logMessage(QXmppLogger::WarningMessage, "Ignoring unwanted search result");
        return;
    }

    if (shareIq.type() == QXmppIq::Error)
    {
        if ((shareIq.error().condition() == QXmppStanza::Error::ItemNotFound) && oldItem)
            model->removeItem(oldItem);
    } else {
        QModelIndex index = model->updateItem(oldItem, newItem);
        if (newItem->size())
        {
            statusBar->clearMessage();
            view->setExpanded(index, true);
        }
        else
        {
            statusBar->showMessage(tr("No files found"), 3000);
        }
    }

    // if we retrieved the contents of a download queue item, process queue
    if (view == downloadsView)
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
    if (tab == sharesWidget)
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
        db->setMappedDirectories(settings.value("SharesDirectories").toStringList());
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
    connect(shortcut, SIGNAL(activated()),
            shares, SIGNAL(showPanel()));
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

