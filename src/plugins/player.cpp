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

#undef USE_DECLARATIVE
#ifdef USE_DECLARATIVE
#include <QDeclarativeContext>
#include <QDeclarativeView>
#endif
#include <QAbstractNetworkCache>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmapCache>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>

#include "QSoundFile.h"
#include "QSoundPlayer.h"

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "player.h"

#define DURATION_WIDTH 30
#define PLAYER_ROSTER_ID "0_player"

enum PlayerColumns {
    ArtistColumn = 0,
    TitleColumn,
    DurationColumn,
    MaxColumn
};

enum PlayerRole {
    AlbumRole = Qt::UserRole,
    ArtistRole,
    DownloadingRole,
    DurationRole,
    ImageUrlRole,
    PlayingRole,
    TitleRole,
    UrlRole,
};

static bool isLocal(const QUrl &url)
{
    return url.scheme() == "file" || url.scheme() == "qrc";
}

class Item {
public:
    Item();
    ~Item();
    int row() const;
    bool updateMetaData(QSoundFile *file);

    QString album;
    QString artist;
    qint64 duration;
    QUrl imageUrl;
    QString title;
    QUrl url;

    Item *parent;
    QList<Item*> children;
};

Item::Item()
    : duration(0),
    parent(0)
{
}

Item::~Item()
{
    foreach (Item *item, children)
        delete item;
}

// Update the metadata for the current item.
bool Item::updateMetaData(QSoundFile *file)
{
    if (!file || !file->open(QIODevice::ReadOnly))
        return false;

    duration = file->duration();

    QStringList values;
    values = file->metaData(QSoundFile::AlbumMetaData);
    if (!values.isEmpty())
        album = values.first();

    values = file->metaData(QSoundFile::ArtistMetaData);
    if (!values.isEmpty())
        artist = values.first();

    values = file->metaData(QSoundFile::TitleMetaData);
    if (!values.isEmpty())
        title = values.first();

    return true;
}

int Item::row() const
{
    if (!parent)
        return -1;
    return parent->children.indexOf((Item*)this);
}

class PlayerModelPrivate
{
public:
    PlayerModelPrivate(PlayerModel *qq);
    QModelIndex createIndex(Item *item, int column = 0) const;
    QList<Item*> find(Item *parent, const QUrl &url);
    void processXml(Item *item, QIODevice *reply);
    void processQueue();
    void save();
    QIODevice *dataFile(Item *item);
    QSoundFile::FileType dataType(Item *item);
    QSoundFile *soundFile(Item *item);

    QMap<QUrl, QUrl> dataCache;
    QNetworkReply *dataReply;
    Item *cursorItem;
    QNetworkAccessManager *network;
    QSoundPlayer *player;
    int playId;
    bool playStop;
    PlayerModel *q;
    Item *rootItem;
};

PlayerModelPrivate::PlayerModelPrivate(PlayerModel *qq)
    : dataReply(0),
    cursorItem(0),
    playId(-1),
    playStop(false),
    q(qq)
{
}

QModelIndex PlayerModelPrivate::createIndex(Item *item, int column) const
{
    if (item && item != rootItem)
        return q->createIndex(item->row(), column, item);
    else
        return QModelIndex();
}

QList<Item*> PlayerModelPrivate::find(Item *parent, const QUrl &url)
{
    QList<Item*> items;
    foreach (Item *item, parent->children) {
        if (url.isEmpty() || item->url == url)
            items << item;
        if (!item->children.isEmpty())
            items << find(item, url);
    }
    return items;
}

void PlayerModelPrivate::processQueue()
{
    if (dataReply)
        return;

    QList<Item*> items = find(rootItem, QUrl());
    foreach (Item *item, items) {
#ifndef USE_DECLARATIVE
        // check image
        if (item->imageUrl.isValid() &&
            !isLocal(item->imageUrl) &&
            !dataCache.contains(item->imageUrl))
        {
            //qDebug("Requesting image %s", qPrintable(item->imageUrl.toString()));
            dataReply = network->get(QNetworkRequest(item->imageUrl));
            dataReply->setProperty("_request_url", item->imageUrl);
            dataReply->setProperty("_request_type", "image");
            q->connect(dataReply, SIGNAL(finished()), q, SLOT(dataReceived()));
            return;
        }
#endif

        // check data
        if (item->url.isValid() &&
            !isLocal(item->url) &&
            !dataCache.contains(item->url))
        {
            //qDebug("Requesting data %s", qPrintable(url.toString()));
            dataReply = network->get(QNetworkRequest(item->url));
            dataReply->setProperty("_request_url", item->url);
            dataReply->setProperty("_request_type", "data");
            q->connect(dataReply, SIGNAL(finished()), q, SLOT(dataReceived()));
            emit q->dataChanged(createIndex(item, 0), createIndex(item, MaxColumn));
            return;
        }
    }
}

