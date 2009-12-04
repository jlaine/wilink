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

#include "qnetio/wallet.h"
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
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon(":/wDesktop.png"));

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Setup system tray icon */
    TrayIcon trayIcon;

    /* Run application */
    return app.exec();
}

