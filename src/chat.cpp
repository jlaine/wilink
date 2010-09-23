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

#include <QApplication>
#include <QAuthenticator>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QPluginLoader>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>

#include "QXmppConfiguration.h"
#include "QXmppConstants.h"
#include "QXmppLogger.h"
#include "QXmppMessage.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"

#include "application.h"
#include "chat.h"
#include "chat_accounts.h"
#include "chat_client.h"
#include "chat_panel.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_roster_item.h"
#include "chat_status.h"
#include "chat_utils.h"
#include "flickcharm.h"
#include "systeminfo.h"
#include "updatesdialog.h"

Chat::Chat(QWidget *parent)
    : QMainWindow(parent)
{
    m_client = new ChatClient(this);
    m_rosterModel =  new ChatRosterModel(m_client, this);
    connect(m_rosterModel, SIGNAL(rosterReady()), this, SLOT(resizeContacts()));
    connect(m_rosterModel, SIGNAL(pendingMessages(int)), this, SLOT(pendingMessages(int)));

    /* set up logger */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    m_client->setLogger(logger);

    /* build splitter */
    QSplitter *splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    /* left panel */
    m_rosterView = new ChatRosterView(m_rosterModel);
#ifdef WILINK_EMBEDDED
    m_rosterView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    FlickCharm *charm = new FlickCharm(this);
    charm->activateOn(m_rosterView);
#endif
    connect(m_rosterView, SIGNAL(clicked(QModelIndex)), this, SLOT(rosterClicked(QModelIndex)));
    connect(m_rosterView, SIGNAL(itemMenu(QMenu*, QModelIndex)), this, SIGNAL(rosterMenu(QMenu*, QModelIndex)));
    connect(m_rosterView, SIGNAL(itemDrop(QDropEvent*, QModelIndex)), this, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)));
    splitter->addWidget(m_rosterView);
    splitter->setStretchFactor(0, 0);

    /* right panel */
    m_conversationPanel = new QStackedWidget;
    m_conversationPanel->hide();
    connect(m_conversationPanel, SIGNAL(currentChanged(int)), this, SLOT(panelChanged(int)));
    splitter->addWidget(m_conversationPanel);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    /* build status bar */
    statusBar()->setSizeGripEnabled(false);
    // avoid border around widgets on OS X
    statusBar()->setStyleSheet("QStatusBar::item { border: none; }");

    statusBar()->addPermanentWidget(new ChatStatus(m_client));

    /* get handle to application */
    Application *wApp = qobject_cast<Application*>(qApp);
    Q_ASSERT(wApp);

    /* create menu */
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_optionsMenu = m_fileMenu->addMenu(QIcon(":/options.png"), tr("&Options"));

    QAction *action = m_optionsMenu->addAction(QIcon(":/chat.png"), tr("Chat accounts"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(showAccounts()));

    action = m_optionsMenu->addAction(QIcon(":/contact-offline.png"), tr("Show offline contacts"));
    action->setCheckable(true);
    action->setChecked(wApp->showOfflineContacts());
    m_rosterView->setShowOfflineContacts(wApp->showOfflineContacts());
    connect(action, SIGNAL(toggled(bool)), wApp, SLOT(setShowOfflineContacts(bool)));
    connect(wApp, SIGNAL(showOfflineContactsChanged(bool)), m_rosterView, SLOT(setShowOfflineContacts(bool)));

    if (wApp->isInstalled())
    {
        action = m_optionsMenu->addAction(QIcon(":/favorite-active.png"), tr("Open at login"));
        action->setCheckable(true);
        action->setChecked(wApp->openAtLogin());
        connect(action, SIGNAL(toggled(bool)), wApp, SLOT(setOpenAtLogin(bool)));
        connect(wApp, SIGNAL(openAtLoginChanged(bool)), action, SLOT(setChecked(bool)));
    }
    QObject::connect(wApp, SIGNAL(messageClicked(QWidget*)),
        this, SLOT(messageClicked(QWidget*)));

    if (wApp->updatesDialog())
    {
        action = m_fileMenu->addAction(QIcon(":/refresh.png"), tr("Check for &updates"));
        connect(action, SIGNAL(triggered(bool)), wApp->updatesDialog(), SLOT(check()));
    }

    action = m_fileMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Q));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

    /* "Edit" menu */
    QMenu *m_editMenu = menuBar()->addMenu(tr("&Edit"));

    m_findAction = m_editMenu->addAction(QIcon(":/search.png"), tr("&Find"));
    m_findAction->setEnabled(false);
    m_findAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_F));

    m_findAgainAction = m_editMenu->addAction(tr("Find a&gain"));
    m_findAgainAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_G));
    m_findAgainAction->setEnabled(false);

    /* set up client */
    connect(m_client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));

    /* set up keyboard shortcuts */
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_N), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(detachPanel()));
#ifdef Q_OS_MAC
    shortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_W), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(close()));