void PlayerModelPrivate::processXml(Item *channel, QIODevice *reply)
{
    // parse document
    QDomDocument doc;
    if (!doc.setContent(reply)) {
        qWarning("Received invalid XML");
        return;
    }

    QDomElement channelElement = doc.documentElement().firstChildElement("channel");

    // parse channel
    channel->artist = channelElement.firstChildElement("itunes:author").text();
    channel->title = channelElement.firstChildElement("title").text();
    QDomElement imageUrlElement = channelElement.firstChildElement("image").firstChildElement("url");
    if (!imageUrlElement.isNull())
        channel->imageUrl = QUrl(imageUrlElement.text());
    emit q->dataChanged(createIndex(channel, 0), createIndex(channel, MaxColumn));

    // parse items
    QDomElement itemElement = channelElement.firstChildElement("item");
    while (!itemElement.isNull()) {
        const QString title = itemElement.firstChildElement("title").text();
        QUrl audioUrl;
        QDomElement enclosureElement = itemElement.firstChildElement("enclosure");
        while (!enclosureElement.isNull() && !audioUrl.isValid()) {
            if (QSoundFile::typeFromMimeType(enclosureElement.attribute("type").toAscii()) != QSoundFile::UnknownFile)
                audioUrl = QUrl(enclosureElement.attribute("url"));
            enclosureElement = enclosureElement.nextSiblingElement("enclosure");
        }

        Item *item = new Item;
        item->url = audioUrl;
        item->artist = channel->artist;
        item->imageUrl = channel->imageUrl;
        item->title = title;
        item->parent = channel;
        q->beginInsertRows(createIndex(channel, 0), channel->children.size(), channel->children.size());
        channel->children.append(item);
        q->endInsertRows();
        itemElement = itemElement.nextSiblingElement("item");
    }
}

QIODevice *PlayerModelPrivate::dataFile(Item *item)
{
    if (isLocal(item->url))
        return new QFile(item->url.toLocalFile());
    else if (dataCache.contains(item->url)) {
        const QUrl dataUrl = dataCache.value(item->url);
        return wApp->networkCache()->data(dataUrl);
    }
    return 0;
}

QSoundFile::FileType PlayerModelPrivate::dataType(Item *item)
{
    if (isLocal(item->url)) {
        return QSoundFile::typeFromFileName(item->url.toLocalFile());
    } else if (dataCache.contains(item->url)) {
        const QUrl audioUrl = dataCache.value(item->url);
        QNetworkCacheMetaData::RawHeaderList headers = wApp->networkCache()->metaData(audioUrl).rawHeaders();
        foreach (const QNetworkCacheMetaData::RawHeader &header, headers)
        {
            if (header.first.toLower() == "content-type")
                return QSoundFile::typeFromMimeType(header.second);
        }
    }
    return QSoundFile::UnknownFile;
}

QSoundFile *PlayerModelPrivate::soundFile(Item *item)
{
    QSoundFile::FileType type = dataType(item);
    if (type != QSoundFile::UnknownFile) {
        QIODevice *device = dataFile(item);
        if (device)
            return new QSoundFile(device, type);
    }
    return 0;
}

