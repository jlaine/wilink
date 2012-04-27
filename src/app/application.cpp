/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#include <QMenu>
#include <QSslSocket>
#include <QSystemTrayIcon>
#include <QThread>
#include <QUrl>

#include "QSoundPlayer.h"

#include "application.h"
#include "declarative.h"
#include "notifications.h"
#include "settings.h"
#include "systeminfo.h"

Application *wApp = 0;

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    ApplicationSettings *appSettings;
    QSoundPlayer *soundPlayer;
    QThread *soundThread;
#ifdef USE_SYSTRAY
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    Notification *trayNotification;
#endif
};

ApplicationPrivate::ApplicationPrivate()
    : appSettings(0)
#ifdef USE_SYSTRAY
    , trayIcon(0)
    , trayMenu(0)
    , trayNotification(0)
#endif
{
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv),
    d(new ApplicationPrivate)
{
    bool check;
    Q_UNUSED(check);

    wApp = this;

    // set application properties
    setApplicationName("wiLink");
    setApplicationVersion(WILINK_VERSION);
    setOrganizationDomain("wifirst.net");
    setOrganizationName("Wifirst");
    setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    setWindowIcon(QIcon(":/32x32/wiLink.png"));
#endif

    // initialise settings
    d->appSettings = new ApplicationSettings(this);
    if (d->appSettings->openAtLogin())
        d->appSettings->setOpenAtLogin(true);

    // initialise sound player
    d->soundThread = new QThread(this);
    d->soundThread->start();
    d->soundPlayer = new QSoundPlayer;
    d->soundPlayer->setInputDeviceName(d->appSettings->audioInputDeviceName());
    check = connect(d->appSettings, SIGNAL(audioInputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setInputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setOutputDeviceName(d->appSettings->audioOutputDeviceName());
    check = connect(d->appSettings, SIGNAL(audioOutputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setOutputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setNetworkAccessManager(new NetworkAccessManager(d->soundPlayer));
    d->soundPlayer->moveToThread(d->soundThread);

    // add SSL root CA for wifirst.net and download.wifirst.net
    QSslSocket::addDefaultCaCertificates(":/UTN_USERFirst_Hardware_Root_CA.pem");
}

Application::~Application()
{
#ifdef USE_SYSTRAY
    // destroy tray icon
    if (d->trayIcon)
    {
#ifdef Q_OS_WIN
        // FIXME : on Windows, deleting the icon crashes the program
        d->trayIcon->hide();
#else
        delete d->trayIcon;
        delete d->trayMenu;
#endif
    }
#endif

    // stop sound player
    d->soundThread->quit();
    d->soundThread->wait();
    delete d->soundPlayer;

    delete d;
}

#ifndef Q_OS_MAC
void Application::alert(QWidget *widget)
{
    QApplication::alert(widget);
}

void Application::platformInit()
{
}
#endif

/** Create the system tray icon.
 */
void Application::createSystemTrayIcon()
{
#ifdef USE_SYSTRAY
    d->trayIcon = new QSystemTrayIcon;
#ifdef Q_OS_MAC
    d->trayIcon->setIcon(QIcon(":/32x32/wiLink-black.png"));
#else
    d->trayIcon->setIcon(QIcon(":/32x32/wiLink.png"));
#endif

    d->trayMenu = new QMenu;
    QAction *action = d->trayMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered()),
            this, SLOT(quit()));
    d->trayIcon->setContextMenu(d->trayMenu);

    connect(d->trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
    connect(d->trayIcon, SIGNAL(messageClicked()),
            this, SLOT(trayClicked()));
    d->trayIcon->show();
#endif
}

bool Application::isMobile() const
{
#ifdef WILINK_EMBEDDED
    return true;
#else
    return false;
#endif
}

QString Application::osType() const
{
    return SystemInfo::osType();
}

QUrl Application::resolvedUrl(const QUrl &url, const QUrl &base)
{
    return base.resolved(url);
}

QSoundPlayer *Application::soundPlayer()
{
    return d->soundPlayer;
}

#ifdef USE_SYSTRAY
void Application::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason != QSystemTrayIcon::Context)
        emit showWindows();
}

void Application::trayClicked()
{
    if (d->trayNotification) {
        QMetaObject::invokeMethod(d->trayNotification, "clicked");
        d->trayNotification = 0;
    }
}
#endif
