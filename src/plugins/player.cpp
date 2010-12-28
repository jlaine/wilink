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

#include <QLayout>
#include <QPushButton>
#include <QSettings>

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"

#include "qsound/QSoundFile.h"
#include "qsound/QSoundPlayer.h"
#include "player.h"

enum PlayerColumns {
    ArtistColumn = 0,
    TitleColumn,
    MaxColumn
};

PlayerModel::Item::Item()
    : parent(0)
{
}

PlayerModel::Item::~Item()
{
    foreach (Item *item, children)
        delete item;
}

int PlayerModel::Item::row() const
{
    if (!parent)
        return -1;
    return parent->children.indexOf((Item*)this);
}

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new Item;

    // load saved playlist
    QSettings settings;
    QStringList values = settings.value("PlayerUrls").toStringList();
    foreach (const QString &value, values)
        addUrl(QUrl(value));
}

PlayerModel::~PlayerModel()
{
    delete m_rootItem;
}

bool PlayerModel::addUrl(const QUrl &url)
{
    QSoundFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly))
        return false;

    Item *item = new Item;
    item->parent = m_rootItem;
    item->url = url;

    QStringList values = file.metaData(QSoundFile::TitleMetaData);
    if (!values.isEmpty())
        item->title = values.first();
    else
        item->title = url.toLocalFile();

    values = file.metaData(QSoundFile::ArtistMetaData);
    if (!values.isEmpty())
        item->artist = values.first();

    beginInsertRows(createIndex(item->parent), item->parent->children.size(), item->parent->children.size());
    m_rootItem->children.append(item);
    endInsertRows();

    return true;
}

int PlayerModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::UserRole)
        return item->url;

    if (index.column() == ArtistColumn) {
        if (role == Qt::DisplayRole)
            return item->artist;
        else if (role == Qt::DecorationRole)
            return QIcon(":/play.png");
    } else if (index.column() == TitleColumn) {
        if (role == Qt::DisplayRole)
            return item->title;
    }
    return QVariant();
}

QModelIndex PlayerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return createIndex(parentItem->children[row], column);
}

QModelIndex PlayerModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Item *item = static_cast<Item*>(index.internalPointer());
    return createIndex(item->parent);
}

int PlayerModel::rowCount(const QModelIndex &parent) const
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return parentItem->children.size();
}

void PlayerModel::save()
{
    QSettings settings;
    QStringList values;
    foreach (Item *item, m_rootItem->children)
        values << item->url.toString();
    settings.setValue("PlayerUrls", values);
}

PlayerPanel::PlayerPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow),
    m_playId(-1)
{
    bool check;

    setObjectName("z_2_player");
    setWindowIcon(QIcon(":/start.png"));
    setWindowTitle(tr("Media player"));

    m_model = new PlayerModel(this);

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(10);

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
                    this, SLOT(stop()));
    Q_ASSERT(check);
    controls->addWidget(m_stopButton);

    controls->addStretch();
    layout->addLayout(controls);

    // playlist
    m_view = new PlayerView;
    m_view->setModel(m_model);
    m_view->setIconSize(QSize(32, 32));
    check = connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
                    this, SLOT(doubleClicked(QModelIndex)));
    Q_ASSERT(check);
    layout->addWidget(m_view);

    setLayout(layout);

    // register panel
    filterDrops(m_view);

    check = connect(m_chat, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)),
                    this, SLOT(rosterDrop(QDropEvent*, QModelIndex)));
    Q_ASSERT(check);

    QMetaObject::invokeMethod(this, "registerPanel", Qt::QueuedConnection);
}

void PlayerPanel::doubleClicked(const QModelIndex &index)
{
    Application *wApp = qobject_cast<Application*>(qApp);
    if (m_playId >= 0)
        wApp->soundPlayer()->stop(m_playId);

    QUrl audioUrl = index.data(Qt::UserRole).toUrl();
    if (audioUrl.isValid() && audioUrl != m_playUrl) {
        m_playId = wApp->soundPlayer()->play(audioUrl.toLocalFile());
        m_playUrl = audioUrl;
        m_playButton->hide();
        m_stopButton->show();
        return;
    }

    m_playId = -1;
    m_playUrl = QUrl();
    m_stopButton->hide();
    m_playButton->show();
}

void PlayerPanel::play()
{
    QModelIndex index = m_model->index(0, 0, QModelIndex());
    doubleClicked(index);
}

/** Handle a drop event on a roster entry.
 */
void PlayerPanel::rosterDrop(QDropEvent *event, const QModelIndex &index)
{
    if (index.data(ChatRosterModel::IdRole).toString() != objectName())
        return;

    int added = 0;
    int found = 0;
    foreach (const QUrl &url, event->mimeData()->urls())
    {
        if (url.scheme() != "file")
            continue;
        if (event->type() == QEvent::Drop)
            if (m_model->addUrl(url))
                added++;
        found++;
    }
    if (found)
        event->acceptProposedAction();
    if (added)
        m_model->save();
}

void PlayerPanel::stop()
{
    Application *wApp = qobject_cast<Application*>(qApp);
    if (m_playId >= 0)
        wApp->soundPlayer()->stop(m_playId);
    m_playId = -1;
    m_playUrl = QUrl();
    m_stopButton->hide();
    m_playButton->show();
}

PlayerView::PlayerView(QWidget *parent)
    : QTreeView(parent)
{
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

