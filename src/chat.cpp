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
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAuthenticator>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QPluginLoader>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QToolBar>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"

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
#include "chat_status.h"
#include "chat_utils.h"
#include "systeminfo.h"
#include "updatesdialog.h"

class ChatPrivate
{
public:
    QToolBar *actions;
    QMenu *fileMenu;
    QAction *findAction;
    QAction *findAgainAction;

    ChatClient *client;
    QList<ChatPanel*> chatPanels;
    ChatRosterModel *rosterModel;
    ChatRosterView *rosterView;
    QString windowTitle;

    QWidget *leftPanel;
    QStackedWidget *conversationPanel;

    QList<ChatPlugin*> plugins;
};

Chat::Chat(QWidget *parent)
    : QMainWindow(parent),
    d(new ChatPrivate)
{
    bool check;

    /* get handle to application */
    check = connect(wApp, SIGNAL(messageClicked(QWidget*)),
                    this, SLOT(messageClicked(QWidget*)));
    Q_ASSERT(check);

    d->client = new ChatClient(this);
    d->rosterModel =  new ChatRosterModel(d->client, this);
    connect(d->rosterModel, SIGNAL(rosterReady()), this, SLOT(resizeContacts()));
    connect(d->rosterModel, SIGNAL(pendingMessages(int)), this, SLOT(pendingMessages(int)));

    /* set up logger */
    QXmppLogger *logger = new QXmppLogger(this);
    logger->setLoggingType(QXmppLogger::SignalLogging);
    d->client->setLogger(logger);

    /* build splitter */
    QSplitter *splitter = new QSplitter;
    splitter->setChildrenCollapsible(false);

    /* left panel */
    d->leftPanel = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->setMargin(0);
    leftLayout->setSpacing(0);

    d->actions = new QToolBar;
    d->actions->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    d->actions->hide();
    leftLayout->addWidget(d->actions);

    d->rosterView = new ChatRosterView(d->rosterModel);
    d->rosterView->setShowOfflineContacts(wApp->showOfflineContacts());
    connect(wApp, SIGNAL(showOfflineContactsChanged(bool)), d->rosterView, SLOT(setShowOfflineContacts(bool)));
    connect(d->rosterView, SIGNAL(clicked(QModelIndex)), this, SLOT(rosterClicked(QModelIndex)));
    connect(d->rosterView, SIGNAL(itemMenu(QMenu*, QModelIndex)), this, SIGNAL(rosterMenu(QMenu*, QModelIndex)));
    connect(d->rosterView, SIGNAL(itemDrop(QDropEvent*, QModelIndex)), this, SIGNAL(rosterDrop(QDropEvent*, QModelIndex)));
    leftLayout->addWidget(d->rosterView);

    d->leftPanel->setLayout(leftLayout);
    splitter->addWidget(d->leftPanel);
    splitter->setStretchFactor(0, 0);

    /* right panel */
    d->conversationPanel = new QStackedWidget;
    d->conversationPanel->hide();
    connect(d->conversationPanel, SIGNAL(currentChanged(int)), this, SLOT(panelChanged(int)));
    splitter->addWidget(d->conversationPanel);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    /* build status bar */
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(new ChatStatus(d->client));

   /* create menu */
    d->fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *action = d->fileMenu->addAction(QIcon(":/options.png"), tr("&Preferences"));
    action->setMenuRole(QAction::PreferencesRole);
    connect(action, SIGNAL(triggered()), this, SLOT(showPreferences()));

    action = d->fileMenu->addAction(QIcon(":/chat.png"), tr("Chat accounts"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(showAccounts()));

    if (wApp->updatesDialog())
    {
        action = d->fileMenu->addAction(QIcon(":/refresh.png"), tr("Check for &updates"));
        connect(action, SIGNAL(triggered(bool)), wApp->updatesDialog(), SLOT(check()));
    }

    action = d->fileMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    action->setMenuRole(QAction::QuitRole);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Q));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

    /* "Edit" menu */
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    d->findAction = editMenu->addAction(QIcon(":/search.png"), tr("&Find"));
    d->findAction->setEnabled(false);
    d->findAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_F));

    d->findAgainAction = editMenu->addAction(tr("Find a&gain"));
    d->findAgainAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_G));
    d->findAgainAction->setEnabled(false);

    /* set up client */
    connect(d->client, SIGNAL(error(QXmppClient::Error)), this, SLOT(error(QXmppClient::Error)));

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
    foreach (ChatPanel *panel, d->chatPanels)
        if (!panel->parent())
            delete panel;

    // disconnect
    d->client->disconnectFromServer();

    // unload plugins
    for (int i = d->plugins.size() - 1; i >= 0; i--)
        d->plugins[i]->finalize(this);

    delete d;
}

