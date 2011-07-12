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
import wiLink 2.0
import 'utils.js' as Utils

FocusScope {
    id: root

    property int pendingMessages
    focus: true

    Client {
        id: appClient

        logger: QXmppLogger {
            loggingType: QXmppLogger.SignalLogging
        }

        onAuthenticationFailed: {
            console.log("Failed to authenticate with chat server");
            var jid = Utils.jidToBareJid(appClient.jid);
            dialogSwapper.showPanel('AccountPasswordDialog.qml', {'jid': jid});
        }

        onConflictReceived: {
            console.log("Received a resource conflict from chat server");
            application.quit();
        }
    }

    Item {
        id: appClipboard

        visible: false

        function copy(text) {
            helper.text = text;
            helper.selectAll();
            helper.copy();
        }

        TextEdit {
            id: helper
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

    PanelSwapper {
        id: dialogSwapper

        opacity: 0
        z: 10

        onCurrentItemChanged: {
            if (currentItem) {
                dialogSwapper.focus = true;
                x = Math.max(0, Math.floor((parent.width - currentItem.width) / 2));
                y = Math.max(0, Math.floor((parent.height - currentItem.height) / 2));
                opacity = 1;
            } else {
                swapper.focus = true;
                opacity = 0;
            }
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
        window.minimumWidth = 360;
        window.minimumHeight = 360;

        var jid = window.objectName;
        if (jid == '') {
            console.log("Failed to get window JID");
            application.quit();
        }

        var password = appWallet.get(jid);
        appClient.connectToServer(jid, password);
        swapper.showPanel('ChatPanel.qml');
    }

    Keys.forwardTo: dock
}
