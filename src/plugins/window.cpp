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
#include <QAuthenticator>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeNetworkAccessManagerFactory>
#include <QDeclarativeView>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStringList>
#include <QTimer>

#include "QSoundPlayer.h"

#include "QXmppCallManager.h"
#include "QXmppConfiguration.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppLogger.h"
#include "QXmppMessage.h"
#include "QXmppMucManager.h"
#include "QXmppRosterIq.h"
#include "QXmppRosterManager.h"
#include "QXmppRtpChannel.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "accounts.h"
#include "application.h"
#include "utils.h"
#include "idle/idle.h"
#include "calls.h"
#include "console.h"
#include "conversations.h"
#include "declarative.h"
#include "diagnostics.h"
#include "discovery.h"
#include "history.h"
#include "phone.h"
#include "phone/sip.h"
#include "photos.h"
#include "player.h"
#include "preferences.h"
#include "rooms.h"
#include "roster.h"
#include "shares.h"
#include "shares/options.h"
#include "updatesdialog.h"
#include "window.h"

class NetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    QNetworkAccessManager *create(QObject * parent)
    {
        return new NetworkAccessManager(parent);
    }
};

class ChatPrivate
{
public:
    QAction *findAction;
    QAction *findAgainAction;

    ChatClient *client;
    ChatRosterModel *rosterModel;
    QDeclarativeView *rosterView;
    QString windowTitle;
};

