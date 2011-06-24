/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>

#include "application.h"
#include "declarative.h"
#include "window.h"

Window::Window(const QUrl &url, const QString &jid, QWidget *parent)
    : QMainWindow(parent)
{
    bool check;
    setObjectName(jid);
    if (jid.isEmpty())
        setWindowTitle(qApp->applicationName());
    else
        setWindowTitle(QString("%1 - %2").arg(jid, qApp->applicationName()));

    // FIXME: set logger
    //logger->setLoggingType(QXmppLogger::SignalLogging);

    // create declarative view
    QDeclarativeView *view = new QDeclarativeView;
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    // load plugin
    Plugin plugin;
    plugin.registerTypes("wiLink");
    plugin.initializeEngine(view->engine(), "wiLink");

    view->setAttribute(Qt::WA_OpaquePaintEvent);
    view->setAttribute(Qt::WA_NoSystemBackground);
    view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    view->viewport()->setAttribute(Qt::WA_NoSystemBackground);

    QDeclarativeContext *context = view->rootContext();
    context->setContextProperty("application", wApp);
    context->setContextProperty("window", this);

    setCentralWidget(view);

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

    /* set up keyboard shortcuts */
#ifdef Q_OS_MAC
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_W), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
#endif

    setMinimumHeight(240);
    setMinimumWidth(320);

    // load QML
    view->setSource(url);
}

void Window::alert()
{
    // show the chat window
    if (!isVisible()) {
#ifdef Q_OS_MAC
        show();
#else
        showMinimized();
#endif
    }

    /* NOTE : in Qt built for Mac OS X using Cocoa, QApplication::alert
     * only causes the dock icon to bounce for one second, instead of
     * bouncing until the user focuses the window. To work around this
     * we implement our own version.
     */
    wApp->alert(this);
}

void Window::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
        emit isActiveWindowChanged();
}

QFileDialog *Window::fileDialog()
{
    QFileDialog *dialog = new QDeclarativeFileDialog(this);
    return dialog;
}

QMessageBox *Window::messageBox()
{
    QMessageBox *box = new QMessageBox(this);
    box->setIcon(QMessageBox::Question);
    return box;
}

