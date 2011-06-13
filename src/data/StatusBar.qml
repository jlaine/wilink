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
        anchors.left: parent.left
        anchors.margins: 4
        delegate: Item {
            id: item

            height: 25
            width: parent.width

            Rectangle {
                id: background

                anchors.fill: parent
                border.width: 1
                border.color: 'transparent'
                color: 'transparent'
                radius: block.radius
                smooth: block.smooth
            }

            StatusPill {
                id: statusPill

                anchors.left:  parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 5
                height: 16
                width: 16
                presenceStatus: model.status
            }

            Text {
                id: text

                anchors.left: statusPill.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 5
                color: 'white'
                text: model.name
            }

            MouseArea {
                property variant positionPressed
                property int pressedIndex

                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    item.state = 'hovered'
                }

                onExited: {
                    item.state = ''
                }

                onPressed: {
                    if (block.state != 'expanded') {
                        block.state = 'expanded'
                    } else {
                        var pos = mapToItem(view, mouse.x, mouse.y)
                        currentIndex = view.indexAt(pos.x, pos.y)
                        block.state = ''
                    }
                }
            }
        }

        model: ListModel {}
    }

    Idle {
        id: idle

        Component.onCompleted: idle.start()
    }

    Component.onCompleted: {
        combo.model.append({'name': qsTr('Available'), 'status': QXmppPresence.Online});
        combo.model.append({'name': qsTr('Away'), 'status': QXmppPresence.Away});
        combo.model.append({'name': qsTr('Busy'), 'status': QXmppPresence.DND});
        combo.model.append({'name': qsTr('Offline'), 'status': QXmppPresence.Offline});
        combo.setStatusType(window.client.statusType)
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
