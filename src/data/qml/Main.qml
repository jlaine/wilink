/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

import QtQuick 1.1
import wiLink 2.0

FocusScope {
    id: root

    focus: !appSettings.isMobile

    // A model representing the user's accounts.
    AccountModel {
        id: accountModel

        function clientForJid(jid) {
            var panel = swapper.findPanel('ChatPanel.qml', {accountJid: jid});
            return panel ? panel.client : undefined;
        }

        onModelReset: {
            appPlugins.unload();
            if (accountModel.count) {
                appPlugins.load();
            } else {
                dialogSwapper.showPanel('SetupDialog.qml', {'accountType': 'wifirst'});
            }
        }
    }

    // Logger which records debugging and protocol (XMPP, SIP) information.
    //
    // Its data can be viewed using the debugging plugin.
    QXmppLogger {
        id: appLogger
        loggingType: QXmppLogger.SignalLogging
    }

    // Notifier for displaying desktop notifications.
    Notifier {
        id: appNotifier
    }

    // Manager for loading and unloading plugins.
    //
    // The plugins contain the bulk of the application logic,
    // they control which panels are displayed, etc..
    PluginLoader {
        id: appPlugins
    }

    PreferenceModel {
        id: appPreferences
    }

    ApplicationSettings {
        id: appSettings
    }

    SoundPlayer {
        id: appSoundPlayer

        inputDeviceName: appSettings.audioInputDeviceName
        outputDeviceName: appSettings.audioOutputDeviceName
    }

    Style {
        id: appStyle
        isMobile: appSettings.isMobile
    }

    // The application updater.
    Updater {
        id: appUpdater

        onStateChanged: {
            // when an update is ready to install, prompt user
            if (appUpdater.state == Updater.PromptState) {
                dialogSwapper.showPanel('AboutDialog.qml');
                window.alert();
            }
        }
    }

    // The window background.
    Rectangle {
        anchors.fill: parent
        color: 'white'
    }

    // The left-hand dock.
    Dock {
        id: dock

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 1
    }

    // The main display area.
    PanelSwapper {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: dock.right
        anchors.right: parent.right
        focus: true
    }

    // The overlay for displaying dialogs.
    PanelSwapper {
        id: dialogSwapper

        opacity: 0
        z: 10

        onCurrentItemChanged: {
            if (currentItem) {
                dialogSwapper.forceActiveFocus();
                x = Math.max(0, Math.floor((parent.width - currentItem.width) / 2));
                y = Math.max(0, Math.floor((parent.height - currentItem.height) / 2));
                opacity = 1;
            } else {
                swapper.forceActiveFocus();
                opacity = 0;
            }
        }
    }

    // The mouse area used to cancel menus by clicking outside the menu.
    MouseArea {
        id: menuCancelArea

        anchors.fill: parent
        enabled: false
        z: 11

        onClicked: menuLoader.hide()
    }

    // The overlay for displaying popup menus and combo boxes.
    Loader {
        id: menuLoader

        z: 12

        function hide() {
            menuCancelArea.enabled = false;
            menuLoader.item.opacity = 0;
        }

        function show(x, y) {
            menuCancelArea.enabled = true;
            menuLoader.x = x;
            menuLoader.y = y;
            menuLoader.item.opacity = 1;
        }

        Connections {
            target: menuLoader.item
            onItemClicked: menuLoader.hide()
        }
    }

    // A timer to delay the application startup.
    //
    // This allows the window to be painted before accessing the
    // system wallet, which is blocking on OS X.
    Timer {
        interval: 100
        repeat: false
        running: true

        onTriggered: {
            if (accountModel.count) {
                appPlugins.load();
            } else {
                dialogSwapper.showPanel('SetupDialog.qml', {'accountType': 'wifirst'});
            }
        }
    }

    Component.onCompleted: {
        window.minimumWidth = 360;
        window.minimumHeight = 360;
        window.fullScreen = appSettings.isMobile && appSettings.osType != 'android';
        window.showAndRaise();

        application.showWindows.connect(function() {
            window.showAndRaise();
        });
    }

    Connections {
        target: window

        onShowAbout: {
            dialogSwapper.showPanel('AboutDialog.qml');
        }

        onShowHelp: {
            Qt.openUrlExternally('https://www.wifirst.net/wilink/faq');
        }

        onShowPreferences: {
            dialogSwapper.showPanel('PreferenceDialog.qml');
        }
    }

    Keys.forwardTo: dock
}