/** Adds an action to the tool bar.
 *
 * @param icon
 * @param text
 */
QAction *Chat::addAction(const QIcon &icon, const QString &text)
{
    QAction *action = d->actions->addAction(icon, text);
    d->actions->show();
    return action;
}

/** Connect signals for the given panel.
 *
 * @param panel
 */
void Chat::addPanel(ChatPanel *panel)
{
    if (d->chatPanels.contains(panel))
        return;
    connect(panel, SIGNAL(attachPanel()), this, SLOT(attachPanel()));
    connect(panel, SIGNAL(destroyed(QObject*)), this, SLOT(destroyPanel(QObject*)));
    connect(panel, SIGNAL(dropPanel(QDropEvent*)), this, SLOT(dropPanel(QDropEvent*)));
    connect(panel, SIGNAL(hidePanel()), this, SLOT(hidePanel()));
    connect(panel, SIGNAL(notifyPanel(QString, int)), this, SLOT(notifyPanel(QString, int)));
    connect(panel, SIGNAL(showPanel()), this, SLOT(showPanel()));

    // allows plugins to polish panel
    foreach (ChatPlugin *plugin, d->plugins)
        plugin->polish(this, panel);

    d->chatPanels << panel;
}

void Chat::attachPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(sender());
    if (!panel)
        return;

    // add panel
    if (d->conversationPanel->indexOf(panel) < 0)
    {
#ifdef WILINK_EMBEDDED
        d->leftPanel->hide();
#endif
        d->conversationPanel->addWidget(panel);
        d->conversationPanel->show();
        if (d->conversationPanel->count() == 1)
            resizeContacts();
    }
    d->conversationPanel->setCurrentWidget(panel);
}

/** When a panel is destroyed, from it from our list of panels.
 *
 * @param obj
 */
void Chat::destroyPanel(QObject *obj)
{
    d->chatPanels.removeAll(static_cast<ChatPanel*>(obj));
}

/** Detach the current panel as a window.
 */
void Chat::detachPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(d->conversationPanel->currentWidget());
    if (!panel)
        return;

    QPoint oldPos = d->conversationPanel->mapToGlobal(panel->pos());
    oldPos.setY(oldPos.y() - 20);
    if (d->conversationPanel->count() == 1)
    {
        d->conversationPanel->hide();
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    d->conversationPanel->removeWidget(panel);
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
    QModelIndex index = d->rosterModel->findItem(panel->objectName());
    if (index.isValid())
        emit rosterDrop(event, index);
}

/** Hide a panel.
 */
void Chat::hidePanel()
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    if (d->conversationPanel->indexOf(panel) < 0)
    {
        panel->hide();
        return;
    }

    // close view
    if (d->conversationPanel->count() == 1)
    {
        d->conversationPanel->hide();
#ifdef WILINK_EMBEDDED
        d->leftPanel->show();
#endif
        QTimer::singleShot(100, this, SLOT(resizeContacts()));
    }
    d->conversationPanel->removeWidget(panel);
}

/** Notify the user of activity on a panel.
 */
