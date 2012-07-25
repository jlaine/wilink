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

#include <QApplication>
#include <QLocale>
#include <QSslSocket>
#include <QTranslator>

#include "qtlocalpeer.h"

#ifdef Q_OS_MAC
void mac_init();
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
#ifdef Q_OS_MAC
    mac_init();
#endif

    /* Create application */
    QApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion(WILINK_VERSION);
    app.setOrganizationDomain("wifirst.net");
    app.setOrganizationName("Wifirst");
    app.setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    app.setWindowIcon(QIcon(":/app.png"));
#endif

    /* Parse command line arguments */
    const QString firstArgument = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QString();

    /* Open RPC socket */
    QtLocalPeer *peer = new QtLocalPeer(&app, "wiLink");
    if (peer->isClient()) {
        if (firstArgument.isEmpty())
            peer->sendMessage("SHOW", 1000);
        else
            peer->sendMessage("OPEN " + firstArgument, 1000);
        return EXIT_SUCCESS;
    }

    /* Add SSL root CA for wifirst.net and download.wifirst.net */
    QSslSocket::addDefaultCaCertificates(":/app.pem");

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
    Window window(peer);
#ifdef MEEGO_EDITION_HARMATTAN
    //QUrl qmlSource("https://download.wifirst.net/wiLink/2.3/MeegoMain.qml");
    QUrl qmlSource("qrc:/qml/MeegoMain.qml");
#else
    //QUrl qmlSource("https://download.wifirst.net/wiLink/2.3/Main.qml");
    QUrl qmlSource("qrc:/qml/Main.qml");
#endif
    if (!firstArgument.isEmpty())
        window.setInitialMessage("OPEN " + firstArgument);
    QMetaObject::invokeMethod(&window, "setSource", Qt::QueuedConnection, Q_ARG(QUrl, qmlSource));

    /* Run application */
    return app.exec();
}
