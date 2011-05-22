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

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
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
#include <QStatusBar>
#include <QStringList>
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
#include "chat_search.h"
#include "shares.h"
#include "shares/model.h"
#include "shares/options.h"
#include "shares/view.h"

static int parallelDownloadLimit = 2;

#define SIZE_COLUMN_WIDTH 80
#define PROGRESS_COLUMN_WIDTH 100
// display message in statusbar for 10 seconds
#define STATUS_TIMEOUT 10000
// keep directory listings for 10 seconds
#define REFRESH_INTERVAL 10
#define REGISTER_INTERVAL 60

// common queries
#define Q SharesModelQuery
#define Q_FIND_LOCATIONS(locations)  Q(QXmppShareItem::LocationsRole, Q::Equals, QVariant::fromValue(locations))
#define Q_FIND_TRANSFER(job) \
    (Q(QXmppShareItem::TypeRole, Q::Equals, QXmppShareItem::FileItem) && \
     Q(PacketId, Q::Equals, job->data(QXmppShareExtension::TransactionRole)))

/** Constructs a SharePanel.
 */
SharePanel::SharePanel(Chat *chat, QXmppShareDatabase *sharesDb, QWidget *parent)
    : ChatPanel(parent),
    chatWindow(chat),
    client(0),
    db(sharesDb)
{
    bool check;
    setWindowIcon(QIcon(":/share.png"));
    setWindowTitle(tr("Shares"));
    setWindowHelp(tr("You can select the folders you want to share with other users from the shares options."));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);

    // HEADER

    layout->addLayout(headerLayout());

    ChatSearchBar *searchBar = new ChatSearchBar;
    searchBar->setControlsVisible(false);
    searchBar->setDelay(500);
    searchBar->setText(tr("Enter the name of the file you are looking for."));
    check = connect(searchBar, SIGNAL(find(QString, QTextDocument::FindFlags, bool)),
                    this, SLOT(find(QString, QTextDocument::FindFlags, bool)));
    Q_ASSERT(check);
    layout->addWidget(searchBar);
    layout->addSpacing(4);

    // MAIN

    // shares view
    SharesModel *sharesModel = new SharesModel(this);
    sharesView = new SharesView;
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
    layout->addWidget(sharesView);

    // downloads help
    downloadsHelp = new QLabel;
    downloadsHelp->setMargin(10);
    downloadsHelp->setOpenExternalLinks(true);
    downloadsHelp->setWordWrap(true);
    layout->addWidget(downloadsHelp);

    // downloads view
    queueModel = new SharesModel(this);
    downloadsView = new SharesView;
    downloadsView->setModel(queueModel);
    check = connect(downloadsView, SIGNAL(doubleClicked(const QModelIndex&)),
                    this, SLOT(transferDoubleClicked(const QModelIndex&)));
    Q_ASSERT(check);

    check = connect(downloadsView, SIGNAL(expandRequested(QModelIndex)),
                    downloadsView, SLOT(expand(QModelIndex)));
    Q_ASSERT(check);

    layout->addWidget(downloadsView);

    // declarative
    QDeclarativeView *declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("window", chatWindow);

    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/SharePanel.qml"));
    layout->addWidget(declarativeView);

    // FOOTER

    QHBoxLayout *footerLayout = new QHBoxLayout;
    layout->addLayout(footerLayout);

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

    /* remove button */
    removeButton = new QPushButton(tr("Remove"));
    removeButton->setIcon(QIcon(":/remove.png"));
    check = connect(removeButton, SIGNAL(clicked()),
                    this, SLOT(transferRemoved()));
    Q_ASSERT(check);
    footerLayout->addWidget(removeButton);
    //removeButton->hide();

    setLayout(layout);

    /* connect signals */
    QXmppClient *baseClient = chatWindow->client();
    check = connect(this, SIGNAL(logMessage(QXmppLogger::MessageType, QString)),
                    baseClient, SIGNAL(logMessage(QXmppLogger::MessageType, QString)));
    Q_ASSERT(check);

    setClient(baseClient);

    check = connect(this, SIGNAL(findPanel()),
                    searchBar, SLOT(activate()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(findFinished(bool)),
                    searchBar, SLOT(findFinished(bool)));
    Q_ASSERT(check);

    //setFocusProxy(lineEdit);

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

    // add actions
    QAction *optionsAction = addAction(QIcon(":/options.png"), tr("Options"));
    check = connect(optionsAction, SIGNAL(triggered()),
                    this, SLOT(showOptions()));
    Q_ASSERT(check);

    action = chat->addAction(windowIcon(), windowTitle());
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_S));
    action->setVisible(false);
    connect(action, SIGNAL(triggered()),
            this, SIGNAL(showPanel()));
}

