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

#include <QDomDocument>
#include <QLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmapCache>
#include <QTimer>
#include <QTreeView>
#include <QUrl>

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "podcasts.h"

enum PodcastsColumns {
    MainColumn = 0,
    ImageColumn,
    MaxColumn
};

PodcastsModel::Item::Item()
    : parent(0)
{
}

PodcastsModel::Item::~Item()
{
    foreach (PodcastsModel::Item *item, children)
        delete item;
}

int PodcastsModel::Item::row() const
{
    if (!parent)
        return -1;
    return parent->children.indexOf((PodcastsModel::Item*)this);
}

PodcastsModel::PodcastsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    Application *wApp = qobject_cast<Application*>(qApp);
    m_network = new QNetworkAccessManager(this);
    m_network->setCache(wApp->networkCache());

    m_rootItem = new Item;

    addChannel(QUrl("http://radiofrance-podcast.net/podcast09/rss_11529.xml"));
}

PodcastsModel::~PodcastsModel()
{
    delete m_rootItem;
}

void PodcastsModel::addChannel(const QUrl &url)
{
    Item *channel = new Item;
    channel->parent = m_rootItem;
    channel->url = url;
    beginInsertRows(QModelIndex(), m_rootItem->children.size(), m_rootItem->children.size());
    m_rootItem->children << channel;
    endInsertRows();

    QNetworkReply *reply = m_network->get(QNetworkRequest(channel->url));
    connect(reply, SIGNAL(finished()), this, SLOT(xmlReceived()));
}

int PodcastsModel::columnCount(const QModelIndex &parent) const
{
    return MaxColumn;
}

QVariant PodcastsModel::data(const QModelIndex &index, int role) const
{
    Item *item = static_cast<Item*>(index.internalPointer());
    if (!index.isValid() || !item)
        return QVariant();

    if (index.column() == MainColumn) {
        if (role == Qt::DisplayRole)
            return item->title;
        else if (role == Qt::DecorationRole) {
            QPixmap pixmap;
            if (QPixmapCache::find(item->imageUrl.toString(), &pixmap))
                return QIcon(pixmap);
        }
    } else if (index.column() == ImageColumn) {
        if (role == Qt::DisplayRole)
            return item->audioUrl.toString();
    }
    return QVariant();
}

QModelIndex PodcastsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return createIndex(parentItem->children[row], column);
}

QModelIndex PodcastsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Item *item = static_cast<Item*>(index.internalPointer());
    return createIndex(item->parent, MainColumn);
}

int PodcastsModel::rowCount(const QModelIndex &parent) const
{
    Item *parentItem = parent.isValid() ? static_cast<Item*>(parent.internalPointer()) : m_rootItem;
    return parentItem->children.size();
}

void PodcastsModel::imageReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Request for image failed");
        return;
    }

    // store result
    const QUrl imageUrl = reply->url();
    const QByteArray data = reply->readAll();
    QPixmap pixmap;
    if (!pixmap.loadFromData(data, 0)) {
        qWarning("Received invalid image");
        return;
    }
    QPixmapCache::insert(imageUrl.toString(), pixmap);

    // update affected items
    foreach (Item *item, m_rootItem->children) {
        if (item->imageUrl == imageUrl) {
            emit dataChanged(createIndex(item, MainColumn), createIndex(item, MainColumn));
        }
    }
}

void PodcastsModel::xmlReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning("Request for XML failed");
        return;
    }

    // find channel
    Item *channel = 0;
    foreach (Item *item, m_rootItem->children) {
        if (item->url == reply->url()) {
            channel = item;
            break;
        }
    }
    if (!channel)
        return;

    // parse document
    QDomDocument doc;
    if (!doc.setContent(reply)) {
        qWarning("Received invalid XML");
        return;
    }

    QDomElement channelElement = doc.documentElement().firstChildElement("channel");

    // parse channel
    channel->title = channelElement.firstChildElement("title").text();
    QDomElement imageUrlElement = channelElement.firstChildElement("image").firstChildElement("url");
    if (!imageUrlElement.isNull()) {
        channel->imageUrl = QUrl(imageUrlElement.text());
        QNetworkReply *reply = m_network->get(QNetworkRequest(channel->imageUrl));
        connect(reply, SIGNAL(finished()), this, SLOT(imageReceived()));
    }
    emit dataChanged(createIndex(channel, MainColumn), createIndex(channel, MainColumn));

    // parse items
    QDomElement itemElement = channelElement.firstChildElement("item");
    while (!itemElement.isNull()) {
        const QString title = itemElement.firstChildElement("title").text();
        QUrl audioUrl;
        QDomElement enclosureElement = itemElement.firstChildElement("enclosure");
        while (!enclosureElement.isNull() && !audioUrl.isValid()) {
            if (enclosureElement.attribute("type") == "audio/mpeg")
                audioUrl = QUrl(enclosureElement.attribute("url"));
            enclosureElement = enclosureElement.nextSiblingElement("enclosure");
        }

        if (audioUrl.isValid()) {
            qDebug("got audio %s", qPrintable(audioUrl.toString()));
        }

        Item *item = new Item;
        item->audioUrl = audioUrl;
        item->imageUrl = channel->imageUrl;
        item->title = title;
        item->parent = channel;
        beginInsertRows(createIndex(channel, MainColumn), channel->children.size(), channel->children.size());
        channel->children.append(item);
        endInsertRows();
        itemElement = itemElement.nextSiblingElement("item");
    }
}

PodcastsPanel::PodcastsPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow)
{
    setObjectName("z_2_podcasts");
    setWindowIcon(QIcon(":/photos.png"));
    setWindowTitle(tr("My podcasts"));

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(10);

    m_model = new PodcastsModel(this);
    m_view = new QTreeView;
    m_view->setModel(m_model);
    m_view->setIconSize(QSize(32, 32));
    //m_view->setRootIsDecorated(false);

    layout->addWidget(m_view);
    setLayout(layout);

    /* register panel */
    QTimer::singleShot(0, this, SIGNAL(registerPanel()));
}

// PLUGIN

class PodcastsPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool PodcastsPlugin::initialize(Chat *chat)
{
    PodcastsPanel *panel = new PodcastsPanel(chat);
    chat->addPanel(panel);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(podcasts, PodcastsPlugin)

