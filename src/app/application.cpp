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

#include <QSslSocket>
#include <QThread>
#include <QUrl>

#include "QSoundPlayer.h"

#include "application.h"
#include "declarative.h"
#include "settings.h"
#include "systeminfo.h"

Application *wApp = 0;

class ApplicationPrivate
{
public:
    ApplicationPrivate();

    QSoundPlayer *soundPlayer;
    QThread *soundThread;
};

ApplicationPrivate::ApplicationPrivate()
    : soundPlayer(0)
    , soundThread(0)
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
    ApplicationSettings *appSettings = new ApplicationSettings(this);
    if (appSettings->openAtLogin())
        appSettings->setOpenAtLogin(true);

    // initialise sound player
    d->soundThread = new QThread(this);
    d->soundThread->start();
    d->soundPlayer = new QSoundPlayer;
    d->soundPlayer->setInputDeviceName(appSettings->audioInputDeviceName());
    check = connect(appSettings, SIGNAL(audioInputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setInputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setOutputDeviceName(appSettings->audioOutputDeviceName());
    check = connect(wSettings, SIGNAL(audioOutputDeviceNameChanged(QString)),
                    d->soundPlayer, SLOT(setOutputDeviceName(QString)));
    Q_ASSERT(check);
    d->soundPlayer->setNetworkAccessManager(new NetworkAccessManager(d->soundPlayer));
    d->soundPlayer->moveToThread(d->soundThread);

    // add SSL root CA for wifirst.net and download.wifirst.net
    QSslSocket::addDefaultCaCertificates(":/UTN_USERFirst_Hardware_Root_CA.pem");
}

Application::~Application()
{
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

