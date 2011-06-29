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

import QtQuick 1.0
import QXmpp 0.4
import wiLink 1.2
import 'utils.js' as Utils

FocusScope {
    id: root
    focus: true

    Client {
        id: appClient

        logger: QXmppLogger {
            loggingType: QXmppLogger.SignalLogging
        }

        onAuthenticationFailed: {
            console.log("Failed to authenticate with chat server");
            var jid = Utils.jidToBareJid(appClient.jid);
            dialogLoader.showDialog('AccountPasswordDialog.qml', {'jid': jid});
        }

        onConflictReceived: {
            console.log("Received a resource conflict from chat server");
            application.quit();
        }
    }

    Style {
        id: appStyle
    }

    Wallet {
        id: appWallet
    }

    Dock {
        id: dock

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 1
    }

    PanelSwapper {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: dock.right
        anchors.right: parent.right
        focus: true
    }

    MouseArea {
        id: cancelArea

        anchors.fill: parent
        enabled: false
        z: 11

        onClicked: menuLoader.hide()
    }

    Loader {
        id: dialogLoader

        property variant lastSource
        property variant properties

        z: 10

        function hide() {
            dialogLoader.item.opacity = 0;
            swapper.focus = true;
        }

        function show() {
            dialogLoader.item.opacity = 1;
            dialogLoader.focus = true;
        }

        function showDialog(source, properties) {
            if (dialogLoader.lastSource == source) {
                for (var key in properties) {
                    dialogLoader.item[key] = properties[key];
                }
                dialogLoader.item.opacity = 1;
                dialogLoader.focus = true;
            } else {
                dialogLoader.properties = properties;
                dialogLoader.source = source;
                dialogLoader.lastSource = source;
            }
        }

        onLoaded: {
            x = Math.max(0, Math.floor((parent.width - width) / 2));
            y = Math.max(0, Math.floor((parent.height - height) / 2));
            for (var key in dialogLoader.properties) {
                dialogLoader.item[key] = dialogLoader.properties[key];
            }
            dialogLoader.item.opacity = 1;
            dialogLoader.focus = true;
        }

        Connections {
            target: dialogLoader.item
            onRejected: dialogLoader.hide()
        }
    }

    Loader {
        id: menuLoader

        z: 12

        function hide() {
            cancelArea.enabled = false;
            menuLoader.item.opacity = 0;
        }

        function show(x, y) {
            cancelArea.enabled = true;
            menuLoader.x = x;
            menuLoader.y = y;
            menuLoader.item.opacity = 1;
        }

        Connections {
            target: menuLoader.item
            onItemClicked: menuLoader.hide()
        }
    }

    Component.onCompleted: {
        var jid = window.objectName;
        if (jid == '') {
            console.log("Failed to get window JID");
            application.quit();
        }

        var password = appWallet.get(jid);
        if (password == '') {
            dialogLoader.showDialog('AccountPasswordDialog.qml', {'jid': jid});
        } else {
            appClient.connectToServer(jid, password);
        }
        swapper.showPanel('ChatPanel.qml');
    }

    Keys.forwardTo: dock
}
