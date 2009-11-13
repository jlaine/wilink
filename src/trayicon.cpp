/*
 * wDesktop
 * Copyright (C) 2008-2009 Bolloré telecom
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
#include <QDebug>
#include <QDesktopServices>
#include <QDomDocument>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "qnetio/wallet.h"
#include "photos.h"
#include "trayicon.h"

static const QUrl baseUrl("https://www.wifirst.net/w/");

TrayIcon::TrayIcon()
    : photos(NULL)
{
    network = new QNetworkAccessManager(this);
    connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(network, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)), Wallet::instance(), SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> &)));

    /* icons */
    icons.append(qMakePair(baseUrl.resolved(QUrl("images/shared/url_icon.png")), (QAction*)NULL));
    fetchIcon();

    /* fetch menu */
    QNetworkRequest req(baseUrl);
    req.setRawHeader("Accept", "application/xml");
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showMenu()));
}

void TrayIcon::fetchIcon()
{
    if (icons.empty())
        return;

    QPair<QUrl, QAction*> entry = icons.first();
    QNetworkReply *reply = network->get(QNetworkRequest(entry.first));
    connect(reply, SIGNAL(finished()), this, SLOT(showIcon()));
}

void TrayIcon::openUrl()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        QDesktopServices::openUrl(action->data().toUrl());
}

void TrayIcon::showIcon()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    /* display current icon */
    QPair<QUrl, QAction*> entry = icons.takeFirst();
    QPixmap pixmap;
    QByteArray data = reply->readAll();
    if (!pixmap.loadFromData(data, 0))
    {
        qWarning() << "could not load icon" << entry.first;
        return;
    }
    if (entry.second) {
        entry.second->setIcon(QIcon(pixmap));
    } else {
        setIcon(QIcon(pixmap));
        show();
    }

    /* fetch next icon */
    fetchIcon();
}

void TrayIcon::showMenu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;

    QMenu *menu = new QMenu;
    QAction *action;

    /* parse menu */
    QDomDocument doc;
    doc.setContent(reply);
    QDomElement item = doc.documentElement().firstChildElement("menu").firstChildElement("menu");
    while (!item.isNull())
    {
        const QString image = item.firstChildElement("image").text();
        const QString link = item.firstChildElement("link").text();
        const QString text = item.firstChildElement("label").text();
        if (text.isEmpty())
            menu->addSeparator();
        else {
            action = menu->addAction(text);
            action->setData(baseUrl.resolved(link));
            connect(action, SIGNAL(triggered(bool)), this, SLOT(openUrl()));
            /* schedule retrieval of icon */
            if (!image.isEmpty())
                icons.append(QPair<QUrl, QAction*>(baseUrl.resolved(image), action));
        }
        item = item.nextSiblingElement("menu");
    }

    /* add static entries */
    menu->addSeparator();
    action = menu->addAction(tr("&Upload photos"));
    icons.append(QPair<QUrl, QAction*>(baseUrl.resolved(QString("images/wfox/album.png")), action));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(uploadPhotos()));

    action = menu->addAction(tr("&Quit"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    setContextMenu(menu);

    /* load icons */
    fetchIcon();
}

void TrayIcon::uploadPhotos()
{
    if (!photos)
    {
        QAction *action = qobject_cast<QAction *>(sender());
        photos = new Photos("wifirst://www.wifirst.net/w", action->icon());
    }
    photos->show();
    photos->raise();
}


