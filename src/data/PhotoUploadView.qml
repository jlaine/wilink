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
    id: block

    property alias model: view.model
    
    height: view.count * 32

    ListView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left

        delegate: Rectangle {
            color: 'green'
            width: view.width - 1
            height: 32

            Image {
                id: image

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                width: 32
                height: 32
                source: model.avatar
            }

            ProgressBar {
                id: progress

                anchors.margins: 8
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: image.right
                anchors.right: parent.right
                maximumValue: 1.0
                value: model.progress

                Text {
                    anchors.centerIn: parent
                    text: model.name
                }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}