#endif
}

Chat::~Chat()
{
    foreach (ChatPanel *panel, m_chatPanels)
        if (!panel->parent())
            delete panel;

    // disconnect
    m_client->disconnectFromServer();

    // unload plugins
    for (int i = m_plugins.size() - 1; i >= 0; i--)
        m_plugins[i]->finalize(this);
}

/** Connect signals for the given panel.
 *
 * @param panel
 */
void Chat::addPanel(ChatPanel *panel)
{
    if (m_chatPanels.contains(panel))
        return;
    connect(panel, SIGNAL(destroyed(QObject*)), this, SLOT(destroyPanel(QObject*)));
    connect(panel, SIGNAL(dropPanel(QDropEvent*)), this, SLOT(dropPanel(QDropEvent*)));
    connect(panel, SIGNAL(hidePanel()), this, SLOT(hidePanel()));
    connect(panel, SIGNAL(notifyPanel(QString, int)), this, SLOT(notifyPanel(QString, int)));
    connect(panel, SIGNAL(registerPanel()), this, SLOT(registerPanel()));
    connect(panel, SIGNAL(showPanel()), this, SLOT(showPanel()));
    connect(panel, SIGNAL(unregisterPanel()), this, SLOT(unregisterPanel()));
    m_chatPanels << panel;
}

/** When a panel is destroyed, from it from our list of panels.
 *
 * @param obj
 */
void Chat::destroyPanel(QObject *obj)
{
    m_chatPanels.removeAll(static_cast<ChatPanel*>(obj));
}

/** Detach the current panel as a window.
 */
void Chat::detachPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(m_conversationPanel->currentWidget());
    if (!panel)
        return;

    QPoint oldPos = m_conversationPanel->mapToGlobal(panel->pos());
    oldPos.setY(oldPos.y() - 20);
    if (m_conversationPanel->count() == 1)
    {
        m_conversationPanel->hide();
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    m_conversationPanel->removeWidget(panel);
    panel->setParent(0, Qt::Window);
    panel->move(oldPos);
    panel->show();
}

void Chat::dropPanel(QDropEvent *event)
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(sender());
    if (!panel)
        return;

    // notify plugins
    QModelIndex index = m_rosterModel->findItem(panel->objectName());
    if (index.isValid())
        emit rosterDrop(event, index);
}

/** Hide a panel.
 */
void Chat::hidePanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (m_conversationPanel->indexOf(panel) < 0)
    {
        panel->hide();
        return;
    }

    // close view
    if (m_conversationPanel->count() == 1)
    {
        m_conversationPanel->hide();
#ifdef WILINK_EMBEDDED
        m_rosterView->show();
#endif
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    m_conversationPanel->removeWidget(panel);
}

/** Notify the user of activity on a panel.
 */
void Chat::notifyPanel(const QString &message, int options)
{
    Application *wApp = qobject_cast<Application*>(qApp);
    QWidget *panel = qobject_cast<QWidget*>(sender());
    QWidget *window = panel->isVisible() ? panel->window() : this;

    // add pending message
    bool showMessage = (options & ChatPanel::ForceNotification);
    if (!window->isActiveWindow() || (window == this && m_conversationPanel->currentWidget() != panel))
    {
        m_rosterModel->addPendingMessage(panel->objectName());
        showMessage = true;
    }
    if (showMessage)
        wApp->showMessage(panel, panel->windowTitle(), message);

    // show the chat window
    if (!window->isVisible())
    {
#ifdef Q_OS_MAC
        window->show();
#else
        window->showMinimized();
#endif
    }

    /* NOTE : in Qt built for Mac OS X using Cocoa, QApplication::alert
     * only causes the dock icon to bounce for one second, instead of
     * bouncing until the user focuses the window. To work around this
     * we implement our own version.
     */
    wApp->alert(window);
}

