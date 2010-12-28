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

#include "chat.h"
#include "chat_plugin.h"
#include "player.h"

PlayerModel::PlayerModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_rootItem = new Item;
}

PlayerModel::~PlayerModel()
{
    delete m_rootItem;
}

void PlayerModel::addFile(const QUrl &url)
{
    Item *item = new Item;
    item->url = url;
    beginInsertRows(QModelIndex(), m_rootItem->children.size(), m_rootItem->children.size());
    m_rootItem->children.append(item);
    endInsertRows();
}

QVariant PlayerModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (role == Qt::DisplayRole)
        return item->title;
    else if (role == Qt::DecorationRole) {
        return QIcon(":/play.png");
    }
    return QVariant();
}

int PlayerModel::rowCount(const QModelIndex &parent) const
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return parentItem->children.size();
}

PlayerPanel::PlayerPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow),
    m_playId(-1)
{
    setObjectName("z_2_player");
    setWindowIcon(QIcon(":/start.png"));
    setWindowTitle(tr("Media player"));

    m_model = new PlayerModel(this);

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(10);

    m_view = new PlayerView;
    m_view->setModel(m_model);
    m_view->setIconSize(QSize(32, 32));
    layout->addWidget(m_view);

    setLayout(layout);

    // register panel
    QMetaObject::invokeMethod(this, "registerPanel", Qt::QueuedConnection);
}

void PlayerPanel::doubleClicked(const QModelIndex &index)
{

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

