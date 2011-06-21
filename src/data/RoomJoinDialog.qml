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
import wiLink 1.2

Dialog {
    id: dialog

    property alias model: view.model
    property QtObject panel: null

    helpText: qsTr('Enter the name of the chat room you want to join. If the chat room does not exist yet, it will be created for you.')
    title: qsTr('Join or create a chat room')
    minimumWidth: 360
    minimumHeight: 300

    onAccepted: {
        if (roomEdit.text.length) {
            var jid = roomEdit.text;
            if (jid.indexOf('@') < 0)
                jid += '@' + view.model.rootJid;
            parent.hide();
            panel.showRoom(jid);
        }
    }

    Item {
        anchors.fill: contents

        InputBar {
            id: roomEdit

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            Keys.onReturnPressed: dialog.accepted()
        }

        ListView {
            id: view

            anchors.top: roomEdit.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: scrollBar.left
            clip: true
            model: DiscoveryModel {
                details: false
                manager: window.client.discoveryManager
                rootJid: window.client.mucServer
            }

            delegate: Rectangle {
                width: parent.width - 1
                height: appStyle.icon.smallSize

                Image {
                    id: image

                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: appStyle.icon.smallSize
                    height: appStyle.icon.smallSize
                    smooth: true
                    source: model.avatar
                }

                Text {
                    anchors.left: image.right
                    anchors.leftMargin: 4
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.name
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        roomEdit.text = model.jid;
                        dialog.accepted();
                    }
                }
            }
        }

        ScrollBar {
            id: scrollBar

            anchors.top: roomEdit.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            clip: true
            flickableItem: view
        }
    }
}

