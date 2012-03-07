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
import QXmpp 0.4
import wiLink 2.0

Rectangle {
    id: statusBar

    property bool autoAway: false

    color: '#567dbc'
    height: appStyle.icon.tinySize + 3 * appStyle.margin.normal

    Component {
        id: pillDelegate

        StatusPill {
            property variant model

            presenceStatus: model.status
        }
    }

    Component {
        id: statusMenu

        Menu {
            id: menu

            iconDelegate: pillDelegate

            onItemClicked: {
                var statusType = menu.model.get(index).status;
                appClient.statusType = statusType;
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
            presenceStatus: appClient.statusType
        }

        Image {
            id: eject

            anchors.right: parent.right
            anchors.rightMargin: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
            source: 'eject.png'
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
        color: 'white'
        text: {
            switch (appClient.state) {
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
                if (appClient.statusType == QXmppPresence.Online) {
                    autoAway = true;
                    appClient.statusType = QXmppPresence.Away;
                }
            } else if (autoAway) {
                autoAway = false;
                appClient.statusType = QXmppPresence.Online;
            }
        }

        Component.onCompleted: idle.start()
    }
}
