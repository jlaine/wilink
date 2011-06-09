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

#include <QAudioDeviceInfo>
#include <QMainWindow>

#include "chat_client.h"
#include "chat_preferences.h"

class Application;
class ChatPrivate;
class ChatRosterModel;
class ChatRosterView;
class QAudioInput;
class QAudioOutput;
class QBuffer;
class QCheckBox;
class QComboBox;
class QFileDialog;
class QLabel;
class QMessageBox;
class QModelIndex;
class QSoundMeterBar;

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

    /** Plugins should connect to this signal to handle XMPP URIs.
     */
    void urlClick(const QUrl &url);

public slots:
    void alert();
    void openUrl(const QUrl &url);
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

class ChatOptions : public ChatPreferencesTab
{
    Q_OBJECT

public:
    ChatOptions();
    bool save();

private:
    QCheckBox *openAtLogin;
    QCheckBox *showOfflineContacts;
};

class SoundOptions : public ChatPreferencesTab
{
    Q_OBJECT

public:
    SoundOptions();
    bool save();

private slots:
    void startInput();
    void startOutput();
    void stopOutput();

private:
    // devices
    QComboBox *inputCombo;
    QList<QAudioDeviceInfo> inputDevices;
    QComboBox *outputCombo;
    QList<QAudioDeviceInfo> outputDevices;

    // notifications
    QCheckBox *incomingMessageSound;
    QCheckBox *outgoingMessageSound;

    // test
    QSoundMeterBar *testBar;
    QBuffer *testBuffer;
    QPushButton *testButton;
    QLabel *testLabel;
    QAudioInput *testInput;
    QAudioOutput *testOutput;
};

#endif
