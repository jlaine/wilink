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

#include <QCoreApplication>
#include <QDebug>
#include <QDomDocument>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSystemTrayIcon>
#include <QTimer>

#include "qnetio/wallet.h"

#include "chat.h"
#include "chat_client.h"
#include "chat_plugin.h"
#include "menu.h"

static const QUrl baseUrl("https://www.wifirst.net/w/");
static const QString authSuffix = "@wifirst.net";
static int retryInterval = 15000;

Menu::Menu(QObject *parent)
    : QObject(parent)
{
    userAgent = QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii();

    /* prepare network manager */
    network = new QNetworkAccessManager(this);
    connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
            QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));

    QTimer::singleShot(1000, this, SLOT(fetchMenu()));
}

void Menu::fetchIcon()
{
    if (icons.empty())
        return;

    QPair<QUrl, QAction*> entry = icons.first();
    QNetworkRequest req(entry.first);
    req.setRawHeader("User-Agent", userAgent);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showIcon()));
}

void Menu::fetchMenu()
{
    QNetworkRequest req(baseUrl);
    req.setRawHeader("Accept", "application/xml");
    req.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    req.setRawHeader("User-Agent", userAgent);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showMenu()));
}

void Menu::showMenu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    Q_ASSERT(reply != NULL);

    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning("Failed to retrieve menu");
        QTimer::singleShot(retryInterval, this, SLOT(fetchMenu()));
        return;
    }

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

    /* parse messages */
    item = doc.documentElement().firstChildElement("messages").firstChildElement("message");
    while (!item.isNull())
    {
        const QString id = item.firstChildElement("id").text();
        const QString title = item.firstChildElement("title").text();
        const QString text = item.firstChildElement("text").text();
        const int type = item.firstChildElement("type").text().toInt();
        enum QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information;
        switch (type)
        {
            case 3: icon = QSystemTrayIcon::Critical; break;
            case 2: icon = QSystemTrayIcon::Warning; break;
            case 1: icon = QSystemTrayIcon::Information; break;
            case 0: icon = QSystemTrayIcon::NoIcon; break;
        }
        if (!seenMessages.contains(id))
        {
            //showMessage(title, text, icon);
            seenMessages.append(id);
        }
        item = item.nextSiblingElement("message");
    }

    /* parse preferences */
#if 0
    item = doc.documentElement().firstChildElement("preferences");
    QString urlString = item.firstChildElement("updates").text();
    if (!urlString.isEmpty())
        updates->setUrl(baseUrl.resolved(QUrl(urlString)));

    refreshInterval = item.firstChildElement("refresh").text().toInt() * 1000;
    if (refreshInterval > 0)
        QTimer::singleShot(refreshInterval, this, SLOT(fetchMenu()));

    urlString = item.firstChildElement("diagnostics").text();
    if (!urlString.isEmpty())
        diagnostics->setUrl(baseUrl.resolved(QUrl(urlString)));
#endif

    /* fetch icons */
    fetchIcon();
}

// PLUGIN

class MenuPlugin : public ChatPlugin
{
public:
    bool initialize(Chat *chat);
};

bool MenuPlugin::initialize(Chat *chat)
{
    QString url;
    QString domain = chat->chatClient()->getConfiguration().domain();
    if (domain != "wifirst.net")
        return false;

    Menu *menu = new Menu(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(menu, MenuPlugin)

