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

#ifndef __WILINK_CHAT_H__
#define __WILINK_CHAT_H__

#include <QWidget>
#include <QMainWindow>

#include "qxmpp/QXmppClient.h"
#include "qxmpp/QXmppRoster.h"

class ChatClient;
class ChatPanel;
class ChatRosterModel;
class ChatRosterView;
class Idle;
class QComboBox;
class QLabel;
class QModelIndex;
class QPushButton;
class QSplitter;
class QStackedWidget;

/** Chat represents the user interface's main window.
 */
class Chat : public QMainWindow
{
    Q_OBJECT

public:
    Chat(QWidget *parent = 0);
    ~Chat();

    ChatClient *client();
    ChatRosterModel *rosterModel();
    QMenu *optionsMenu();
    bool open(const QString &jid, const QString &password, bool ignoreSslErrors);
    void addPanel(ChatPanel *panel);
    ChatPanel *panel(const QString &objectName);

signals:
    /** Plugins should connect to this signal to handle click events
     *  on a roster entry.
     */
    void rosterClick(const QModelIndex &index);

    /** Plugins should connect to this signal to handle drop events
     *  on a roster entry.
     */
    void rosterDrop(QDropEvent *event, const QModelIndex &index);

    /** Plugins should connect to this signal to add entries
     *  to the context menu for a roster entry.
     */
    void rosterMenu(QMenu *menu, const QModelIndex &index);

private slots:
    void connected();
    void disconnected();
    void error(QXmppClient::Error error);
    void panelChanged(int index);
    void promptCredentials();
    void resizeContacts();
    void rosterClicked(const QModelIndex &index);
    void secondsIdle(int);
    void statusChanged(int currentIndex);

    void detachPanel();
    void destroyPanel(QObject *obj);
    void dropPanel(QDropEvent *event);
    void hidePanel();
    void notifyPanel();
    void registerPanel();
    void showPanel();
    void unregisterPanel();

private:
    void changeEvent(QEvent *event);

private:
    bool autoAway;
    bool isBusy;
    bool isConnected;

    Idle *m_idle;

    ChatClient *m_client;
    QList<ChatPanel*> m_chatPanels;
    QMenu *m_optionsMenu;
    ChatRosterModel *m_rosterModel;
    ChatRosterView *m_rosterView;

    QComboBox *m_statusCombo;
    QStackedWidget *m_conversationPanel;
};

#endif
