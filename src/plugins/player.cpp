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

#define DURATION_WIDTH 60
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
    DurationRole,
    PlayingRole,
    TitleRole,
    UrlRole,
};

class Item {
public:
    Item();
    ~Item();
    bool isLocal() const;
    int row() const;
    bool updateMetaData(QSoundFile *file);

    QString album;
    QString artist;
    qint64 duration;
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

bool Item::isLocal() const
{
    return url.scheme() == "file" || url.scheme() == "qrc";
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
    void processQueue();
    void save();
    QSoundFile *soundFile(Item *item);

    QMap<QUrl, QUrl> audioCache;
    QNetworkReply *audioReply;
    Item *cursorItem;
    QNetworkAccessManager *network;
    QSoundPlayer *player;
    int playId;
    bool playStop;
    PlayerModel *q;
    Item *rootItem;
};

PlayerModelPrivate::PlayerModelPrivate(PlayerModel *qq)
    : audioReply(0),
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

void PlayerModelPrivate::processQueue()
{
    foreach (Item *item, rootItem->children) {
        const QUrl url = item->url;
        if (!url.isValid() || item->isLocal())
            continue;

        if (!audioCache.contains(url)) {
            qDebug("Requesting audio %s", qPrintable(url.toString()));
            audioReply = network->get(QNetworkRequest(url));
            audioReply->setProperty("_request_url", url);
            q->connect(audioReply, SIGNAL(finished()), q, SLOT(audioReceived()));
            return;
        }
        break;
    }
}

QSoundFile *PlayerModelPrivate::soundFile(Item *item)
{
    QSoundFile *file = 0;
    if (item->isLocal()) {
        file = new QSoundFile(item->url.toLocalFile());
    } else if (audioCache.contains(item->url)) {
        const QUrl audioUrl = audioCache.value(item->url);
        QIODevice *device = wApp->networkCache()->data(audioUrl);
        if (device)
            file = new QSoundFile(device, QSoundFile::Mp3File);
    }
    return file;
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
    roleNames.insert(DurationRole, "duration");
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

void PlayerModel::audioReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    const QUrl audioUrl = reply->property("_request_url").toUrl();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Request for audio failed");
        return;
    }

    // follow redirect
    QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectUrl.isValid()) {
        redirectUrl = reply->url().resolved(redirectUrl);

        qDebug("Following redirect to %s", qPrintable(redirectUrl.toString()));
        d->audioReply = d->network->get(QNetworkRequest(redirectUrl));
        d->audioReply->setProperty("_request_url", audioUrl);
        connect(d->audioReply, SIGNAL(finished()), this, SLOT(audioReceived()));
        return;
    }

    qDebug("Received audio %s", qPrintable(audioUrl.toString()));
    d->audioCache[audioUrl] = reply->url();
    d->audioReply = 0;
    foreach (Item *item, d->rootItem->children) {
        if (item->url == audioUrl) {
            QSoundFile *file = d->soundFile(item);
            if (file) {
                if (item->updateMetaData(file))
                    emit dataChanged(d->createIndex(item, 0), d->createIndex(item, MaxColumn));
                delete file;
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
    else if (role == DurationRole)
        return item->duration;
    else if (role == PlayingRole)
        return (item == d->cursorItem);
    else if (role == TitleRole)
        return item->title;
    else if (role == UrlRole)
        return item->url;

    if (index.column() == ArtistColumn) {
        if (role == Qt::DisplayRole)
            return item->artist;
        else if (role == Qt::DecorationRole && item == d->cursorItem)
            return QPixmap(":/start.png");
    } else if (index.column() == TitleColumn) {
        if (role == Qt::DisplayRole)
            return item->title;
    } else if (index.column() == DurationColumn) {
        if (role == Qt::DisplayRole) {
            return QTime().addMSecs(item->duration).toString("m:ss");
        }
    }
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

    m_playButton = new QPushButton(QIcon(":/start.png"), tr("Play"));
    check = connect(m_playButton, SIGNAL(clicked()),
                    this, SLOT(play()));
    Q_ASSERT(check);
    controls->addWidget(m_playButton);

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
    filterDrops(m_view);
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
        m_playButton->hide();
        m_stopButton->setEnabled(true);
        m_stopButton->show();
    } else {
        m_stopButton->hide();
        m_playButton->show();
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
    if (url.scheme() != "file" && url.scheme() != "qrc")
        return QList<QUrl>() << url;

    QList<QUrl> urls;
    const QString path = url.toLocalFile();
    if (QFileInfo(path).isDir()) {
        QDir dir(path);
        QStringList children = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        foreach (const QString &child, children)
            urls << getUrls(QUrl::fromLocalFile(dir.filePath(child)));
    } else {
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
        qDebug() << "add" << url.toString();
        // check protocol is supported
        if (url.scheme() != "file" &&
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
    setColumnWidth(DurationColumn, 40);
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

    const int available = e->size().width() - DURATION_WIDTH;
    setColumnWidth(ArtistColumn, available/2);
    setColumnWidth(TitleColumn, available/2);
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

