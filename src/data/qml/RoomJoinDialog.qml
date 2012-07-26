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
import wiLink 2.0
import 'utils.js' as Utils

Dialog {
    id: dialog

    property QtObject client
    property alias model: roomView.model

    title: qsTr('Join or create a chat room')
    minimumWidth: 360
    minimumHeight: 300

    onAccepted: {
        if (roomEdit.text.length) {
            var jid = roomEdit.text;
            if (jid.indexOf('@') < 0)
                jid += '@' + roomView.model.rootJid;
            var panel = swapper.findPanel('ChatPanel.qml');
            panel.showRoom(jid);
            dialog.close();
        }
    }

    Item {
        anchors.fill: contents

        PanelHelp {
            id: help

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            iconSize: 32
            text: qsTr('Enter the name of the chat room you want to join. If the chat room does not exist yet, it will be created for you.')
        }

        InputBar {
            id: roomEdit

            anchors.top: help.bottom
            anchors.topMargin: appStyle.margin.normal
            anchors.left: parent.left
            anchors.right: parent.right
            focus: true
        }

        ScrollView {
            id: roomView

            anchors.top: roomEdit.bottom
            anchors.topMargin: appStyle.margin.normal
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true
            model: DiscoveryModel {
                details: false
                manager: Qt.isQtObject(dialog.client) ? dialog.client.discoveryManager : null
                rootJid: Qt.isQtObject(dialog.client) ? dialog.client.mucServer : ''
            }

            delegate: Rectangle {
                width: parent.width - 1
                height: appStyle.icon.smallSize

                Image {
                    id: image

                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    source: 'image://icon/chat'
                    sourceSize.width: appStyle.icon.smallSize
                    sourceSize.height: appStyle.icon.smallSize
                }

                Label {
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
    }
}

