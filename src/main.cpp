/*
 * wDesktop
 * Copyright (C) 2009 Bolloré telecom
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

#include <signal.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "config.h"
#include "qnetio/wallet.h"
#include "application.h"
#include "trayicon.h"

static void signal_handler(int sig)
{
    qApp->quit();
}

int main(int argc, char *argv[])
{
    /* Create application */
    QApplication app(argc, argv);
    app.setApplicationName("wDesktop");
    app.setApplicationVersion(WDESKTOP_VERSION);
    app.setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    app.setWindowIcon(QIcon(":/wDesktop.png"));
#endif

    /* Make sure application runs at login */
    if (Application::isInstalled())
        Application::setRunAtLogin(true);

    /* Load translations */
    QTranslator translator;
    translator.load(QString(":/%1.qm").arg(QLocale::system().name().split("_").first()));
    app.installTranslator(&translator);

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Setup system tray icon */
    TrayIcon trayIcon;
    trayIcon.show();

    /* Run application */
    return app.exec();
}

