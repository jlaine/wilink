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
import wiLink 2.4

Rectangle {
    id: statusBar

    property bool autoAway: false

    border.color: '#DDDDDD'
    border.width: 1
    color: '#F5F5F5'
    height: appStyle.icon.tinySize + 3 * appStyle.margin.normal

    function addClient(client) {
        clients.append({client: client});
    }

    function removeClient(client) {
        for (var i = 0; i < clients.count; ++i) {
            var chatClient = clients.get(i).client;
            if (chatClient == client) {
                clients.remove(i);
                break;
            }
        }
    }

    function setPresenceStatus(statusType) {
        statusPill.presenceStatus =  statusType;
        for (var i = 0; i < clients.count; ++i) {
            var chatClient = clients.get(i).client;
            chatClient.statusType = statusType;
        }
    }

    ListModel {
        id: clients
    }

    Component {
        id: statusMenu

        Menu {
            id: menu

            delegate: StatusDelegate {
                onClicked: itemClicked(index)
            }

            onItemClicked: {
                setPresenceStatus(menu.model.get(index).status);
            }

            Component.onCompleted: {
                menu.model.append({'text': qsTr('Available'), 'status': 'available'});
                menu.model.append({'text': qsTr('Away'), 'status': 'away'});
                menu.model.append({'text': qsTr('Busy'), 'status': 'busy'});
                menu.model.append({'text': qsTr('Offline'), 'status': 'offline'});
            }
        }
    }

    Item {
        id: statusArea
        
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: 2
        width: appStyle.icon.tinySize + 11 + 3 * appStyle.margin.normal

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
            anchors.leftMargin: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
            presenceStatus: 'available'
        }

        Label {
            id: eject

            anchors.right: parent.right
            anchors.rightMargin: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
            font.family: appStyle.icon.fontFamily
            text: '\uF052'
            z: 1
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

        transitions: Transition {
            PropertyAnimation { target: highlight; properties: 'opacity'; duration: appStyle.animation.normalDuration }
        }
    }

    Label {
        id: connectionText

        anchors.left: statusArea.right
        anchors.leftMargin: appStyle.spacing.horizontal
        anchors.right: parent.right
        anchors.rightMargin: appStyle.margin.normal
        anchors.verticalCenter: parent.verticalCenter
        text: {
            var connected = 0;
            var connecting = 0;
            for (var i = 0; i < clients.count; ++i) {
                var clientState = clients.get(i).client.state;
                switch (clientState) {
                case QXmppClient.ConnectedState:
                    connected++;
                    break;
                case QXmppClient.ConnectingState:
                    connecting++;
                    break;
                }
            }
            if (connecting)
                return qsTr('Connecting..');
            else if (connected)
                return qsTr('Connected');
            else
                return qsTr('Offline');
        }
    }

    Idle {
        id: idle

        onIdleTimeChanged: {
            if (idle.idleTime >= 300) {
                if (statusPill.presenceStatus == 'available') {
                    autoAway = true;
                    setPresenceStatus('away');
                }
            } else if (autoAway) {
                autoAway = false;
                setPresenceStatus('available');
            }
        }

        Component.onCompleted: idle.start()
    }
}
