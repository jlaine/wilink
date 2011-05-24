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

#include <QCheckBox>
#include <QComboBox>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>

#include "QSoundPlayer.h"

#include "QXmppBookmarkManager.h"
#include "QXmppBookmarkSet.h"
#include "QXmppClient.h"
#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppMucIq.h"
#include "QXmppMucManager.h"
#include "QXmppUtils.h"

#include "application.h"
#include "chat.h"
#include "chat_form.h"
#include "chat_history.h"
#include "chat_plugin.h"
#include "chat_roster.h"
#include "chat_utils.h"

#include "rooms.h"

enum MembersColumns {
    JidColumn = 0,
    AffiliationColumn,
};

ChatRoomWatcher::ChatRoomWatcher(Chat *chatWindow)
    : QObject(chatWindow), chat(chatWindow)
{
    QXmppClient *client = chat->client();

    // add extensions
    QXmppBookmarkManager *bookmarkManager = client->findExtension<QXmppBookmarkManager>();
    if (!bookmarkManager) {
        bookmarkManager = new QXmppBookmarkManager(client);
        client->addExtension(bookmarkManager);
    }

    mucManager = client->findExtension<QXmppMucManager>();
    if (!mucManager) {
        mucManager = new QXmppMucManager;
        client->addExtension(mucManager);
    }

    bool check;
    check = connect(mucManager, SIGNAL(invitationReceived(QString,QString,QString)),
                    this, SLOT(invitationReceived(QString,QString,QString)));
    Q_ASSERT(check);

    // add roster hooks
    check = connect(chat, SIGNAL(urlClick(QUrl)),
                    this, SLOT(urlClick(QUrl)));
    Q_ASSERT(check);
}

void ChatRoomWatcher::invitationHandled(QAbstractButton *button)
{
    QMessageBox *box = qobject_cast<QMessageBox*>(sender());
    if (box && box->standardButton(button) == QMessageBox::Yes)
    {
        const QString roomJid = box->objectName();
        joinRoom(roomJid, true);
        invitations.removeAll(roomJid);
    }
}

/*** Notify the user when an invitation is received.
 */
void ChatRoomWatcher::invitationReceived(const QString &roomJid, const QString &jid, const QString &reason)
{
    // Skip invitations to rooms which we have already joined or
    // which we have already received
    if (chat->panel(roomJid) || invitations.contains(roomJid))
        return;

    const QString contactName = jid;

    QString message = tr("%1 has asked to add you to join the '%2' chat room").arg(contactName, roomJid);
    if(!reason.isNull() && !reason.isEmpty())
        message += ":\n\n" + QString(reason);
    message += "\n\n"+tr("Do you accept?");
    QMessageBox *box = new QMessageBox(QMessageBox::Question,
        tr("Invitation from %1").arg(jid),
        message,
        QMessageBox::Yes | QMessageBox::No,
        chat);
    box->setObjectName(roomJid);
    box->setDefaultButton(QMessageBox::Yes);
    box->setEscapeButton(QMessageBox::No);
    box->open(this, SLOT(invitationHandled(QAbstractButton*)));

    invitations << roomJid;
}

/** Open a XMPP URI if it refers to a chat room.
 */
void ChatRoomWatcher::urlClick(const QUrl &url)
{
    if (url.scheme() == "xmpp" && url.hasQueryItem("join")) {
        QString jid = url.path();
        if (jid.startsWith("/"))
            jid.remove(0, 1);
        joinRoom(jid, true);
    }
}

RoomPanel::RoomPanel(Chat *chatWindow, const QString &jid)
    : ChatPanel(chatWindow),
    chat(chatWindow)
{
    bool check;
    ChatClient *client = chat->client();

    setObjectName(jid);

    // prepare models
    mucRoom = client->findExtension<QXmppMucManager>()->addRoom(jid);

    QSortFilterProxyModel *contactModel = new QSortFilterProxyModel(this);
    contactModel->setSourceModel(chatWindow->rosterModel());
    contactModel->setDynamicSortFilter(true);
    contactModel->setFilterKeyColumn(2);
    contactModel->setFilterRegExp(QRegExp("^(?!offline).+"));

    // header
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // chat history

    QDeclarativeView *declarativeView = new QDeclarativeView;
    QDeclarativeContext *context = declarativeView->rootContext();
    context->setContextProperty("contactModel", contactModel);
    context->setContextProperty("window", chat);

    declarativeView->engine()->addImageProvider("roster", new ChatRosterImageProvider);
    declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    declarativeView->setSource(QUrl("qrc:/RoomPanel.qml"));

    layout->addWidget(declarativeView);
    setFocusProxy(declarativeView);

    // add actions
    optionsAction = addAction(QIcon(":/options.png"), "Options");
    check = connect(optionsAction, SIGNAL(triggered()),
                    mucRoom, SLOT(requestConfiguration()));
    Q_ASSERT(check);

    permissionsAction = addAction(QIcon(":/permissions.png"), "Permissions");
    check = connect(permissionsAction, SIGNAL(triggered()),
                    this, SLOT(changePermissions()));
    Q_ASSERT(check);

    check = connect(mucRoom, SIGNAL(configurationReceived(QXmppDataForm)),
                    this, SLOT(configurationReceived(QXmppDataForm)));
    Q_ASSERT(check);

    // connect signals
    check = connect(declarativeView->rootObject(), SIGNAL(close()),
                    this, SIGNAL(hidePanel()));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(hidePanel()),
                    this, SLOT(deleteLater()));
    Q_ASSERT(check);
}

/** Manage the room's members.
 */
void RoomPanel::changePermissions()
{
    RoomPermissionDialog dialog(mucRoom, "@" + chat->client()->configuration().domain(), chat);
    dialog.exec();
}

/** Display room configuration dialog.
 */
void RoomPanel::configurationReceived(const QXmppDataForm &form)
{
    ChatForm dialog(form, chat);
    if (dialog.exec())
        mucRoom->setConfiguration(dialog.form());
}

