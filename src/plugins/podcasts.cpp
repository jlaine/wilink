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
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "podcasts.h"

PodcastsPanel::PodcastsPanel(Chat *chatWindow)
    : ChatPanel(chatWindow),
    m_chat(chatWindow)
{
    Application *wApp = qobject_cast<Application*>(qApp);
    m_network = new QNetworkAccessManager(this);
    m_network->setCache(wApp->networkCache());

    setObjectName("z_2_podcasts");
    setWindowIcon(QIcon(":/photos.png"));
    setWindowTitle(tr("My podcasts"));

    // build layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->addLayout(headerLayout());
    layout->addSpacing(10);

    m_listWidget = new QListWidget;
    layout->addWidget(m_listWidget);
    setLayout(layout);

    /* register panel */
    QTimer::singleShot(0, this, SIGNAL(registerPanel()));
    
    // load XML 
    QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl("http://radiofrance-podcast.net/podcast09/rss_11529.xml")));
    connect(reply, SIGNAL(finished()), this, SLOT(xmlReceived()));
}

void PodcastsPanel::imageReceived()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || reply->error() != QNetworkReply::NoError) {
        qWarning("Request for image failed");
        return;
    }

    const QByteArray data = reply->readAll();
    QPixmap pixmap;
    if (!pixmap.loadFromData(data, 0)) {
        qWarning("Received invalid image");
        return;
    }

    setWindowIcon(pixmap);
}

void PodcastsPanel::xmlReceived()
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
    setWindowExtra(channelElement.firstChildElement("title").text());
    QDomElement imageUrlElement = channelElement.firstChildElement("image").firstChildElement("url");
    if (!imageUrlElement.isNull()) {
        QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl(imageUrlElement.text())));
        connect(reply, SIGNAL(finished()), this, SLOT(imageReceived()));
    }

    // parse items
    m_listWidget->clear();
    QDomElement itemElement = channelElement.firstChildElement("item");
    while (!itemElement.isNull()) {
        const QString title = itemElement.firstChildElement("title").text();
        m_listWidget->addItem(new QListWidgetItem(title));
        itemElement = itemElement.nextSiblingElement("item");
    }
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

