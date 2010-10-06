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
#include <QTimer>

#include "QXmppClient.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "chat.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "menu.h"

static const QUrl baseUrl("https://www.wifirst.net/wilink/menu/1");

static const QString authSuffix = "@wifirst.net";
static int retryInterval = 15000;

Menu::Menu(Chat *window)
    : QObject(window),
    refreshInterval(0),
    chatWindow(window),
    servicesMenu(0)
{
    bool check;

    userAgent = QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii();

    servicesMenu = chatWindow->menuBar()->addMenu(tr("&Services"));

    /* add roster entry */
    QModelIndex index = chatWindow->rosterModel()->addItem(ChatRosterItem::Other,
        HOME_ROSTER_ID,
        tr("My residence"),
        QIcon(":/favorite-active.png"));
    ChatRosterView *rosterView = chatWindow->rosterView();
    rosterView->expand(rosterView->mapFromRoster(index));

    /* prepare network manager */
    Application *wApp = qobject_cast<Application*>(qApp);
    Q_ASSERT(wApp);
    network = new QNetworkAccessManager(this);
    check = connect(network, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                    QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    Q_ASSERT(check);

    check = connect(chatWindow->client(), SIGNAL(connected()),
                    this, SLOT(fetchMenu()));
    Q_ASSERT(check);
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
    if (!action)
        return;

    const QUrl linkUrl = action->data().toUrl();
    if (linkUrl.scheme() == "xmpp")
        chatWindow->openUrl(linkUrl);
    else
        QDesktopServices::openUrl(linkUrl);
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
            if (linkUrl.scheme() == "xmpp" && linkUrl.hasQueryItem("join"))
            {
                /* add or move chat room entry */
                ChatRosterModel *model = chatWindow->rosterModel();
                QModelIndex homeIndex = model->findItem(HOME_ROSTER_ID);
                QModelIndex index = model->findItem(linkUrl.path());
                if (index.isValid())
                    model->reparentItem(index, homeIndex);
                else
                {
                    index = model->addItem(ChatRosterItem::Room,
                        linkUrl.path(), tr("Chat room"), QIcon(":/chat.png"),
                        homeIndex);
                    QMetaObject::invokeMethod(chatWindow, "rosterClicked", Q_ARG(QModelIndex, index));
                }
                model->setData(index, true, ChatRosterModel::PersistentRole);
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

    /* parse preferences */
    QDomElement preferences = doc.documentElement().firstChildElement("preferences");
    refreshInterval = preferences.firstChildElement("refresh").text().toInt() * 1000;
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
    QString domain = chat->client()->configuration().domain();
    if (domain != "wifirst.net")
        return false;

    new Menu(chat);
    return true;
}

Q_EXPORT_STATIC_PLUGIN2(menu, MenuPlugin)