void PlayerModelPrivate::save()
{
    QSettings settings;
    QStringList values;
    foreach (Item *item, rootItem->children)
        values << item->url.toString();
    settings.setValue("PlayerUrls", values);
}

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    bool check;
    d = new PlayerModelPrivate(this);
    d->rootItem = new Item;

    d->network = new QNetworkAccessManager(this);
    d->network->setCache(wApp->networkCache());

    // init player
    d->player = wApp->soundPlayer();
    check = connect(d->player, SIGNAL(finished(int)),
                    this, SLOT(finished(int)));
    Q_ASSERT(check);

    // set role names
    QHash<int, QByteArray> roleNames;
    roleNames.insert(AlbumRole, "album");
    roleNames.insert(ArtistRole, "artist");
    roleNames.insert(DownloadingRole, "downloading");
    roleNames.insert(DurationRole, "duration");
    roleNames.insert(ImageUrlRole, "imageUrl");
    roleNames.insert(PlayingRole, "playing");
    roleNames.insert(TitleRole, "title");
    roleNames.insert(UrlRole, "url");
    setRoleNames(roleNames);

    // load saved playlist
    QSettings settings;
    QStringList values = settings.value("PlayerUrls").toStringList();
    foreach (const QString &value, values)
        addUrl(QUrl(value));
}

PlayerModel::~PlayerModel()
{
    delete d->rootItem;
    delete d;
}

bool PlayerModel::addUrl(const QUrl &url)
{
    if (!url.isValid())
        return false;

    Item *item = new Item;
    item->parent = d->rootItem;
    item->title = QFileInfo(url.path()).baseName();
    item->imageUrl = QUrl("qrc:/file.png");
    item->url = url;

    // fetch meta data
    QSoundFile *file = d->soundFile(item);
    if (file) {
        item->updateMetaData(file);
        delete file;
    }

    beginInsertRows(d->createIndex(item->parent), item->parent->children.size(), item->parent->children.size());
    d->rootItem->children.append(item);
    endInsertRows();

    d->processQueue();

    // save playlist
    d->save();
    return true;
}

void PlayerModel::dataReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    const QUrl dataUrl = reply->property("_request_url").toUrl();
    const QString dataType = reply->property("_request_type").toString();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Request for data failed");
        d->dataReply = 0;
        return;
    }

    // follow redirect
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        redirectUrl = reply->url().resolved(redirectUrl);

        //qDebug("Following redirect to %s", qPrintable(redirectUrl.toString()));
        d->dataReply = d->network->get(QNetworkRequest(redirectUrl));
        d->dataReply->setProperty("_request_url", dataUrl);
        d->dataReply->setProperty("_request_type", dataType);
        connect(d->dataReply, SIGNAL(finished()), this, SLOT(dataReceived()));
        return;
    }

    d->dataCache[dataUrl] = reply->url();
    d->dataReply = 0;

    // process data
    const QString mimeType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if (dataType == "image") { 
        const QByteArray data = reply->readAll();
        QPixmap pixmap;
        if (!pixmap.loadFromData(data, 0)) {
            qWarning("Received invalid image");
            return;
        }
        QPixmapCache::insert(dataUrl.toString(), pixmap);
    }

    QList<Item*> items = d->find(d->rootItem, QUrl());
    foreach (Item *item, items) {
        if (dataUrl == item->imageUrl) {
            emit dataChanged(d->createIndex(item, 0), d->createIndex(item, MaxColumn));
        } else if (dataUrl == item->url) {
            if (mimeType == "application/xml") {
                QIODevice *device = d->dataFile(item);
                d->processXml(item, device);
            } else {
                QSoundFile *file = d->soundFile(item);
                if (file) {
                    if (item->updateMetaData(file))
                        emit dataChanged(d->createIndex(item, 0), d->createIndex(item, MaxColumn));
                    delete file;
                }
            }
        }
    }
    d->processQueue();
}

int PlayerModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QModelIndex PlayerModel::cursor() const
{
    return d->createIndex(d->cursorItem);
}

void PlayerModel::setCursor(const QModelIndex &index)
{
    Item *item = static_cast<Item*>(index.internalPointer());

    if (item != d->cursorItem) {
        Item *oldItem = d->cursorItem;
        d->cursorItem = item;
        if (oldItem)
            emit dataChanged(d->createIndex(oldItem, 0), d->createIndex(oldItem, MaxColumn));
        if (item)
            emit dataChanged(d->createIndex(item, 0), d->createIndex(item, MaxColumn));
        emit cursorChanged(index);
    }
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == AlbumRole)
        return item->album;
    else if (role == ArtistRole)
        return item->artist;
    else if (role == DownloadingRole)
        return d->dataReply && d->dataReply->property("_request_url").toUrl() == item->url;
    else if (role == DurationRole)
        return item->duration;
    else if (role == ImageUrlRole)
        return item->imageUrl;
    else if (role == PlayingRole)
        return (item == d->cursorItem);
    else if (role == TitleRole)
        return item->title;
    else if (role == UrlRole)
        return item->url;

