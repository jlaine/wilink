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
#include <QDesktopWidget>
#include <QMenuBar>
#include <QSettings>
#include <QShortcut>
#include <QStringList>

#include "qtlocalpeer.h"
#include "window.h"

class WindowPrivate
{
public:
    QString initialMessage;
    QtLocalPeer *peer;
    QDeclarativeView *view;
};

Window::Window(QtLocalPeer *peer, QWidget *parent)
    : QMainWindow(parent)
{
    bool check;
    Q_UNUSED(check);

    d = new WindowPrivate;
    d->peer = peer;
    check = connect(d->peer, SIGNAL(messageReceived(QString)),
                    this, SIGNAL(messageReceived(QString)));
    Q_ASSERT(check);

    // create declarative view
    d->view = new QDeclarativeView;
    d->view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
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

    // restore geometry
    const QByteArray geometry = QSettings().value("WindowGeometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        const QSize desktopSize = QApplication::desktop()->availableGeometry(this).size();
        QSize size;
        size.setHeight(desktopSize.height() - 100);
        size.setWidth((desktopSize.height() * 4.0) / 3.0);
        resize(size);
        move((desktopSize.width() - width()) / 2, (desktopSize.height() - height()) / 2);
    }
}

Window::~Window()
{
    // save geometry
    QSettings settings;
    settings.setValue("WindowGeometry", saveGeometry());
    delete d;
}

void Window::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange ||
        event->type() == QEvent::WindowStateChange)
        emit windowStateChanged();
}

void Window::setFullScreen(bool fullScreen)
{
    if (fullScreen)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

void Window::setInitialMessage(const QString &message)
{
    d->initialMessage = message;
}

void Window::showAndRaise()
{
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
}

void Window::setSource(const QUrl &source)
{
    d->view->setSource(source);
}

void Window::_q_statusChanged()
{
    if (d->view->status() != QDeclarativeView::Ready)
        return;

    d->view->setAttribute(Qt::WA_OpaquePaintEvent);
    d->view->setAttribute(Qt::WA_NoSystemBackground);
    d->view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    d->view->viewport()->setAttribute(Qt::WA_NoSystemBackground);

    if (!d->initialMessage.isEmpty()) {
        QMetaObject::invokeMethod(this, "messageReceived", Q_ARG(QString, d->initialMessage));
        d->initialMessage = QString();
    }
}
