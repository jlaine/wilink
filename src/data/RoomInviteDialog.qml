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

Rectangle {
    property alias model: view.model

    color: '#ccc'
    radius: 10
    width: 320
    height: 240

    Item {
        id: header

        anchors.margins: 8
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 20

        Text {
            id: label

            anchors.top: parent.top
            anchors.left: parent.left
            width: paintedWidth
            text: 'Reason'
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: label.right
            anchors.leftMargin: 16
            anchors.right: parent.right
            border.color: '#c3c3c3'
            border.width: 1
            color: 'white'
            width: 100
            height: reason.paintedHeight

            TextEdit {
                id: reason
                anchors.fill: parent

                focus: true
                smooth: true
                text: ''
                textFormat: TextEdit.PlainText

                Keys.onReturnPressed: {
                }
            }
        }
    }

    ListView {
        id: view

        anchors.margins: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: footer.top
        clip: true
        model: ListModel {
            ListElement { name: 'foo'; avatar: 'peer.png' } 
        }
        delegate: Rectangle {
            width: parent.width - 1
            height: 24

            Rectangle {
                id: check

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
            }

            Image {
                id: image

                anchors.left: check.right
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
        }
    }

    Item {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 32
    }
}