/** Register a panel in the roster list.
  */
void Chat::registerPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(sender());
    if (!panel)
        return;

    m_rosterModel->addItem(panel->objectType(),
        panel->objectName(),
        panel->windowTitle(),
        panel->windowIcon());
}

/** Show a panel.
 */
void Chat::showPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(sender());
    if (!panel)
        return;

    // register panel
    m_rosterModel->addItem(panel->objectType(),
        panel->objectName(),
        panel->windowTitle(),
        panel->windowIcon());

    // if the panel is detached, stop here
    if (panel->isVisible() && !panel->parent())
    {
        panel->raise();
        panel->activateWindow();
        panel->setFocus();
        return;
    }

    // add panel
    if (m_conversationPanel->indexOf(panel) < 0)
    {
        m_conversationPanel->addWidget(panel);
#ifdef WILINK_EMBEDDED
        m_rosterView->hide();
#endif
        m_conversationPanel->show();
        if (m_conversationPanel->count() == 1)
            resizeContacts();
    }

    // make sure window is visible
    if (!isActiveWindow())
    {
        setWindowState(windowState() & ~Qt::WindowMinimized);
        show();
        raise();
        activateWindow();
    }
    m_conversationPanel->setCurrentWidget(panel);
}

/** Handle switching between panels.
 *
 * @param index
 */
void Chat::panelChanged(int index)
{
    disconnect(m_findAction, SIGNAL(triggered(bool)));
    disconnect(m_findAgainAction, SIGNAL(triggered(bool)));

    QWidget *widget = m_conversationPanel->widget(index);
    if (!widget)
    {
        m_findAction->setEnabled(false);
        m_findAgainAction->setEnabled(false);
        return;
    }

    // connect find actions
    connect(m_findAction, SIGNAL(triggered(bool)), widget, SIGNAL(findPanel()));
    connect(m_findAgainAction, SIGNAL(triggered(bool)), widget, SIGNAL(findAgainPanel()));
    m_findAction->setEnabled(true);
    m_findAgainAction->setEnabled(true);

    m_rosterModel->clearPendingMessages(widget->objectName());

    // select the corresponding roster entry
    QModelIndex rosterIndex = m_rosterModel->findItem(widget->objectName());
    m_rosterView->setCurrentIndex(m_rosterView->mapFromRoster(rosterIndex));

    widget->setFocus();
}

/** When the window is activated, pass focus to the active chat.
 *
 * @param event
 */
void Chat::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
    {
        int index = m_conversationPanel->currentIndex();
        if (index >= 0)
            panelChanged(index);
    }
}

/** Handle an error talking to the chat server.
 *
 * @param error
 */
void Chat::error(QXmppClient::Error error)
{
    if(error == QXmppClient::XmppStreamError)
    {
        if (m_client->xmppStreamError() == QXmppStanza::Error::Conflict)
        {
            // if we received a resource conflict, exit
            qWarning("Received a resource conflict from chat server");
            qApp->quit();
        }
        else if (m_client->xmppStreamError() == QXmppStanza::Error::NotAuthorized)
        {
            // prompt user for credentials at the next main loop execution
            QTimer::singleShot(0, this, SLOT(promptCredentials()));
        }
    }
}

/** The number of pending messages changed.
 */
void Chat::pendingMessages(int messages)
{
    QString title = m_windowTitle;
    if (messages)
        title += " - " + tr("%n message(s)", "", messages);
    QWidget::setWindowTitle(title);
}

/** Prompt for credentials then connect.
 */
void Chat::promptCredentials()
{
    QXmppConfiguration config = m_client->configuration();
    QString password = config.password();
    if (ChatAccounts::getPassword(config.jidBare(), password, this) &&
        password != config.password())
    {
        config.setPassword(password);
        m_client->connectToServer(config);
    }
}

/** Return this window's chat client.
 */
QXmppClient *Chat::client()
{
    return m_client;
}

/** Return this window's chat roster model.
 */
ChatRosterModel *Chat::rosterModel()
{
    return m_rosterModel;
}

/** Return this window's chat roster view.
 */
ChatRosterView *Chat::rosterView()
{
    return m_rosterView;
}

/** Open the connection to the chat server.
 *
 * @param jid
 */
bool Chat::open(const QString &jid)
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
#ifndef Q_OS_SYMBIAN
        config.setIgnoreSslErrors(false);
