/*
 * wDesktop
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

#ifndef __WDESKTOP_CHAT_SHARES_H__
#define __WDESKTOP_CHAT_SHARES_H__

#include <QDir>
#include <QIcon>
#include <QTreeWidget>

#include "qxmpp/QXmppShareIq.h"

#include "chat_panel.h"

class ChatClient;
class ChatSharesDatabase;
class QLineEdit;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QXmppPacket;
class QXmppTransferFile;

class ChatSharesView : public QTreeWidget
{
public:
    ChatSharesView(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *e);
};

class ChatShares : public ChatPanel
{
    Q_OBJECT

public:
    ChatShares(ChatClient *client, QWidget *parent = 0);
    void setShareServer(const QString &server);

signals:
    void fileExpected(const QString &sid);

private slots:
    void findRemoteFiles();
    void itemDoubleClicked(QTreeWidgetItem *item);
    void registerWithServer();
    void shareGetIqReceived(const QXmppShareGetIq &getIq);
    void shareSearchIqReceived(const QXmppShareSearchIq &share);
    void searchFinished(const QXmppShareSearchIq &share);

private:
    qint64 addCollection(const QXmppShareIq::Collection &collection, QTreeWidgetItem *parent);
    qint64 addFile(const QXmppShareIq::File &file, QTreeWidgetItem *parent);
    void clearView();

private:
    QString shareServer;
    QDir sharesDir;
    QIcon collectionIcon;
    QIcon fileIcon;

    ChatClient *client;
    ChatSharesDatabase *db;
    QLineEdit *lineEdit;
    QTreeWidget *treeWidget;
    QTimer *registerTimer;
};

#endif
