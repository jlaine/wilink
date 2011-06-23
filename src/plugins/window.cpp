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
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>
#include <QTimer>

#include "QXmppLogger.h"

#include "application.h"
#include "declarative.h"
#include "photos.h"
#include "roster.h"
#include "updatesdialog.h"
#include "window.h"

class ChatPrivate
{
public:
    ChatClient *client;
    QDeclarativeView *rosterView;
};

Window::Window(QWidget *parent)
    : QMainWindow(parent),
    d(new ChatPrivate)
{
    bool check;

    // create client
    d->client = new ChatClient(this);

    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    d->client->setLogger(logger);

    // create declarative view
    d->rosterView = new QDeclarativeView;
    d->rosterView->setMinimumWidth(240);
    d->rosterView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    d->rosterView->engine()->addImageProvider("photo", new PhotoImageProvider);
    d->rosterView->engine()->addImageProvider("roster", new ChatRosterImageProvider);
    d->rosterView->engine()->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory);

    d->rosterView->setAttribute(Qt::WA_OpaquePaintEvent);
    d->rosterView->setAttribute(Qt::WA_NoSystemBackground);
    d->rosterView->viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
    d->rosterView->viewport()->setAttribute(Qt::WA_NoSystemBackground);

    QDeclarativeContext *context = d->rosterView->rootContext();
    context->setContextProperty("application", wApp);
    context->setContextProperty("window", this);

    setCentralWidget(d->rosterView);

    /* "File" menu */
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *action = fileMenu->addAction(QIcon(":/options.png"), tr("&Preferences"));
    action->setMenuRole(QAction::PreferencesRole);
    check = connect(action, SIGNAL(triggered()),
                    this, SIGNAL(showPreferences()));
    Q_ASSERT(check);

    if (wApp->updatesDialog())
    {
        action = fileMenu->addAction(QIcon(":/refresh.png"), tr("Check for &updates"));
        connect(action, SIGNAL(triggered(bool)), wApp->updatesDialog(), SLOT(check()));
    }

    action = fileMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
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

    // resize
    QSize size = QApplication::desktop()->availableGeometry(this).size();
    size.setHeight(size.height() - 100);
    size.setWidth((size.height() * 4.0) / 3.0);
    resize(size);
}

Window::~Window()
{
    // disconnect
    d->client->disconnectFromServer();
    delete d;
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

/** Return this window's chat client.
 */
ChatClient *Window::client()
{
    return d->client;
}

/** Open the connection to the chat server.
 *
 * @param jid
 */
bool Window::open(const QString &jid)
{
    d->client->configuration().setJid(jid);
    setObjectName(jid);

    // load QML
    d->rosterView->setSource(QUrl("qrc:/main.qml"));

    return true;
}