#endif
    }

    /* set keep alive */
    config.setKeepAliveTimeout(15);

    /* connect to server */
    m_client->connectToServer(config);

    /* load plugins */
    QObjectList plugins = QPluginLoader::staticInstances();
    foreach (QObject *object, plugins)
    {
        ChatPlugin *plugin = qobject_cast<ChatPlugin*>(object);
        if (plugin)
        {
            plugin->initialize(this);
            m_plugins << plugin;
        }
    }

    /* Create "Help" menu here, so that it remains last */
    m_helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *action = m_helpMenu->addAction(tr("%1 FAQ").arg(qApp->applicationName()));
#ifdef Q_OS_MAC
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Question));
#else
    action->setShortcut(QKeySequence(Qt::Key_F1));
#endif
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showHelp()));

    action = m_helpMenu->addAction(tr("About %1").arg(qApp->applicationName()));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    return true;
}

/** Return this window's "File" menu.
 */
QMenu *Chat::fileMenu()
{
    return m_fileMenu;
}

/** Handle a click on a system tray message.
 */
void Chat::messageClicked(QWidget *context)
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(context);
    if (panel && m_chatPanels.contains(panel))
        QTimer::singleShot(0, panel, SIGNAL(showPanel()));
}

/** Return this window's "Options" menu.
 */
QMenu *Chat::optionsMenu()
{
    return m_optionsMenu;
}

/** Find a panel by its object name.
 *
 * @param objectName
 */
ChatPanel *Chat::panel(const QString &objectName)
{
    foreach (ChatPanel *panel, m_chatPanels)
        if (panel->objectName() == objectName)
            return panel;
    return 0;
}

/** Try to resize the window to fit the contents of the contacts list.
 */
void Chat::resizeContacts()
{
#ifndef WILINK_EMBEDDED
    QSize hint = m_rosterView->sizeHint();
    hint.setHeight(hint.height() + m_rosterView->sizeHintForRow(0) + 4);
    QSize barHint = statusBar()->sizeHint();
    hint.setHeight(hint.height() + barHint.height());
    if (barHint.width() > hint.width())
        hint.setWidth(barHint.width());
    if (m_conversationPanel->isVisible())
        hint.setWidth(hint.width() + 500);

    // respect current size
    if (m_conversationPanel->isVisible() && hint.width() < size().width())
        hint.setWidth(size().width());
    if (hint.height() < size().height())
        hint.setHeight(size().height());

    /* Make sure we do not resize to a size exceeding the desktop size
     * + some padding for the window title.
     */
    QDesktopWidget *desktop = QApplication::desktop();
    const QRect &available = desktop->availableGeometry(this);
    if (hint.height() > available.height() - 100)
    {
        hint.setHeight(available.height() - 100);
        hint.setWidth(hint.width() + 32);
    }

    resize(hint);
#endif
}

void Chat::rosterClicked(const QModelIndex &index)
{
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // notify plugins
    emit rosterClick(index);

    // show requested panel
    ChatPanel *chatPanel = panel(jid);
    if (chatPanel)
        QTimer::singleShot(0, chatPanel, SIGNAL(showPanel()));
}

void Chat::setWindowTitle(const QString &title)
{
    m_windowTitle = title;
    QWidget::setWindowTitle(title);
}

/** Display an "about" dialog.
 */
void Chat::showAbout()
{
    QDialog dlg;
    dlg.setWindowTitle(tr("About %1").arg(qApp->applicationName()));

    QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/wiLink-64.png"));
    hbox->addWidget(icon);
    hbox->addSpacing(20);
    hbox->addWidget(new QLabel(QString("<p style=\"font-size: xx-large;\">%1</p>"
        "<p style=\"font-size: large;\">%2</p>")
        .arg(qApp->applicationName(),
            tr("version %1").arg(qApp->applicationVersion()))));
    layout->addItem(hbox);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    layout->addWidget(buttonBox);
    dlg.setLayout(layout);
    dlg.exec();
}

/** Display the help web page.
 */
void Chat::showHelp()
{
    QDesktopServices::openUrl(QUrl(HELP_URL));
}

/** Unregister a panel from the roster list.
 */
void Chat::unregisterPanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (!panel)
        return;

    m_rosterModel->removeItem(panel->objectName());
}

