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

Rectangle {
    id: statusBar

    property bool autoAway: false

    color: '#567dbc'

    ComboBox {
        id: combo

        function statusType() {
            return combo.model.get(combo.currentIndex).status;
        }

        function setStatusType(statusType) {
            for (var i = 0; i < combo.model.count; i++) {
                if (combo.model.get(i).status == statusType) {
                    combo.currentIndex = i;
                    break;
                }
            }
        }

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 4
        currentIndex: 3

        model: ListModel {
            ListElement { name: 'Available'; status: QXmppPresence.Online }
            ListElement { name: 'Away'; status: QXmppPresence.Away }
            ListElement { name: 'Busy'; status: QXmppPresence.DND }
            ListElement { name: 'Offline'; status: QXmppPresence.Offline }
        }
        width: 200
    }

    Idle {
        id: idle

        Component.onCompleted: idle.start()
    }

    Connections {
        target: idle

        onIdleTimeChanged: {
            if (idle.idleTime >= 300) {
                if (combo.statusType() == QXmppPresence.Online) {
                    autoAway = true;
                    combo.setStatusType(QXmppPresence.Away);
                }
            } else if (autoAway) {
                autoAway = false;
                combo.setStatusType(QXmppPresence.Online);
            }
        }
    }

    Connections {
        target: combo

        onCurrentIndexChanged: {
            var statusType = combo.statusType();
            if (statusType != window.client.statusType)
                window.client.statusType = statusType;
        }
    }

    Connections {
        target: window.client

        onConnected: combo.setStatusType(window.client.statusType)
        onDisconnected: combo.setStatusType(QXmppPresence.Offline)
    }
}