#ifndef USE_DECLARATIVE
    if (index.column() == ArtistColumn) {
        if (role == Qt::DisplayRole)
            return item->artist;
        else if (role == Qt::DecorationRole) {
            QPixmap pixmap;
            if (item == d->cursorItem)
                return QPixmap(":/start.png");
            else if (d->dataReply && d->dataReply->property("_request_url").toUrl() == item->url)
                return QPixmap(":/download.png");
            else if (item->imageUrl.scheme() == "file") {
                const QString path = item->imageUrl.toLocalFile();
                return QPixmap(path);
            }
            else if (item->imageUrl.scheme() == "qrc") {
                const QString path = ":" + item->imageUrl.path();
                return QPixmap(path);
            }
            else if (QPixmapCache::find(item->imageUrl.toString(), &pixmap))
                return QIcon(pixmap);
        }
    } else if (index.column() == TitleColumn) {
        if (role == Qt::DisplayRole)
            return item->title;
    } else if (index.column() == DurationColumn) {
        if (role == Qt::DisplayRole) {
            return QTime().addMSecs(item->duration).toString("m:ss");
        }
    }
#endif
    return QVariant();
}

QVariant PlayerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == ArtistColumn)
            return tr("Artist");
        else if (section == TitleColumn)
            return tr("Title");
        else if (section == DurationColumn)
            return tr("Duration");
    }
    return QVariant();
}

QModelIndex PlayerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : d->rootItem;
    return d->createIndex(parentItem->children[row], column);
}

QModelIndex PlayerModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Item *item = static_cast<Item*>(index.internalPointer());
    return d->createIndex(item->parent);
}

void PlayerModel::finished(int id)
{
    if (id != d->playId)
        return;
    d->playId = -1;
    if (d->playStop) {
        setCursor(QModelIndex());
        d->playStop = false;
    } else {
        const QModelIndex index = cursor();
        QModelIndex nextIndex = index.sibling(index.row() + 1, index.column());
        play(nextIndex);
    }
}

void PlayerModel::play(const QModelIndex &index)
{
    // stop previous audio
    if (d->playId >= 0) {
        d->player->stop(d->playId);
        d->playId = -1;
    }

    // start new audio output
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return;

    QSoundFile *file = d->soundFile(item);
    if (file) {
        d->playId = d->player->play(file);
        setCursor(index);
    } else {
        setCursor(QModelIndex());
    }
}

bool PlayerModel::removeRow(int row)
{
    return QAbstractItemModel::removeRow(row);
}

bool PlayerModel::removeRows(int row, int count, const QModelIndex &parent)
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : d->rootItem;

    const int minIndex = qMax(0, row);
    const int maxIndex = qMin(row + count, parentItem->children.size()) - 1;
    beginRemoveRows(parent, minIndex, maxIndex);
    for (int i = maxIndex; i >= minIndex; --i)
        parentItem->children.removeAt(i);
    endRemoveRows();

    // save playlist
    d->save();
    return true;
}

QModelIndex PlayerModel::row(int row)
{
    return index(row, 0, QModelIndex());
}

int PlayerModel::rowCount(const QModelIndex &parent) const
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : d->rootItem;
    return parentItem->children.size();
}

void PlayerModel::stop()
{
    if (d->playId >= 0) {
        d->player->stop(d->playId);
        d->playStop = true;
    }
}

