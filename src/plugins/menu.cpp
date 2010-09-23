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
#include <QDesktopServices>
#include <QDomDocument>
#include <QMenu>
#include <QMenuBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSortFilterProxyModel>
#include <QSystemTrayIcon>
#include <QTimer>

#include "QXmppClient.h"

#include "qnetio/wallet.h"

#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "menu.h"

static const QUrl baseUrl("https://www.wifirst.net/wilink/menu/");

static const QString authSuffix = "@wifirst.net";
static int retryInterval = 15000;

Menu::Menu(QMenuBar *bar, Chat *window)
    : QObject(bar),
    refreshInterval(0),
    chatWindow(window),
    menuBar(bar),
    servicesMenu(0)
{
    bool check;

    userAgent = QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii();

    servicesMenu = new QMenu(tr("&Services"));
    menuBar->addMenu(servicesMenu);

    /* add roster entry */
    QModelIndex index = chatWindow->rosterModel()->addItem(ChatRosterItem::Other,
        "home",
        tr("My residence"),
        QIcon(":/favorite-active.png"));
    QSortFilterProxyModel *proxyModel = qobject_cast<QSortFilterProxyModel*>(chatWindow->rosterView()->model());
    chatWindow->rosterView()->expand(proxyModel->mapFromSource(index));

    /* prepare network manager */
    network = new QNetworkAccessManager(this);
    check = connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                    QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    Q_ASSERT(check);

    check = connect(chatWindow->client(), SIGNAL(connected()),
                    this, SLOT(fetchMenu()));
    Q_ASSERT(check);
}

Menu::~Menu()
{
    delete servicesMenu;
}

void Menu::fetchIcon(const QUrl &url, QAction *action)
{
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", userAgent);
    QNetworkReply *reply = network->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(showIcon()));
    icons[reply] = action;
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

void Menu::openUrl()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        QDesktopServices::openUrl(action->data().toUrl());
}

void Menu::showIcon()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply || !icons.contains(reply))
        return;

    QAction *action = icons.take(reply);
    QPixmap pixmap;
    QByteArray data = reply->readAll();
    if (!pixmap.loadFromData(data, 0))
        return;
    action->setIcon(QIcon(pixmap));
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

    /* clear menu bar */
    icons.clear();
    servicesMenu->clear();

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
        QUrl linkUrl(link);
        if (text.isEmpty())
            servicesMenu->addSeparator();
        else if (linkUrl.isValid())
        {
            if (linkUrl.scheme() == "xmpp")
            {
                chatWindow->rosterModel()->addItem(ChatRosterItem::Room,
                    linkUrl.path(),
                    tr("Chat room"),
                    QIcon(":/chat.png"),
                    chatWindow->rosterModel()->findItem("home"));
            }
            action = servicesMenu->addAction(text);
            action->setData(baseUrl.resolved(link));
            connect(action, SIGNAL(triggered(bool)), this, SLOT(openUrl()));

            /* request icon */
            if (!image.isEmpty())
                fetchIcon(baseUrl.resolved(image), action);
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
#endif

    refreshInterval = item.firstChildElement("refresh").text().toInt() * 1000;
    if (refreshInterval > 0)
        QTimer::singleShot(refreshInterval, this, SLOT(fetchMenu()));
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
    QString domain = chat->client()->configuration().domain();
    if (domain != "wifirst.net")
        return false;

    new Menu(chat->menuBar(), chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(menu, MenuPlugin)