void SharePanel::directoryChanged(const QString &path)
{
    const QString sharesLink = QString("<a href=\"%1\">%2</a>").arg(
        QUrl::fromLocalFile(path).toString(),
        tr("downloads folder"));
    downloadsHelp->setText(tr("Received files are stored in your %1. Once a file is received, you can double click to open it.").arg(sharesLink));
}

/** When the main XMPP stream is disconnected, disconnect the shares-specific
 *  stream too.
 */
void SharePanel::disconnected()
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
void SharePanel::getFailed(const QString &packetId)
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
void SharePanel::transferAbort(QXmppShareItem *item)
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
void SharePanel::transferDestroyed(QObject *obj)
{
    downloadJobs.removeAll(static_cast<QXmppTransferJob*>(obj));
    processDownloadQueue();
}

/** When the user double clicks an item in the download queue,
 *  open it if it is fully retrieved.
 */
void SharePanel::transferDoubleClicked(const QModelIndex &index)
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
void SharePanel::transferFinished(QXmppTransferJob *job)
{
    QXmppShareItem *queueItem = queueModel->get(Q_FIND_TRANSFER(job));
    if (!queueItem)
        return;

    queueItem->setData(PacketId, QVariant());
    if (job->error() == QXmppTransferJob::NoError)
    {
        statusBar->showMessage(QString("%1 - %2").arg(tr("Downloaded"), queueItem->name()), STATUS_TIMEOUT);
        queueItem->setData(TransferPath, job->localFileUrl().toLocalFile());
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
void SharePanel::transferProgress(qint64 done, qint64 total)
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
void SharePanel::transferRemoved()
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

void SharePanel::find(const QString &needle, QTextDocument::FindFlags flags, bool changed)
{
    sharesFilter = needle;
    if (!needle.isEmpty() && needle.size() < 3)
    {
        emit findFinished(true);
        return;
    }

    // search for files
    QXmppShareExtension *extension = client->findExtension<QXmppShareExtension>();
    const QString requestId = extension->search(QXmppShareLocation(shareServer), 1, sharesFilter);
    if (!requestId.isEmpty())
        searches.insert(requestId, sharesView);
    else
        emit findFinished(false);
}

void SharePanel::transferStarted(QXmppTransferJob *job)
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

void SharePanel::indexFinished(double elapsed, int updated, int removed)
{
    statusBar->showMessage(tr("Indexed %1 files in %2s").arg(updated).arg(elapsed), STATUS_TIMEOUT);
}

void SharePanel::indexStarted()
{
    statusBar->showMessage(tr("Indexing files"), STATUS_TIMEOUT);
}

/** Add the selected items to the download queue.
 */
void SharePanel::downloadItem()
{
    foreach (const QModelIndex &index, sharesView->selectionModel()->selectedRows())
    {
        QXmppShareItem *item = static_cast<QXmppShareItem*>(index.internalPointer());
        if (item)
            queueItem(item);
    }
}

void SharePanel::queueItem(QXmppShareItem *item)
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
            SharesModel::QueryOptions(), queueItem);
    if (queueItem->type() == QXmppShareItem::CollectionItem && (!queueItem->size() || emptyChild))
    {
        if (queueItem->locations().isEmpty())
        {
            logMessage(QXmppLogger::WarningMessage, "No location for collection: " + item->name());
            return;
        }
        const QXmppShareLocation location = queueItem->locations().first();

        // retrieve full tree
        QXmppShareExtension *extension = client->findExtension<QXmppShareExtension>();
        const QString requestId = extension->search(location, 0, sharesFilter);
        if (!requestId.isEmpty())
            searches.insert(requestId, downloadsView);
    }

    // process the download queue
    processDownloadQueue();
}

