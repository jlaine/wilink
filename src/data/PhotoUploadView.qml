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

    height: view.count > 0 ? 120 : 0

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#ffffff'
    }

    Rectangle {
        id: border

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#93b9f2'
        height: 1
    }

    ListView {
        id: view

        anchors.top: border.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        spacing: 4

        delegate: Item {
            width: view.width - 1
            height: appStyle.icon.smallSize

            Image {
                id: thumbnail


                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                width: appStyle.icon.smallSize
                height: appStyle.icon.smallSize
                smooth: true
                source: model.avatar
            }

            Item {
                id: main

                anchors.left: thumbnail.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 4

                ProgressBar {
                    anchors.fill: parent
                    maximumValue: 1.0
                    value: model.progress
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
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

