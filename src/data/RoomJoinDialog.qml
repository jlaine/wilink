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

    title: qsTr('Join or create a chat room')

    onAccepted: {
        if (roomEdit.text.length) {
            dialog.hide();

            var url = 'xmpp://' + window.objectName + '/' + roomEdit.text + '@' + view.model.rootJid + '?join';
            Qt.openUrlExternally(url);
        }
    }

    Text {
        id: help

        anchors.top: contents.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        height: paintedHeight
        text: qsTr('Enter the name of the chat room you want to join. If the chat room does not exist yet, it will be created for you.')
        wrapMode: Text.WordWrap
    }

    Rectangle {
        id: bar

        anchors.margins: 8
        anchors.top: help.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        border.color: '#c3c3c3'
        border.width: 1
        color: 'white'
        width: 100
        height: roomEdit.paintedHeight

        TextEdit {
            id: roomEdit

            anchors.fill: parent
            focus: true
            smooth: true
            textFormat: TextEdit.PlainText

            Keys.onReturnPressed: {
                dialog.accepted();
                return false;
            }
        }
    }

    ListView {
        id: view

        anchors.margins: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: bar.bottom
        anchors.bottom: contents.bottom
        clip: true
        model: DiscoveryModel {
            manager: client.discoveryManager
            rootJid: client.mucServer
        }

        delegate: Rectangle {
            width: parent.width - 1
            height: 24

            Image {
                id: image

                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
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
                    dialog.hide();

                    var url = 'xmpp://' + window.objectName + '/' + model.jid + '?join';
                    Qt.openUrlExternally(url);
                }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: bar.bottom
        anchors.bottom: contents.bottom
        anchors.right: parent.right
        anchors.margins: 8
        flickableItem: view
    }
}

