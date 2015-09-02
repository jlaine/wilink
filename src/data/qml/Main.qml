/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

import QtQuick 2.3
import QtQuick.LocalStorage 2.0
import wiLink 2.4
import 'scripts/storage.js' as Storage
import 'scripts/utils.js' as Utils

FocusScope {
    id: root

    focus: !appSettings.isMobile

    /** A model representing the user's accounts.
     */
    AccountModel {
        id: accountModel
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

    ApplicationSettings {
        id: appSettings

        property string wifirstBaseUrl: 'https://apps.wifirst.net'
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
        visible: false
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

        visible: false
        z: 10

        onCurrentItemChanged: {
            if (currentItem) {
                dialogSwapper.forceActiveFocus();
                x = Math.max(0, Math.floor((parent.width - currentItem.width) / 2));
                y = Math.max(0, Math.floor((parent.height - currentItem.height) / 2));
                visible = true;
                dialogOverlay.visible = true;
            } else {
                swapper.forceActiveFocus();
                visible = true;
                dialogOverlay.visible = false;
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
        visible: false
        z: 12

        function hide() {
            menuCancelArea.enabled = false;
            menuLoader.item.visible = false;
        }

        function show(x, y) {
            menuCancelArea.enabled = true;
            menuLoader.x = x;
            menuLoader.y = y;
            menuLoader.item.visible = true;
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

        // set window geometry
        var desktop = window.availableGeometry;
        window.size = Qt.size(Math.min(1280, desktop.width - 100), Math.min(960, desktop.height - 100));
        window.pos = Qt.point((desktop.width - window.width) / 2, (desktop.height - window.height) / 2);
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
                var args = Utils.parseWilinkURI(bits[1]);
                var verb = args.verb;
                if (verb) {
                    // Received a well formed wilink://verb?param1=value1&param2=value2... command
                    var params = args.params;
                    switch (verb) {
                        case 'about':
                            dialogSwapper.showPanel('AboutDialog.qml');
                            break;
                        case 'chat':
                            // Accepts the following params
                            //  * jid
                            //
                            // Ex: wilink://chat?jid=90210-001@wifirst.net
                            //
                            var jid = params.jid;
                            if ( /^[^@/ ]+@[^@/ ]+$/.test(jid) ) {
                                var panel = swapper.findPanel('ChatPanel.qml');
                                if (panel) {
                                    if (panel.wifirstRosterReceived) {
                                        // Wifirst roster received
                                        console.log('Wifirst roster received, opening chat with ' + jid);
                                        panel.showConversation(jid);
                                    } else {
                                        // Wifirst roster NOT received, postponing all actions
                                        console.log('Postponing chat opening with ' + jid + ' to when Wifirst roster will have been received');
                                        panel.delayedOpeningAction = 'open_conversation';
                                        panel.delayedOpeningJid = jid;
                                    }
                                }
                            } else {
                                swapper.showPanel('ChatPanel.qml');
                            }
                            break;
                        case 'conference':
                            // Accepts the following params
                            //  * jid
                            //
                            // Ex: wilink://conference?jid=blabla@conference.wifirst.net
                            //
                            var jid = params.jid;
                            if ( /^[^@/ ]+@[^@/ ]+$/.test(jid) ) {
                                console.log('Open chat room: ' + jid);
                                var panel = swapper.findPanel('ChatPanel.qml');
                                if (panel) {
                                    if (panel.wifirstRosterReceived) {
                                        // Wifirst roster received
                                        console.log('Wifirst roster received, opening room ' + jid);
                                        panel.showRoom(jid);
                                    } else {
                                        // Wifirst roster NOT received, postponing all actions
                                        console.log('Postponing room opening of ' + jid + ' to when Wifirst roster will have been received');
                                        panel.delayedOpeningAction = 'open_room';
                                        panel.delayedOpeningJid = jid;
                                    }
                                }
                            } else {
                                swapper.showPanel('ChatPanel.qml');
                            }
                            break;
                        case 'phone':
                            swapper.showPanel('PhonePanel.qml');
                            break;
                        case 'reload':
                            window.reload();
                            return;
                        default:
                            break;
                    }
                }
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
            Qt.openUrlExternally(appSettings.wifirstBaseUrl + '/wilink/faq');
        }

        /** Shows the "preferences" dialog.
         */
        onShowPreferences: {
            dialogSwapper.showPanel('PreferenceDialog.qml');
        }
    }

    Timer {
        interval: 100
        repeat: false
        running: true

        /** Load plugins at startup.
         */
        onTriggered: {
            Storage.initialize();

            // Handle wiLink < 2.4 migration.
            var version = Storage.getSetting('version', '');
            if (version < '2.4.0') {
                var migrated = false;
                for (var i = accountModel.count - 1; i >= 0; --i) {
                    var account = accountModel.get(i);
                    if (account.type == 'xmpp' && (account.realm == 'wifirst.net' || account.realm == 'gmail.com')) {
                        console.log("Removing old account: " + account.username);
                        accountModel.remove(i);
                        migrated = true;
                    }
                }
                if (migrated) {
                    accountModel.submit();
                }
            }
            Storage.setSetting('version', appUpdater.applicationVersion);

            // Check whether an account is configured.
            if (accountModel.count) {
                swapper.showPanel('WebPanel.qml');
                appPlugins.load();
            } else {
                swapper.showPanel('SetupBackground.qml');
                dialogSwapper.showPanel('SetupDialog.qml', {accountChoice: false});
            }
            rpcTimer.start();
        }
    }

    Timer {
        id: rpcTimer

        interval: 2000
        repeat: false

        /** Start handling RPC messages.
         */
        onTriggered: {
            window.startMessages();
        }
    }

    Keys.forwardTo: dock
}