void Chat::notifyPanel(const QString &message, int options)
{
    QWidget *panel = qobject_cast<QWidget*>(sender());
    QWidget *window = panel->isVisible() ? panel->window() : this;

    // add pending message
    bool showMessage = (options & ChatPanel::ForceNotification);
    if (!window->isActiveWindow() || (window == this && d->conversationPanel->currentWidget() != panel))
    {
        d->rosterModel->addPendingMessage(panel->objectName());
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

/** Show a panel.
 */
void Chat::showPanel()
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(sender());
    if (!panel)
        return;

    // if the panel is detached, stop here
    if (panel->isVisible() && !panel->parent())
    {
        panel->raise();
        panel->activateWindow();
        panel->setFocus();
        return;
    }

    // add panel
    if (d->conversationPanel->indexOf(panel) < 0)
    {
        d->conversationPanel->addWidget(panel);
#ifdef WILINK_EMBEDDED
        d->leftPanel->hide();
#endif
        d->conversationPanel->show();
        if (d->conversationPanel->count() == 1)
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
    d->conversationPanel->setCurrentWidget(panel);
}

/** Handle switching between panels.
 *
 * @param index
 */
void Chat::panelChanged(int index)
{
    disconnect(d->findAction, SIGNAL(triggered(bool)));
    disconnect(d->findAgainAction, SIGNAL(triggered(bool)));

    QWidget *widget = d->conversationPanel->widget(index);
    if (!widget)
    {
        d->findAction->setEnabled(false);
        d->findAgainAction->setEnabled(false);
        return;
    }

    // connect find actions
    connect(d->findAction, SIGNAL(triggered(bool)), widget, SIGNAL(findPanel()));
    connect(d->findAgainAction, SIGNAL(triggered(bool)), widget, SIGNAL(findAgainPanel()));
    d->findAction->setEnabled(true);
    d->findAgainAction->setEnabled(true);

    d->rosterModel->clearPendingMessages(widget->objectName());

    // select the corresponding roster entry
    QModelIndex newIndex = d->rosterView->mapFromRoster(
        d->rosterModel->findItem(widget->objectName()));
    QModelIndex currentIndex = d->rosterView->currentIndex();
    while (currentIndex.isValid() && currentIndex != newIndex)
        currentIndex = currentIndex.parent();
    if (currentIndex != newIndex)
        d->rosterView->setCurrentIndex(newIndex);

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
        int index = d->conversationPanel->currentIndex();
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
void Chat::pendingMessages(int messages)
{
    QString title = d->windowTitle;
    if (messages)
        title += " - " + tr("%n message(s)", "", messages);
    QWidget::setWindowTitle(title);
}

/** Prompt for credentials then connect.
 */
void Chat::promptCredentials()
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

/** Return this window's chat client.
 */
ChatClient *Chat::client()
{
    return d->client;
}

/** Return this window's chat roster model.
 */
ChatRosterModel *Chat::rosterModel()
{
    return d->rosterModel;
}

/** Return this window's chat roster view.
 */
ChatRosterView *Chat::rosterView()
{
    return d->rosterView;
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
        config.setIgnoreSslErrors(false);
    }

    /* set keep alive */
    config.setKeepAliveTimeout(15);

    /* connect to server */
    d->client->connectToServer(config);

    /* load plugins */
    QObjectList plugins = QPluginLoader::staticInstances();
    foreach (QObject *object, plugins)
    {
        ChatPlugin *plugin = qobject_cast<ChatPlugin*>(object);
        if (plugin)
        {
            plugin->initialize(this);
            d->plugins << plugin;
        }
    }

    /* Create "Help" menu here, so that it remains last */
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *action = helpMenu->addAction(tr("%1 FAQ").arg(qApp->applicationName()));
#ifdef Q_OS_MAC
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Question));
#else
    action->setShortcut(QKeySequence(Qt::Key_F1));
#endif
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showHelp()));

    action = helpMenu->addAction(tr("About %1").arg(qApp->applicationName()));
    action->setMenuRole(QAction::AboutRole);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

    return true;
}

/** When asked to open an XMPP URI, let plugins handle it.
 */
void Chat::openUrl(const QUrl &url)
{
    emit urlClick(url);
}

/** Return this window's "File" menu.
 */
QMenu *Chat::fileMenu()
{
    return d->fileMenu;
}

/** Handle a click on a system tray message.
 */
void Chat::messageClicked(QWidget *context)
{
    ChatPanel *panel = qobject_cast<ChatPanel*>(context);
    if (panel && d->chatPanels.contains(panel))
        QTimer::singleShot(0, panel, SIGNAL(showPanel()));
}

/** Find a panel by its object name.
 *
 * @param objectName
 */
ChatPanel *Chat::panel(const QString &objectName)
{
    foreach (ChatPanel *panel, d->chatPanels)
        if (panel->objectName() == objectName)
            return panel;
    return 0;
}

/** Try to resize the window to fit the contents of the contacts list.
 */
void Chat::resizeContacts()
{
#ifndef WILINK_EMBEDDED
    QSize hint = d->rosterView->sizeHint();
    hint.setHeight(hint.height() + d->rosterView->sizeHintForRow(0) + 4);
    QSize barHint = statusBar()->sizeHint();
    hint.setHeight(hint.height() + barHint.height());
    if (barHint.width() > hint.width())
        hint.setWidth(barHint.width());
    if (d->conversationPanel->isVisible())
        hint.setWidth(hint.width() + 500);

    // respect current size
    if (d->conversationPanel->isVisible() && hint.width() < size().width())
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
    d->windowTitle = title;
    QWidget::setWindowTitle(title);
}