PlayerPanel::PlayerPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow)
{
    bool check;

    setObjectName(PLAYER_ROSTER_ID);
    setWindowIcon(QIcon(":/start.png"));
    setWindowTitle(tr("Media player"));

    m_model = new PlayerModel(this);
    check = connect(m_model, SIGNAL(cursorChanged(QModelIndex)),
                    this, SLOT(cursorChanged(QModelIndex)));
    Q_ASSERT(check);

    m_player = wApp->soundPlayer();

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(5);
    setLayout(layout);

    // controls
    QHBoxLayout *controls = new QHBoxLayout;
    controls->addStretch();

    m_stopButton = new QPushButton(QIcon(":/stop.png"), tr("Stop"));
    m_stopButton->hide();
    check = connect(m_stopButton, SIGNAL(clicked()),
                    m_model, SLOT(stop()));
    Q_ASSERT(check);
    controls->addWidget(m_stopButton);

    controls->addStretch();
    layout->addLayout(controls);
    layout->addSpacing(10);

    // playlist
#ifdef USE_DECLARATIVE
    QDeclarativeView *view = new QDeclarativeView;
    QDeclarativeContext *ctxt = view->rootContext();
    ctxt->setContextProperty("playerModel", m_model);
    //view->setSource(QUrl::fromLocalFile("src/data/player.qml"));
    view->setSource(QUrl("qrc:/player.qml"));
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    layout->addWidget(view, 1);
    setFocusProxy(view);
    filterDrops(view);
#else
    m_view = new PlayerView;
    m_view->setModel(m_model);
    m_view->setIconSize(QSize(32, 32));
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    check = connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
                    m_model, SLOT(play(QModelIndex)));
    Q_ASSERT(check);
    layout->addWidget(m_view, 1);
    setFocusProxy(m_view);
    filterDrops(m_view);

    // select first track
    if (m_model->rowCount(QModelIndex()))
        m_view->selectionModel()->select(m_model->index(0, 0, QModelIndex()), QItemSelectionModel::Rows | QItemSelectionModel::Select);
#endif

    // register panel
    check = connect(m_chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
                    this, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    Q_ASSERT(check);

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_M), m_chat);
    check = connect(shortcut, SIGNAL(activated()),
                    this, SIGNAL(showPanel()));
    Q_ASSERT(check);
}

void PlayerPanel::cursorChanged(const QModelIndex &index)
{
    if (index.isValid()) {
        m_stopButton->setEnabled(true);
        m_stopButton->show();
    } else {
        m_stopButton->hide();
    }
}

void PlayerPanel::play()
{
#ifdef USE_DECLARATIVE
    m_model->play(m_model->index(0, 0, QModelIndex()));
#else
    QList<QModelIndex> selected = m_view->selectionModel()->selectedIndexes();
    if (!selected.isEmpty())
        m_model->play(selected.first());
    else if (m_model->rowCount(QModelIndex()))
        m_model->play(m_model->index(0, 0, QModelIndex()));
#endif
}

static QList<QUrl> getUrls(const QUrl &url) {
    if (!isLocal(url))
        return QList<QUrl>() << url;

    QList<QUrl> urls;
    const QString path = url.toLocalFile();
    if (QFileInfo(path).isDir()) {
        QDir dir(path);
        QStringList children = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        foreach (const QString &child, children)
            urls << getUrls(QUrl::fromLocalFile(dir.filePath(child)));
    } else if (QSoundFile::typeFromFileName(path) != QSoundFile::UnknownFile) {
        urls << url;
    }
    return urls;
}

/** Handle a drop event on a roster entry.
 */
void PlayerPanel::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    if (index.data(ChatRosterModel::IdRole).toString() != objectName())
        return;

    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        // check protocol is supported
        if (!isLocal(url) &&
            url.scheme() != "http" &&
            url.scheme() != "https")
            continue;

        if (event->type() == QEvent::Drop) {
            foreach (const QUrl &child, getUrls(url))
                m_model->addUrl(child);
        }
        found++;
    }
    if (found)
        event->acceptProposedAction();
}

PlayerView::PlayerView(QWidget *parent)
    : QTreeView(parent)
{
    header()->setResizeMode(QHeaderView::Fixed);
}

void PlayerView::setModel(PlayerModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(DurationColumn, DURATION_WIDTH);
}

void PlayerView::keyPressEvent(QKeyEvent *event)
{
    const QModelIndex &index = currentIndex();
    if (index.isValid()) {
        switch (event->key())
        {
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            model()->removeRow(index.row());
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            emit doubleClicked(index);
            break;
        }
    }

    QTreeView::keyPressEvent(event);
}

void PlayerView::resizeEvent(QResizeEvent *e)
{
    QTreeView::resizeEvent(e);

    const int available = e->size().width() - 16 - DURATION_WIDTH;
    setColumnWidth(ArtistColumn, available/2);
    setColumnWidth(TitleColumn, available/2);
    setColumnWidth(DurationColumn, DURATION_WIDTH);
}

// PLUGIN

class PlayerPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool PlayerPlugin::initialize(Chat *chat)
{
    PlayerPanel *panel = new PlayerPanel(chat);
    chat->addPanel(panel);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(player, PlayerPlugin)

