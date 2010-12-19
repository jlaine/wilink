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
#include <QTimer>
#include <QTreeView>
#include <QUrl>

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "podcasts.h"

PodcastsModel::PodcastsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    Application *wApp = qobject_cast<Application*>(qApp);
    m_network = new QNetworkAccessManager(this);
    m_network->setCache(wApp->networkCache());

    QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl("http://radiofrance-podcast.net/podcast09/rss_11529.xml")));
    connect(reply, SIGNAL(finished()), this, SLOT(xmlReceived()));
}

int PodcastsModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant PodcastsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    if (index.column() == 0) {
        if (role == Qt::DisplayRole)
            return m_items[index.row()].title;
        else if (role == Qt::DecorationRole)
            return QIcon(m_pixmap);
    } else if (index.column() == 0) {
        if (role == Qt::DisplayRole)
            return m_items[index.row()].audioUrl;
    }
    return QVariant();
}

QModelIndex PodcastsModel::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column);
}

QModelIndex PodcastsModel::parent(const QModelIndex & index) const
{
    return QModelIndex();
}

int PodcastsModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_items.size();
    else
        return 0;
}

void PodcastsModel::imageReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply->error() != QNetworkReply::NoError) {
        qWarning("Request for image failed");
        return;
    }

    const QByteArray data = reply->readAll();
    if (!m_pixmap.loadFromData(data, 0)) {
        qWarning("Received invalid image");
        return;
    }

    if (m_items.size())
        emit dataChanged(index(0, 0, QModelIndex()), index(m_items.size() - 1, 0, QModelIndex()));
#if 0
    QIcon icon(m_pixmap);
    for (int i = 0; i < m_listWidget->count(); ++i)
        m_listWidget->item(i)->setIcon(icon);
    setWindowIcon(icon);
#endif
}

void PodcastsModel::xmlReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply->error() != QNetworkReply::NoError) {
        qWarning("Request for XML failed");
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(reply)) {
        qWarning("Received invalid XML");
        return;
    }

    QDomElement channelElement = doc.documentElement().firstChildElement("channel");

    // parse channel
    //setWindowExtra(channelElement.firstChildElement("title").text());
    QDomElement imageUrlElement = channelElement.firstChildElement("image").firstChildElement("url");
    if (!imageUrlElement.isNull()) {
        QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl(imageUrlElement.text())));
        connect(reply, SIGNAL(finished()), this, SLOT(imageReceived()));
    }

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
        Item item;
        item.audioUrl = audioUrl;
        item.title = title;
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
        m_items.append(item);
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
    m_view->setRootIsDecorated(false);

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

