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

    Component {
        id: statusMenu

        Menu {
            id: menu

            onItemClicked: {
                var statusType = menu.model.get(index).status;
                console.log("status req: " + statusType);
                if (statusType != window.client.statusType)
                    window.client.statusType = statusType;
            }

            Component.onCompleted: {
                menu.model.append({'text': qsTr('Available'), 'status': QXmppPresence.Online});
                menu.model.append({'text': qsTr('Away'), 'status': QXmppPresence.Away});
                menu.model.append({'text': qsTr('Busy'), 'status': QXmppPresence.DND});
                menu.model.append({'text': qsTr('Offline'), 'status': QXmppPresence.Offline});
            }
        }
    }

    Item {
        id: statusArea
        
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 2
        width: 48

        Rectangle {
            id: highlight

            anchors.fill: parent
            border.width: 1
            border.color: 'white'
            color: '#90acd8'
            opacity: 0
            radius: 3
        }

        StatusPill {
            id: statusPill

            anchors.left:  parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 5
            height: 16
            width: 16
            presenceStatus: window.client.statusType
        }

        Text {
            anchors.left: statusPill.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 5
            color: 'white'
            text: '<html>&#9167;</html>'
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onEntered: statusArea.state = 'hovered'
            onExited: statusArea.state = ''
            onClicked: {
                var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
                menuLoader.sourceComponent = statusMenu;
                menuLoader.show(pos.x, pos.y - menuLoader.item.height);
            }
        }

        states: State {
            name: 'hovered'
            PropertyChanges { target: highlight; opacity: 1 }
        }
    }

    Text {
        id: connectionText

        anchors.left: statusArea.right
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 5
        color: 'white'
        text: {
            switch (window.client.state) {
            case QXmppClient.ConnectedState:
                return qsTr('Connected');
            case QXmppClient.ConnectingState:
                return qsTr('Connecting..');
            case QXmppClient.DisconnectedState:
                return qsTr('Offline');
            }
        }
    }

    Idle {
        id: idle

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

        Component.onCompleted: idle.start()
    }

    Component.onCompleted: {
        combo.model.append({'name': qsTr('Available'), 'status': QXmppPresence.Online});
        combo.model.append({'name': qsTr('Away'), 'status': QXmppPresence.Away});
        combo.model.append({'name': qsTr('Busy'), 'status': QXmppPresence.DND});
        combo.model.append({'name': qsTr('Offline'), 'status': QXmppPresence.Offline});
        combo.setStatusType(QXmppPresence.Online);
    }
}
