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
#include <QComboBox>
#include <QDebug>
#include <QDesktopWidget>
#include <QLayout>
#include <QList>
#include <QMenuBar>
#include <QPluginLoader>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>

#include "idle/idle.h"

#include "qxmpp/QXmppConfiguration.h"
#include "qxmpp/QXmppConstants.h"
#include "qxmpp/QXmppLogger.h"
#include "qxmpp/QXmppMessage.h"
#include "qxmpp/QXmppRoster.h"
#include "qxmpp/QXmppRosterIq.h"
#include "qxmpp/QXmppUtils.h"

#include "qnetio/dns.h"

#include "application.h"
#include "chat.h"
#include "chat_client.h"
#include "chat_dialog.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_roster_item.h"
#include "systeminfo.h"
#include "utils.h"

#define AWAY_TIME 300 // set away after 50s

using namespace QNetIO;

enum StatusIndexes {
    AvailableIndex = 0,
    BusyIndex = 1,
    AwayIndex = 2,
    OfflineIndex = 3,
};

Chat::Chat(QWidget *parent)
    : QMainWindow(parent),
    autoAway(false),
    isBusy(false),
    isConnected(false)
{
    setWindowIcon(QIcon(":/chat.png"));

    m_client = new ChatClient(this);
    m_rosterModel =  new ChatRosterModel(m_client);

    /* set up idle monitor */
    m_idle = new Idle;
    connect(m_idle, SIGNAL(secondsIdle(int)), this, SLOT(secondsIdle(int)));
    m_idle->start();

    /* set up logger */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    m_client->setLogger(logger);

    /* build splitter */
    QSplitter *splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    /* left panel */
    m_rosterView = new ChatRosterView(m_rosterModel);
    connect(m_rosterView, SIGNAL(clicked(QModelIndex)), this, SLOT(rosterClicked(QModelIndex)));
    connect(m_rosterView, SIGNAL(itemMenu(QMenu*, QModelIndex)), this, SIGNAL(rosterMenu(QMenu*, QModelIndex)));
    connect(m_rosterView, SIGNAL(itemDrop(QDropEvent*, QModelIndex)), this, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)));
    connect(m_rosterView->model(), SIGNAL(modelReset()), this, SLOT(resizeContacts()));
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
    // avoid border around widgets on OS X
    statusBar()->setStyleSheet("QStatusBar::item { border: none; }");

    m_statusCombo = new QComboBox;
    m_statusCombo->addItem(QIcon(":/contact-available.png"), tr("Available"));
    m_statusCombo->addItem(QIcon(":/contact-busy.png"), tr("Busy"));
    m_statusCombo->addItem(QIcon(":/contact-away.png"), tr("Away"));
    m_statusCombo->addItem(QIcon(":/contact-offline.png"), tr("Offline"));
    m_statusCombo->setCurrentIndex(OfflineIndex);
    connect(m_statusCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(statusChanged(int)));
    statusBar()->addPermanentWidget(m_statusCombo);

    /* create menu */
    QMenu *menu = menuBar()->addMenu("&File");

    m_optionsMenu = menu->addMenu(QIcon(":/options.png"), tr("&Options"));

    QAction *action = m_optionsMenu->addAction(tr("Chat accounts"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(showAccounts()));

    Application *wApp = qobject_cast<Application*>(qApp);
    if (wApp && wApp->isInstalled())
    {
        action = m_optionsMenu->addAction(tr("Open at login"));
        action->setCheckable(true);
        action->setChecked(wApp->openAtLogin());
        connect(action, SIGNAL(toggled(bool)), wApp, SLOT(setOpenAtLogin(bool)));
    }

    action = menu->addAction(QIcon(":/close.png"), tr("&Quit"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

    /* set up client */
    connect(m_client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));
    connect(m_client, SIGNAL(messageReceived(const QXmppMessage&)), this, SLOT(messageReceived(const QXmppMessage&)));
    connect(m_client, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_client, SIGNAL(disconnected()), this, SLOT(disconnected()));

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
    delete m_idle;
    foreach (ChatPanel *panel, m_chatPanels)
        delete panel;

    // disconnect
    m_client->disconnect();
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
    connect(panel, SIGNAL(notifyPanel()), this, SLOT(notifyPanel()));
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
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    m_conversationPanel->removeWidget(panel);
}

/** Notify the user of activity on a panel.
 */
void Chat::notifyPanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());

    // add pending message
    if (!isActiveWindow() || m_conversationPanel->currentWidget() != panel)
        m_rosterModel->addPendingMessage(panel->objectName());

    // show the chat window
    if (!isVisible())
    {
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
    Application::alert(panel);
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

    // add panel
    if (m_conversationPanel->indexOf(panel) < 0)
    {
        m_conversationPanel->addWidget(panel);
        m_conversationPanel->show();
        if (m_conversationPanel->count() == 1)
            resizeContacts();
    }
    m_conversationPanel->setCurrentWidget(panel);
}

/** Handle switching between panels.
 *
 * @param index
 */
