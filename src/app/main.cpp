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
#include <csignal>

#include <QApplication>
#include <QFileOpenEvent>
#include <QFontDatabase>
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

static QtLocalPeer *peer = 0;
static int aborted = 0;
static void signal_handler(int sig)
{
    Q_UNUSED(sig);

    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

class CustomApplication : public QApplication
{
public:
    CustomApplication(int argc, char *argv[])
        : QApplication(argc, argv)
    {
    }

protected:
    bool event(QEvent *event);
};

bool CustomApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        const QUrl url = static_cast<QFileOpenEvent *>(event)->url();
        if (url.isValid() && url.scheme() == "wilink") {
            QMetaObject::invokeMethod(peer, "messageReceived", Q_ARG(QString, "OPEN " + url.toString()));
            return true;
        }
    }
    return QApplication::event(event);
}

static void usage()
{
    printf("Usage: wiLink [options] [open_url]\n");
    printf("Options:\n");
    printf(" --help               Display this information\n");
    printf(" --qmlroot <url>      Fetch QML from the specified root URL\n");
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_MAC
    mac_init();
#endif

    /* Create application */
    CustomApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion(WILINK_VERSION);
    app.setOrganizationDomain("wifirst.net");
    app.setOrganizationName("Wifirst");
    app.setQuitOnLastWindowClosed(false);
#ifndef Q_OS_MAC
    app.setWindowIcon(QIcon(":/app.png"));
#endif

#ifdef Q_OS_ANDROID
    /* Hack around font issue on Android simulator */
    QFontDatabase db;
    db.addApplicationFont(":/DroidSans.ttf");
#endif

    /* Parse command line arguments */
    QUrl openUrl;
    QUrl qmlRoot = QUrl(QString("https://download.wifirst.net/public/%1/%2/qml/").arg(app.applicationName(), app.applicationVersion()));
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) {
            break;
        } else if (!strcmp(argv[i], "--help")) {
            usage();
            return EXIT_SUCCESS;
        } else if (!strcmp(argv[i], "--qmlroot")) {
            if (i == argc - 1 || !strlen(argv[i+1])) {
                usage();
                return EXIT_FAILURE;
            }
            const QString arg = QString::fromLocal8Bit(argv[++i]);
            qmlRoot = QUrl(arg.endsWith("/") ? arg : arg + "/");
        } else {
            openUrl = QUrl(QString::fromLocal8Bit(argv[i]));
            break;
        }
    }

    /* Open RPC socket */
    peer = new QtLocalPeer(&app, "wiLink");
    if (peer->isClient()) {
        if (openUrl.isValid() && openUrl.scheme() == "wilink")
            peer->sendMessage("OPEN " + openUrl.toString(), 1000);
        else
            peer->sendMessage("SHOW", 1000);
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
    new CustomWindow(peer, qmlRoot);
    if (openUrl.isValid() && openUrl.scheme() == "wilink")
        QMetaObject::invokeMethod(peer, "messageReceived", Q_ARG(QString, "OPEN " + openUrl.toString()));

    /* Run application */
    int ret = app.exec();

    /* Delete window */
    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        if (qobject_cast<CustomWindow*>(widget)) {
            delete widget;
            break;
        }
    }
    return ret;
}