void SharePanel::itemContextMenu(const QModelIndex &index, const QPoint &globalPos)
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
void SharePanel::itemDoubleClicked(const QModelIndex &index)
{
    SharesView *view = qobject_cast<SharesView*>(sender());
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
void SharePanel::itemExpandRequested(const QModelIndex &index)
{
    SharesView *view = qobject_cast<SharesView*>(sender());
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
    QXmppShareExtension *extension = client->findExtension<QXmppShareExtension>();
    const QString requestId = extension->search(location, 1, sharesFilter);
    if (!requestId.isEmpty())
        searches.insert(requestId, view);
}

void SharePanel::presenceReceived(const QXmppPresence &presence)
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
        QXmppTransferManager *transferManager = client->findExtension<QXmppTransferManager>();
        if (forceProxy == "1" && !transferManager->proxyOnly())
        {
            logMessage(QXmppLogger::InformationMessage, "Forcing SOCKS5 proxy");
            transferManager->setProxyOnly(true);
        }

        // activate the shares view
        sharesView->setEnabled(true);
        action->setVisible(true);

        // run one-time configuration
        QSettings settings;
        if (!settings.value("SharesConfigured").toBool())
        {
            settings.setValue("SharesConfigured", true);
            showOptions();
        }
    }
    else if (presence.type() == QXmppPresence::Error &&
        presence.error().type() == QXmppStanza::Error::Modify &&
        presence.error().condition() == QXmppStanza::Error::Redirect)
    {
        const QString domain = shareExtension.firstChildElement("domain").value();
        const QString server = shareExtension.firstChildElement("server").value();

        logMessage(QXmppLogger::InformationMessage, "Redirecting to " + domain + "," + server);

        // reconnect to another server
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

void SharePanel::processDownloadQueue()
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
        QXmppShareExtension *extension = client->findExtension<QXmppShareExtension>();
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

void SharePanel::setClient(QXmppClient *newClient)
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

void SharePanel::showOptions()
{
    chatWindow->showPreferences("shares");
}

void SharePanel::shareSearchIqReceived(const QXmppShareSearchIq &shareIq)
{
    if (shareIq.type() == QXmppIq::Get)
        return;

    // find target view(s)
    SharesView *view = 0;
    const QString requestId = shareIq.id();
    if ((shareIq.type() == QXmppIq::Result || shareIq.type() == QXmppIq::Error) && searches.contains(requestId))
        view = qobject_cast<SharesView*>(searches.take(requestId));
    else if (shareIq.type() == QXmppIq::Set && shareIq.from() == shareServer && sharesFilter.isEmpty())
        view = sharesView;
    else
        return;

    // FIXME : we are casting away constness
    QXmppShareItem *newItem = (QXmppShareItem*)&shareIq.collection();

    // update all concerned views
    SharesModel *model = qobject_cast<SharesModel*>(view->model());
    QXmppShareItem *oldItem = model->get(Q_FIND_LOCATIONS(newItem->locations()),
                                         SharesModel::QueryOptions(SharesModel::PostRecurse));

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
            view->setExpanded(index, true);
    }

    // send search feedback
    if (!oldItem && !sharesFilter.isEmpty())
       emit findFinished(newItem->size() > 0);

    // if we retrieved the contents of a download queue item, process queue
    if (view == downloadsView)
        processDownloadQueue();
}

void SharePanel::shareServerFound(const QString &server)
{
    // register with server
    shareServer = server;

    QXmppElement x;
    x.setTagName("x");
    x.setAttribute("xmlns", ns_shares);

    QXmppElement nickName;
    nickName.setTagName("nickName");
    nickName.setValue(chatWindow->rosterModel()->ownName());
    x.appendChild(nickName);

    QXmppPresence presence;
    presence.setTo(shareServer);
    presence.setExtensions(x);
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNone);
    client->sendPacket(presence);
}

// PLUGIN

class SharesPlugin : public ChatPlugin
{
public:
    SharesPlugin();
    void finalize(Chat *chat);
    bool initialize(Chat *chat);
    QString name() const { return "Shares"; };
    void preferences(ChatPreferences *prefs);

private:
    QXmppShareDatabase *db;
    QSet<Chat*> chats;
};

SharesPlugin::SharesPlugin()
    : db(0)
{
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

        // sanitize settings
        QSettings settings;
        QString sharesDirectory = settings.value("SharesLocation",  QDir::home().filePath("Public")).toString();
        if (sharesDirectory.endsWith("/"))
            sharesDirectory.chop(1);
        QStringList mappedDirectories = settings.value("SharesDirectories").toStringList();

        // create shares database
        db = new QXmppShareDatabase(this);
        db->setDirectory(sharesDirectory);
        db->setMappedDirectories(mappedDirectories);
    }
    chats << chat;

    /* register panel */
    SharePanel *shares = new SharePanel(chat, db);
    chat->addPanel(shares);
    return true;
}

void SharesPlugin::preferences(ChatPreferences *prefs)
{
    if (db)
        prefs->addTab(new SharesOptions(db));
}

Q_EXPORT_STATIC_PLUGIN2(shares, SharesPlugin)

