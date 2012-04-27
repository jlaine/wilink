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

#include <cstdlib>
#include <signal.h>

#include <QLocale>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "application.h"
#include "window.h"

static int aborted = 0;
static void signal_handler(int sig)
{
    Q_UNUSED(sig);

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
    signal(SIGTERM, signal_handler);

    /* Create window */
    Window window;
#ifdef MEEGO_EDITION_HARMATTAN
    //QUrl qmlSource("https://download.wifirst.net/wiLink/2.3/MeegoMain.qml");
    QUrl qmlSource("qrc:/MeegoMain.qml");
#else
    //QUrl qmlSource("https://download.wifirst.net/wiLink/2.3/Main.qml");
    QUrl qmlSource("qrc:/Main.qml");
#endif
    if (argc > 1)
        qmlSource = QUrl(QString::fromLocal8Bit(argv[1]));
    window.setSource(qmlSource);

    /* Run application */
    return app.exec();
}

