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

Item {
    property alias model: view.model
    property alias title: titleText.text

    clip: true

    Rectangle {
        id: rect

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#cccccc'
        width: parent.width
        height: 24
        z: 1

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    ListView {
        id: view

        anchors.top: rect.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        delegate: Item {
            id: item
            width: parent.width
            height: 28

            Rectangle {
                anchors.fill: parent
                border.color: '#ffffff'
                color: '#ffffff'
                gradient: Gradient {
                    GradientStop { id: stop1; position: 0.0; color: '#ffffff' }
                    GradientStop { id: stop2; position: 1.0; color: '#ffffff' }
                }

                Image {
                    id: avatar

                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    smooth: true
                    source: model.avatar
                }

                Text {
                    anchors.left: avatar.right
                    anchors.leftMargin: 6
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                    text: model.name
                }
            }
        }
    }
}
