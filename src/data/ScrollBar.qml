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
    property Flickable flickableItem

    width: 16

    Rectangle {
        id: track

        anchors.fill: parent
        color: '#dfdfdf'
    }

    Rectangle {
        id: handle

        border.color: '#c3c3c3'
        border.width: 1
        color: '#c3c3c3'
        x: 0
        y: flickableItem.visibleArea.yPosition * flickableItem.height
        height: flickableItem.visibleArea.heightRatio * flickableItem.height
        width: parent.width - 1
        radius: 6

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            drag {
                axis: Drag.YAxis
                target: handle
                minimumY: 0
                maximumY: track.height - handle.height
            }

            onEntered: {
                handle.state = 'hovered'
            }

            onExited: {
                handle.state = ''
            }

            onPositionChanged: {
                if (pressedButtons == Qt.LeftButton) {
                    flickableItem.contentY = Math.floor(handle.y / flickableItem.height * flickableItem.contentHeight);
                }
            }
        }

        states: State {
            name: "hovered"
            PropertyChanges { target: handle; color: '#aac7e4'; border.color: '#5488bb' }
        }
    }
}