static QLayout *aboutBox()
{
    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *icon = new QLabel;
    icon->setPixmap(QPixmap(":/wiLink-64.png"));
    hbox->addWidget(icon);
    hbox->addSpacing(20);
    hbox->addWidget(new QLabel(QString("<p style=\"font-size: xx-large;\">%1</p>"
        "<p style=\"font-size: large;\">%2</p>")
        .arg(qApp->applicationName(),
            QString("version %1").arg(qApp->applicationVersion()))));
    return hbox;
}

/** Display an "about" dialog.
 */
void Chat::showAbout()
{
    QDialog dlg;
    dlg.setWindowTitle(tr("About %1").arg(qApp->applicationName()));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(aboutBox());
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

/** Display the preferenes dialog.
 */
void Chat::showPreferences(const QString &focusTab)
{
    ChatPreferences *dialog = new ChatPreferences(this);
    dialog->setWindowModality(Qt::WindowModal);

    connect(dialog, SIGNAL(finished(int)),
            dialog, SLOT(deleteLater()));

    dialog->addTab(new ChatOptions);
    dialog->addTab(new SoundOptions);
    foreach (ChatPlugin *plugin, d->plugins)
        plugin->preferences(dialog);

    dialog->setCurrentTab(focusTab);
#ifdef WILINK_EMBEDDED
    dialog->showMaximized();
#else
    dialog->show();
#endif
}

ChatOptions::ChatOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;

    // GENERAL OPTIONS
    QGroupBox *group = new QGroupBox(tr("General options"));
    QVBoxLayout *vbox = new QVBoxLayout;
    group->setLayout(vbox);
    layout->addWidget(group);

    if (wApp->isInstalled())
    {
        openAtLogin = new QCheckBox(tr("Open at login"));
        openAtLogin->setChecked(wApp->openAtLogin());
        vbox->addWidget(openAtLogin);
    } else {
        openAtLogin = 0;
    }

    showOfflineContacts = new QCheckBox(tr("Show offline contacts"));
    showOfflineContacts->setChecked(wApp->showOfflineContacts());
    vbox->addWidget(showOfflineContacts);

    // ABOUT
    group = new QGroupBox(tr("About %1").arg(wApp->applicationName()));
    group->setLayout(aboutBox());
    layout->addWidget(group);

    setLayout(layout);
    setWindowIcon(QIcon(":/options.png"));
    setWindowTitle(tr("General"));
}

bool ChatOptions::save()
{
    if (openAtLogin)
        wApp->setOpenAtLogin(openAtLogin->isChecked());
    wApp->setShowOfflineContacts(showOfflineContacts->isChecked());
    return true;
}

PluginOptions::PluginOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;

    QGroupBox *group = new QGroupBox(tr("Plugins"));
    QVBoxLayout *box = new QVBoxLayout;
    group->setLayout(box);
    QObjectList plugins = QPluginLoader::staticInstances();
    foreach (QObject *object, plugins)
    {
        ChatPlugin *plugin = qobject_cast<ChatPlugin*>(object);
        if (plugin) {
            box->addWidget(new QLabel(plugin->name()));
        }
    }
    layout->addWidget(group);

    setLayout(layout);
    setWindowIcon(QIcon(":/plugin.png"));
    setWindowTitle(tr("Plugins"));
}

#define SOUND_TEST_SECONDS 5

