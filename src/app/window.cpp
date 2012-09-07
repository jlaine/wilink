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

#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QNetworkDiskCache>
#include <QShortcut>
#include <QStringList>

#include "qtlocalpeer.h"
#include "network.h"
#include "window.h"

#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

class NetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    NetworkAccessManagerFactory(const QString &cachePath)
        : m_cachePath(cachePath)
    {
    }

    QNetworkAccessManager *create(QObject * parent)
    {
        QNetworkAccessManager *manager = new NetworkAccessManager(parent);
        QNetworkDiskCache *cache = new QNetworkDiskCache(manager);
        cache->setCacheDirectory(m_cachePath);
        manager->setCache(cache);
        return manager;
    }

private:
    QString m_cachePath;
};

class CustomWindowPrivate
{
public:
    QStringList messages;
    bool messagesStarted;
    QtLocalPeer *peer;
    bool qmlFail;
    QUrl qmlRoot;
    QDeclarativeView *view;
};

CustomWindow::CustomWindow(QtLocalPeer *peer, const QUrl &qmlRoot, QWidget *parent)
    : QMainWindow(parent)
{
    bool check;
    Q_UNUSED(check);

    d = new CustomWindowPrivate;
    d->messagesStarted = false;
    d->peer = peer;
    d->qmlFail = false;
    d->qmlRoot = qmlRoot;

    check = connect(d->peer, SIGNAL(messageReceived(QString)),
                    this, SLOT(_q_messageReceived(QString)));
    Q_ASSERT(check);

    // create data paths
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    const QString cachePath = QDir(dataPath).filePath("cache");
    QString storagePath = QDir(dataPath).filePath("storage");
    QDir().mkpath(cachePath);
    QDir().mkpath(storagePath);
    // NOTE: for some reason we need native directory separators
    storagePath.replace(QLatin1Char('/'), QDir::separator());

    // create declarative view
    d->view = new QDeclarativeView;
    d->view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->view->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory(cachePath));
    d->view->engine()->setOfflineStoragePath(storagePath);

    check = connect(d->view, SIGNAL(statusChanged(QDeclarativeView::Status)),
                    this, SLOT(_q_statusChanged()));
    Q_ASSERT(check);

    check = connect(d->view->engine(), SIGNAL(quit()),
                    qApp, SLOT(quit()));
    Q_ASSERT(check);

    QDeclarativeContext *context = d->view->rootContext();
    context->setContextProperty("window", this);

    setCentralWidget(d->view);

#ifndef WILINK_EMBEDDED
    /* "File" menu */
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *action = fileMenu->addAction(tr("&Preferences"));
    action->setMenuRole(QAction::PreferencesRole);
    check = connect(action, SIGNAL(triggered()),
                    this, SIGNAL(showPreferences()));
    Q_ASSERT(check);

    action = fileMenu->addAction(tr("&Quit"));
    action->setMenuRole(QAction::QuitRole);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Q));
    check = connect(action, SIGNAL(triggered(bool)),
                    qApp, SLOT(quit()));
    Q_ASSERT(check);

    /* "Help" menu */
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    action = helpMenu->addAction(tr("%1 FAQ").arg(qApp->applicationName()));
#ifdef Q_OS_MAC
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Question));
#else
    action->setShortcut(QKeySequence(Qt::Key_F1));
#endif
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SIGNAL(showHelp()));
    Q_ASSERT(check);

    action = helpMenu->addAction(tr("About %1").arg(qApp->applicationName()));
    action->setMenuRole(QAction::AboutRole);
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SIGNAL(showAbout()));
    Q_ASSERT(check);
#endif

    // register url handler
    QDesktopServices::setUrlHandler("wilink", this, "_q_openUrl");

    // load source
    QMetaObject::invokeMethod(this, "_q_loadSource", Qt::QueuedConnection);
}

CustomWindow::~CustomWindow()
{
    delete d;
}

QRect CustomWindow::availableGeometry() const
{
    return QApplication::desktop()->availableGeometry(this);
}

void CustomWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange ||
        event->type() == QEvent::WindowStateChange)
        emit windowStateChanged();
}

void CustomWindow::reload()
{
    new CustomWindow(d->peer, d->qmlRoot);
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
}

void CustomWindow::setFullScreen(bool fullScreen)
{
    if (fullScreen)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void CustomWindow::showAndRaise()
{
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();

#ifdef Q_WS_X11
    // tell window manager to activate window
    XClientMessageEvent xev;
    xev.type = ClientMessage;
    xev.window = winId();
    xev.message_type = XInternAtom(QX11Info::display(), "_NET_ACTIVE_WINDOW", False);
    xev.format = 32;
    xev.data.l[0] = 2;
    xev.data.l[1] = CurrentTime;
    xev.data.l[2] = 0;
    xev.data.l[3] = 0;
    xev.data.l[4] = 0;

    XSendEvent(QX11Info::display(), QX11Info::appRootWindow(), False,
          (SubstructureNotifyMask | SubstructureRedirectMask),
          (XEvent *)&xev);
#endif
}

void CustomWindow::startMessages()
{
    foreach (const QString &message, d->messages)
        emit messageReceived(message);
    d->messages.clear();
    d->messagesStarted = true;
}

void CustomWindow::_q_loadSource()
{
#ifdef MEEGO_EDITION_HARMATTAN
    const QUrl qmlFile("boot-meego.qml");
#else
    const QUrl qmlFile("boot.qml");
#endif
    const QUrl qmlSource = (d->qmlFail ? QUrl("qrc:/qml/") : d->qmlRoot).resolved(qmlFile);
    qDebug("Window loading %s", qPrintable(qmlSource.toString()));
    d->view->setSource(qmlSource);
}

void CustomWindow::_q_messageReceived(const QString &message)
{
    if (d->messagesStarted) {
        emit messageReceived(message);
    } else {
        d->messages << message;
    }
}

void CustomWindow::_q_openUrl(const QUrl &url)
{
    _q_messageReceived("OPEN " + url.toString());
}

void CustomWindow::_q_statusChanged()
{
    if (d->view->status() == QDeclarativeView::Ready) {
        d->view->setAttribute(Qt::WA_OpaquePaintEvent);
        d->view->setAttribute(Qt::WA_NoSystemBackground);
        d->view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
        d->view->viewport()->setAttribute(Qt::WA_NoSystemBackground);
    } else if (d->view->status() == QDeclarativeView::Error) {
        if (!d->qmlFail) {
            // network load failed, use fallback
            d->qmlFail = true;
            _q_loadSource();
        } else {
            // fallback load failed too, abort
            qApp->quit();
        }
    }
}