Chat::Chat(QWidget *parent)
    : QMainWindow(parent),
    d(new ChatPrivate)
{
    bool check;

    qmlRegisterUncreatableType<QXmppClient>("QXmpp", 0, 4, "QXmppClient", "");
    qmlRegisterUncreatableType<QXmppCall>("QXmpp", 0, 4, "QXmppCall", "");
    qmlRegisterUncreatableType<QXmppCallManager>("QXmpp", 0, 4, "QXmppCallManager", "");
    qmlRegisterUncreatableType<DiagnosticManager>("QXmpp", 0, 4, "DiagnosticManager", "");
    qmlRegisterUncreatableType<QXmppDiscoveryManager>("QXmpp", 0, 4, "QXmppDiscoveryManager", "");
    qmlRegisterUncreatableType<QXmppLogger>("QXmpp", 0, 4, "QXmppLogger", "");
    qmlRegisterType<QXmppDeclarativeMessage>("QXmpp", 0, 4, "QXmppMessage");
    qmlRegisterUncreatableType<QXmppMucManager>("QXmpp", 0, 4, "QXmppMucManager", "");
    qmlRegisterUncreatableType<QXmppMucRoom>("QXmpp", 0, 4, "QXmppMucRoom", "");
    qmlRegisterType<QXmppDeclarativePresence>("QXmpp", 0, 4, "QXmppPresence");
    qmlRegisterUncreatableType<QXmppRosterManager>("QXmpp", 0, 4, "QXmppRosterManager", "");
    qmlRegisterUncreatableType<QXmppRtpAudioChannel>("QXmpp", 0, 4, "QXmppRtpAudioChannel", "");
    qmlRegisterUncreatableType<QXmppTransferJob>("QXmpp", 0, 4, "QXmppTransferJob", "");
    qmlRegisterUncreatableType<QXmppTransferManager>("QXmpp", 0, 4, "QXmppTransferManager", "");
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");

    qmlRegisterType<CallAudioHelper>("wiLink", 1, 2, "CallAudioHelper");
    qmlRegisterType<CallVideoHelper>("wiLink", 1, 2, "CallVideoHelper");
    qmlRegisterType<CallVideoItem>("wiLink", 1, 2, "CallVideoItem");
    qmlRegisterUncreatableType<DeclarativePen>("wiLink", 1, 2, "Pen", "");
    qmlRegisterUncreatableType<ChatClient>("wiLink", 1, 2, "Client", "");
    qmlRegisterType<Conversation>("wiLink", 1, 2, "Conversation");
    qmlRegisterType<DiscoveryModel>("wiLink", 1, 2, "DiscoveryModel");
    qmlRegisterUncreatableType<ChatHistoryModel>("wiLink", 1, 2, "HistoryModel", "");
    qmlRegisterType<Idle>("wiLink", 1, 2, "Idle");
    qmlRegisterType<ListHelper>("wiLink", 1, 2, "ListHelper");
    qmlRegisterType<LogModel>("wiLink", 1, 2, "LogModel");
    qmlRegisterType<PhoneCallsModel>("wiLink", 1, 2, "PhoneCallsModel");
    qmlRegisterUncreatableType<SipClient>("wiLink", 1, 2, "SipClient", "");
    qmlRegisterUncreatableType<SipCall>("wiLink", 1, 2, "SipCall", "");
    qmlRegisterUncreatableType<PhotoUploadModel>("wiLink", 1, 2, "PhotoUploadModel", "");
    qmlRegisterType<PhotoModel>("wiLink", 1, 2, "PhotoModel");
    qmlRegisterType<PlayerModel>("wiLink", 1, 2, "PlayerModel");
    qmlRegisterType<RoomModel>("wiLink", 1, 2, "RoomModel");
    qmlRegisterType<RoomListModel>("wiLink", 1, 2, "RoomListModel");
    qmlRegisterUncreatableType<ChatRosterModel>("wiLink", 1, 2, "RosterModel", "");
    qmlRegisterType<ShareModel>("wiLink", 1, 2, "ShareModel");
    qmlRegisterType<ShareQueueModel>("wiLink", 1, 2, "ShareQueueModel");
    qmlRegisterUncreatableType<QSoundPlayer>("wiLink", 1, 2, "SoundPlayer", "");
    qmlRegisterType<VCard>("wiLink", 1, 2, "VCard");
    qmlRegisterUncreatableType<Chat>("wiLink", 1, 2, "Window", "");

    // crutches for Qt..
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qmlRegisterUncreatableType<QAbstractItemModel>("wiLink", 1, 2, "QAbstractItemModel", "");
    qmlRegisterUncreatableType<QFileDialog>("wiLink", 1, 2, "QFileDialog", "");
    qmlRegisterUncreatableType<QMessageBox>("wiLink", 1, 2, "QMessageBox", "");
    qmlRegisterType<QDeclarativeSortFilterProxyModel>("wiLink", 1, 2, "SortFilterProxyModel");

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
    connect(action, SIGNAL(triggered()), this, SLOT(showPreferences()));

    action = fileMenu->addAction(QIcon(":/chat.png"), tr("Chat accounts"));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(showAccounts()));

    if (wApp->updatesDialog())
    {
        action = fileMenu->addAction(QIcon(":/refresh.png"), tr("Check for &updates"));
        connect(action, SIGNAL(triggered(bool)), wApp->updatesDialog(), SLOT(check()));
    }

    action = fileMenu->addAction(QIcon(":/close.png"), tr("&Quit"));
    action->setMenuRole(QAction::QuitRole);
    action->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_Q));
    connect(action, SIGNAL(triggered(bool)), qApp, SLOT(quit()));

#if 0
    /* "Edit" menu */
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    d->findAction = editMenu->addAction(QIcon(":/search.png"), tr("&Find"));
    d->findAction->setEnabled(false);
    d->findAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_F));

    d->findAgainAction = editMenu->addAction(tr("Find a&gain"));
    d->findAgainAction->setShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_G));
    d->findAgainAction->setEnabled(false);
#endif

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

Chat::~Chat()
{
    // disconnect
    d->client->disconnectFromServer();
    delete d;
}

void Chat::alert()
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

void Chat::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange)
        emit isActiveWindowChanged();
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

QFileDialog *Chat::fileDialog()
{
    QFileDialog *dialog = new QDeclarativeFileDialog(this);
    return dialog;
}

QMessageBox *Chat::messageBox()
{
    QMessageBox *box = new QMessageBox(this);
    box->setIcon(QMessageBox::Question);
    return box;
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

    /* load QML */
    d->rosterView->setSource(QUrl("qrc:/main.qml"));

    return true;
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
    dialog->addTab(new ShareOptions(ShareModel::database()));

    dialog->setCurrentTab(focusTab);
#ifdef WILINK_EMBEDDED
    dialog->showMaximized();
#else
    dialog->show();
#endif
}