void Chat::panelChanged(int index)
{
    QWidget *widget = m_conversationPanel->widget(index);
    if (!widget)
        return;
    m_rosterModel->clearPendingMessages(widget->objectName());
    m_rosterView->selectContact(widget->objectName());
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

/** Handle successful connection to the chat server.
 */
void Chat::connected()
{
    isConnected = true;
    m_statusCombo->setCurrentIndex(isBusy ? BusyIndex : AvailableIndex);
}

/** Handle disconnection from the chat server.
 */
void Chat::disconnected()
{
    isConnected = false;
    m_statusCombo->setCurrentIndex(OfflineIndex);
}

/** Handle an error talking to the chat server.
 *
 * @param error
 */
void Chat::error(QXmppClient::Error error)
{
    if(error == QXmppClient::XmppStreamError &&
       m_client->getXmppStreamError() == QXmppClient::ConflictStreamError)
    {
        // if we received a resource conflict, exit
        qWarning("Received a resource conflict from chat server");
        qApp->quit();
    }
}

/** Handle a chat message.
 */
void Chat::messageReceived(const QXmppMessage &msg)
{
    const QString bareJid = jidToBareJid(msg.from());

    if (msg.type() == QXmppMessage::Chat && !panel(bareJid) && !msg.body().isEmpty())
    {
        ChatDialog *dialog = new ChatDialog(m_client, m_rosterModel, bareJid);
        addPanel(dialog);
        dialog->messageReceived(msg);
    }
}

/** Return this window's chat client.
 */
ChatClient *Chat::client()
{
    return m_client;
}

/** Return this window's chat roster model.
 */
ChatRosterModel *Chat::rosterModel()
{
    return m_rosterModel;
}

/** Open the connection to the chat server.
 *
 * @param jid
 * @param password
 */
bool Chat::open(const QString &jid, const QString &password, bool ignoreSslErrors)
{
    QXmppConfiguration config;
    config.setResource(qApp->applicationName());

    /* get user and domain */
    if (!isBareJid(jid))
    {
        qWarning("Cannot connect to chat server using invalid JID");
        return false;
    }
    QStringList bits = jid.split("@");
    config.setPasswd(password);
    config.setUser(bits[0]);
    config.setDomain(bits[1]);

    /* get the server */
    QList<ServiceInfo> results;
    if (ServiceInfo::lookupService("_xmpp-client._tcp." + config.domain(), results))
    {
        config.setHost(results[0].hostName());
        config.setPort(results[0].port());
    } else {
        config.setHost(config.domain());
    }

    /* set security parameters */
    config.setAutoAcceptSubscriptions(false);
    //config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);
    config.setIgnoreSslErrors(ignoreSslErrors);

    /* set keep alive */
    config.setKeepAliveInterval(60);
    config.setKeepAliveTimeout(15);

    /* connect to server */
    m_client->connectToServer(config);

    /* load plugins */
    QObjectList plugins = QPluginLoader::staticInstances();
    foreach (QObject *object, plugins)
    {
        ChatPlugin *plugin = qobject_cast<ChatPlugin*>(object);
        if (plugin)
            plugin->initialize(this);
    }
    return true;
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
    QSize hint = m_rosterView->sizeHint();
    hint.setHeight(hint.height() + 32);
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
}

void Chat::rosterClicked(const QModelIndex &index)
{
    int type = index.data(ChatRosterModel::TypeRole).toInt();
    const QString jid = index.data(ChatRosterModel::IdRole).toString();

    // create conversation if necessary
    if (type == ChatRosterItem::Contact && !panel(jid))
        addPanel(new ChatDialog(m_client, m_rosterModel, jid));

    // show requested panel
    ChatPanel *chatPanel = panel(jid);
    if (chatPanel)
        QTimer::singleShot(0, chatPanel, SIGNAL(showPanel()));
}

/** Handle user inactivity.
 *
 * @param secs
 */
void Chat::secondsIdle(int secs)
{
    if (secs >= AWAY_TIME)
    {
        if (m_statusCombo->currentIndex() == AvailableIndex)
        {
            m_statusCombo->setCurrentIndex(AwayIndex);
            autoAway = true;
        }
    } else if (autoAway) {
        const int oldIndex = isBusy ? BusyIndex : AvailableIndex;
        m_statusCombo->setCurrentIndex(oldIndex);
    }
}

void Chat::statusChanged(int currentIndex)
{
    autoAway = false;
    if (currentIndex == AvailableIndex)
    {
        isBusy = false;
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            m_client->sendPacket(presence);
        }
        else
            m_client->connectToServer(m_client->getConfiguration());
    } else if (currentIndex == AwayIndex) {
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            presence.setStatus(QXmppPresence::Status::Away);
            m_client->sendPacket(presence);
        }
    } else if (currentIndex == BusyIndex) {
        isBusy = true;
        if (isConnected)
        {
            QXmppPresence presence;
            presence.setType(QXmppPresence::Available);
            presence.setStatus(QXmppPresence::Status::DND);
            m_client->sendPacket(presence);
        }
        else
            m_client->connectToServer(m_client->getConfiguration());
    } else if (currentIndex == OfflineIndex) {
        if (isConnected)
            m_client->disconnect();
    }
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