SoundOptions::SoundOptions()
{
    QVBoxLayout *layout = new QVBoxLayout;

    // DEVICES
    QGroupBox *group = new QGroupBox(tr("Sound devices"));
    QGridLayout *devicesLayout = new QGridLayout;
    devicesLayout->setColumnStretch(2, 1);
    group->setLayout(devicesLayout);
    layout->addWidget(group);

    // output
    QLabel *label = new QLabel;
    label->setPixmap(QPixmap(":/audio-output.png"));
    devicesLayout->addWidget(label, 0, 0);
    devicesLayout->addWidget(new QLabel(tr("Audio playback device")), 0, 1);
    outputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    outputCombo = new QComboBox;
    foreach (const QAudioDeviceInfo &info, outputDevices) {
        outputCombo->addItem(info.deviceName());
        if (info.deviceName() == wApp->audioOutputDevice().deviceName())
            outputCombo->setCurrentIndex(outputCombo->count() - 1);
    }
    devicesLayout->addWidget(outputCombo, 0, 2);

    // input
    label = new QLabel;
    label->setPixmap(QPixmap(":/audio-input.png"));
    devicesLayout->addWidget(label, 1, 0);
    devicesLayout->addWidget(new QLabel(tr("Audio capture device")), 1, 1);
    inputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    inputCombo = new QComboBox;
    foreach (const QAudioDeviceInfo &info, inputDevices) {
        inputCombo->addItem(info.deviceName());
        if (info.deviceName() == wApp->audioInputDevice().deviceName())
            inputCombo->setCurrentIndex(inputCombo->count() - 1);
    }
    devicesLayout->addWidget(inputCombo, 1, 2);

    // test
    testBuffer = new QBuffer(this);
    testBar = new QSoundMeterBar;
    testBar->hide();
    devicesLayout->addWidget(testBar, 2, 0, 1, 3);
    testLabel = new QLabel;
    testLabel->setWordWrap(true);
    devicesLayout->addWidget(testLabel, 3, 0, 1, 2);
    testButton = new QPushButton(tr("Test"));
    connect(testButton, SIGNAL(clicked()), this, SLOT(startInput()));
    devicesLayout->addWidget(testButton, 3, 2);

    // NOTIFICATIONS
    group = new QGroupBox(tr("Sound notifications"));
    QVBoxLayout *notificationsLayout = new QVBoxLayout;
    group->setLayout(notificationsLayout);
    layout->addWidget(group);

    incomingMessageSound = new QCheckBox(tr("Incoming message"));
    incomingMessageSound->setChecked(!wApp->incomingMessageSound().isEmpty());
    notificationsLayout->addWidget(incomingMessageSound);

    outgoingMessageSound = new QCheckBox(tr("Outgoing message"));
    outgoingMessageSound->setChecked(!wApp->outgoingMessageSound().isEmpty());
    notificationsLayout->addWidget(outgoingMessageSound);

    setLayout(layout);
    setWindowIcon(QIcon(":/audio-output.png"));
    setWindowTitle(tr("Sound"));
}

bool SoundOptions::save()
{
    // devices
    if (!inputDevices.isEmpty())
        wApp->setAudioInputDevice(inputDevices[inputCombo->currentIndex()]);
    if (!outputDevices.isEmpty())
        wApp->setAudioOutputDevice(outputDevices[outputCombo->currentIndex()]);

    // notifications
    wApp->setIncomingMessageSound(
        incomingMessageSound->isChecked() ? QLatin1String(":/message-incoming.ogg") : QString());
    wApp->setOutgoingMessageSound(
        outgoingMessageSound->isChecked() ? QLatin1String(":/message-outgoing.ogg") : QString());
    return true;
}

void SoundOptions::startInput()
{
    if (inputDevices.isEmpty() || outputDevices.isEmpty())
        return;
    testButton->setEnabled(false);

    QAudioFormat format;
    format.setFrequency(8000);
    format.setChannels(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // create audio input and output
#ifdef Q_OS_MAC
    // 128ms at 8kHz
    const int bufferSize = 2048 * format.channels();
#else
    // 160ms at 8kHz
    const int bufferSize = 2560 * format.channels();
#endif
    testInput = new QAudioInput(inputDevices[inputCombo->currentIndex()], format, this);
    testInput->setBufferSize(bufferSize);
    testOutput = new QAudioOutput(outputDevices[outputCombo->currentIndex()], format, this);
    testOutput->setBufferSize(bufferSize);

    // start input
    testBar->show();
    testLabel->setText(tr("Speak into the microphone for %1 seconds and check the sound level.").arg(QString::number(SOUND_TEST_SECONDS)));
    testBuffer->open(QIODevice::WriteOnly);
    testBuffer->reset();
    QSoundMeter *inputMeter = new QSoundMeter(testInput->format(), testBuffer, testInput);
    connect(inputMeter, SIGNAL(valueChanged(int)), testBar, SLOT(setValue(int)));
    testInput->start(inputMeter);
    QTimer::singleShot(SOUND_TEST_SECONDS * 1000, this, SLOT(startOutput()));
}

void SoundOptions::startOutput()
{
    testInput->stop();
    testBar->setValue(0);

    // start output
    testLabel->setText(tr("You should now hear the sound you recorded."));
    testBuffer->open(QIODevice::ReadOnly);
    testBuffer->reset();
    QSoundMeter *outputMeter = new QSoundMeter(testOutput->format(), testBuffer, testOutput);
    connect(outputMeter, SIGNAL(valueChanged(int)), testBar, SLOT(setValue(int)));
    testOutput->start(outputMeter);
    QTimer::singleShot(SOUND_TEST_SECONDS * 1000, this, SLOT(stopOutput()));
}

void SoundOptions::stopOutput()
{
    testOutput->stop();
    testBar->setValue(0);
    testBar->hide();

    // cleanup
    delete testInput;
    delete testOutput;
    testLabel->setText(QString());
    testButton->setEnabled(true);
}

