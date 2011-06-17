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

Panel {
    id: panel

    function save() {
        application.incomingMessageSound = incomingMessageSound.checked ? ':/message-incoming.ogg' : '';
        application.outgoingMessageSound = outgoingMessageSound.checked ? ':/message-outgoing.ogg' : '';
    }

    GroupBox {
        id: devices

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height / 2
        title: qsTr('Sound devices')

        Column {
            anchors.fill: devices.contents
            spacing: appStyle.verticalSpacing

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: appStyle.horizontalSpacing

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: 'audio-input.png'
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr('Audio capture device')
                }
            }

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: appStyle.horizontalSpacing

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: 'audio-output.png'
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr('Audio playback device')
                }
            }

            Button {
                text: qsTr('Test')
            }
        }
    }

    GroupBox {
        id: notifications

        anchors.top: devices.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Sound notifications')

        Column {
            anchors.fill: notifications.contents
            spacing: appStyle.verticalSpacing

            CheckBox {
                id: incomingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.incomingMessageSound.length > 0
                text: qsTr('Incoming message')
            }

            CheckBox {
                id: outgoingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.outgoingMessageSound.length > 0
                text: qsTr('Outgoing message')
            }
        }
    }
}

