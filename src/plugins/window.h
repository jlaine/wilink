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

#ifndef __WILINK_CHAT_H__
#define __WILINK_CHAT_H__

#include <QMainWindow>

#include "client.h"

class Application;
class ChatPrivate;
class ChatRosterModel;
class ChatRosterView;
class QFileDialog;
class QMessageBox;

/** Chat represents the user interface's main window.
 */
class Chat : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(ChatClient* client READ client CONSTANT)
    Q_PROPERTY(ChatRosterModel* rosterModel READ rosterModel CONSTANT)
    Q_PROPERTY(bool isActiveWindow READ isActiveWindow NOTIFY isActiveWindowChanged)

public:
    Chat(QWidget *parent = 0);
    ~Chat();

    ChatClient *client();
    ChatRosterModel *rosterModel();

    bool open(const QString &jid);
    void setWindowTitle(const QString &title);

signals:
    void isActiveWindowChanged();

public slots:
    void alert();
    QFileDialog *fileDialog();
    QMessageBox *messageBox();
    void showPreferences(const QString &focusTab = QString());

private slots:
    void error(QXmppClient::Error error);
    void pendingMessages(int messages);
    void promptCredentials();
    void showAbout();
    void showHelp();

protected:
    void changeEvent(QEvent *event);

private:
    ChatPrivate * const d;
};

#endif
