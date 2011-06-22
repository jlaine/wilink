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
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>
#include <QTimer>

#include "QXmppConfiguration.h"
#include "QXmppLogger.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

#include "accounts.h"
#include "application.h"
#include "declarative.h"
#include "photos.h"
#include "roster.h"
#include "window.h"
#include "updatesdialog.h"
#include "utils.h"

class ChatPrivate
{
public:
    ChatClient *client;
    ChatRosterModel *rosterModel;
    QDeclarativeView *rosterView;
    QString windowTitle;
};

Window::Window(QWidget *parent)
    : QMainWindow(parent),
    d(new ChatPrivate)
{
    bool check;

    // create client
    d->client = new ChatClient(this);
    d->rosterModel =  new ChatRosterModel(d->client, this);
    connect(d->rosterModel, SIGNAL(pendingMessages(int)), this, SLOT(pendingMessages(int)));

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
                    this, SLOT(showHelp()));
    Q_ASSERT(check);

    action = helpMenu->addAction(tr("About %1").arg(qApp->applicationName()));
    action->setMenuRole(QAction::AboutRole);
    check = connect(action, SIGNAL(triggered(bool)),
                    this, SIGNAL(showAbout()));
    Q_ASSERT(check);

    /* set up client */
    connect(d->client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));

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

/** Handle an error talking to the chat server.
 *
 * @param error
 */
void Window::error(QXmppClient::Error error)
{
    if(error == QXmppClient::XmppStreamError)
    {
        if (d->client->xmppStreamError() == QXmppStanza::Error::Conflict)
        {
            // if we received a resource conflict, exit
            qWarning("Received a resource conflict from chat server");
            qApp->quit();
        }
        else if (d->client->xmppStreamError() == QXmppStanza::Error::NotAuthorized)
        {
            // prompt user for credentials at the next main loop execution
            QTimer::singleShot(0, this, SLOT(promptCredentials()));
        }
    }
}

/** The number of pending messages changed.
 */
void Window::pendingMessages(int messages)
{
    QString title = d->windowTitle;
    if (messages)
        title += " - " + tr("%n message(s)", "", messages);
    QWidget::setWindowTitle(title);
}

/** Prompt for credentials then connect.
 */
void Window::promptCredentials()
{
    QXmppConfiguration config = d->client->configuration();
    QString password = config.password();
    if (ChatAccounts::getPassword(config.jidBare(), password, this) &&
        password != config.password())
    {
        config.setPassword(password);
        d->client->connectToServer(config);
    }
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

/** Return this window's chat roster model.
 */
ChatRosterModel *Window::rosterModel()
{
    return d->rosterModel;
}

/** Open the connection to the chat server.
 *
 * @param jid
 */
bool Window::open(const QString &jid)
{
    QXmppConfiguration config;
    config.setResource(qApp->applicationName());

    /* get user and domain */
    if (!isBareJid(jid))
    {
        qWarning("Cannot connect to chat server using invalid JID");
        return false;
    }
    config.setJid(jid);
    setObjectName(config.jidBare());

    /* get password */
    QString password;
    if (!ChatAccounts::getPassword(config.jidBare(), password, this))
    {
        qWarning("Cannot connect to chat server without a password");
        return false;
    }
    config.setPassword(password);

    /* set security parameters */
    if (config.domain() == QLatin1String("wifirst.net"))
    {
        config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
        config.setIgnoreSslErrors(false);
    }

    /* set keep alive */
    config.setKeepAliveTimeout(15);

    /* connect to server */
    d->client->connectToServer(config);

    /* load QML */
    d->rosterView->setSource(QUrl("qrc:/main.qml"));

    return true;
}

void Window::setWindowTitle(const QString &title)
{
    d->windowTitle = title;
    QWidget::setWindowTitle(title);
}

/** Display the help web page.
 */
void Window::showHelp()
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

