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

    /** A model representing the user's accounts.
     */
    AccountModel {
        id: accountModel

        /** Reload plugins after accounts were modified.
         */
        onModelReset: {
            appPlugins.unload();
            if (accountModel.count) {
                appPlugins.load();
            } else {
                dialogSwapper.showPanel('SetupDialog.qml');
            }
        }

        /** Load plugins at startup.
         */
        Component.onCompleted: {
            if (accountModel.count) {
                appPlugins.load();
            } else {
                dialogSwapper.showPanel('SetupDialog.qml');
            }
        }
    }

    /** Logger which records debugging and protocol (XMPP, SIP) information.
     *
     * Its data can be viewed using the debugging plugin.
     */
    QXmppLogger {
        id: appLogger
        loggingType: QXmppLogger.SignalLogging
    }

    /** Notifier for displaying desktop notifications.
     *
     * It is used to display incoming chat messages.
     */
    Notifier {
        id: appNotifier
    }

    /** Manager for loading and unloading plugins.
     *
     * The plugins contain the bulk of the application logic,
     * they control which panels are displayed, etc..
     */
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

    /** The application updater.
     *
     * It periodically checks for application updates, and if an update
     * is available, the user is prompted to install it.
     */
    Updater {
        id: appUpdater

        onStateChanged: {
            // when an update is ready to install, prompt user
            if (appUpdater.state == Updater.PromptState) {
                dialogSwapper.showPanel('AboutDialog.qml');
                appNotifier.alert();
            }
        }
    }

    /** The window background.
     */
    Rectangle {
        anchors.fill: parent
        color: 'white'
    }

    /** The left-hand dock.
     */
    Dock {
        id: dock

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 1
    }

    /** The main display area.
     */
    PanelSwapper {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: dock.right
        anchors.right: parent.right
        focus: true
    }

    Rectangle {
        id: dialogOverlay

        anchors.fill: parent
        color: '#44000000'
        opacity: 0
        z: 9

        // This MouseArea prevents clicks on items behind dialog
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
        }
    }

    /** The overlay for displaying dialogs.
     */
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
                dialogOverlay.opacity = 1;
            } else {
                swapper.forceActiveFocus();
                opacity = 0;
                dialogOverlay.opacity = 0;
            }
        }
    }

    /** The mouse area used to cancel menus by clicking outside the menu.
     */
    MouseArea {
        id: menuCancelArea

        anchors.fill: parent
        enabled: false
        z: 11

        onClicked: menuLoader.hide()
    }

    /** The overlay for displaying popup menus and combo boxes.
     */
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

    /** Once the QML file finishes loading, show the application window.
     */
    Component.onCompleted: {
        window.minimumWidth = 360;
        window.minimumHeight = 360;
        window.fullScreen = appSettings.isMobile && appSettings.osType != 'android';
        window.showAndRaise();
    }

    Connections {
        target: window

        /** Handles a message received over RPC socket.
         */
        onMessageReceived: {
            console.log("received RPC message: " + message);
            var bits = message.split(' ');
            var command = bits[0];
            if (command == 'OPEN') {
                // TODO
                console.log("open " + bits[1]);
                dialogSwapper.showPanel('NotificationDialog.qml', {text: bits[1], title: 'Open a link'});
                window.showAndRaise();
            } else if (command == 'SHOW') {
                window.showAndRaise();
            } else if (command == 'QUIT') {
                Qt.quit();
            }
        }

        /** Show the "about" dialog.
         */
        onShowAbout: {
            dialogSwapper.showPanel('AboutDialog.qml');
        }

        /** Shows the FAQ page in the browser.
         */
        onShowHelp: {
            Qt.openUrlExternally('https://www.wifirst.net/wilink/faq');
        }

        /** Shows the "preferences" dialog.
         */
        onShowPreferences: {
            dialogSwapper.showPanel('PreferenceDialog.qml');
        }
    }

    Keys.forwardTo: dock
}
