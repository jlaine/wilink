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

#include <signal.h>

#include <QLocale>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QTranslator>
#include <QtPlugin>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "application.h"
#include "updatesdialog.h"

Q_IMPORT_PLUGIN(calls)
Q_IMPORT_PLUGIN(chats)
Q_IMPORT_PLUGIN(console)
Q_IMPORT_PLUGIN(diagnostics)
Q_IMPORT_PLUGIN(discovery)
Q_IMPORT_PLUGIN(menu)
Q_IMPORT_PLUGIN(photos)
Q_IMPORT_PLUGIN(rooms)
Q_IMPORT_PLUGIN(shares)
Q_IMPORT_PLUGIN(transfers)
Q_IMPORT_PLUGIN(contacts)

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

int main(int argc, char *argv[])
{
    /* Create application */
    Application::platformInit();
    Application app(argc, argv);

    /* Adjust palette to make text selection more visible */
    QPalette palette = QApplication::palette();
    QColor highlight(51, 153, 255);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    QApplication::setPalette(palette);

    /* Load translations */
    QString localeName = QLocale::system().name().split("_").first();

    QTranslator qtTranslator;
    qtTranslator.load(QString(":/qt_%1.qm").arg(localeName));
    app.installTranslator(&qtTranslator);

    QTranslator appTranslator;
    appTranslator.load(QString(":/%1.qm").arg(localeName));
    app.installTranslator(&appTranslator);

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    /* Create system tray icon */
    app.createSystemTrayIcon();

    /* Check for updates */
    UpdatesDialog updates;
    app.setUpdatesDialog(&updates);

    /* Show chat windows */
    QTimer::singleShot(0, &app, SLOT(resetChats()));

    /* Run application */
    return app.exec();
}

