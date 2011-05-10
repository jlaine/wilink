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
    id: scrollBar

    property ListView flickableItem
    property real position: flickableItem.visibleArea.yPosition
    property real pageSize: flickableItem.visibleArea.heightRatio
    property int minHeight: 20

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
        y: scrollBar.position * (track.height - minHeight - 2) + 1
        height: scrollBar.pageSize * (track.height - minHeight - 2) + minHeight
        width: parent.width - 1
        radius: 6
    }

    MouseArea {
        property real pressContentY
        property real pressMouseY
        property real pressScale
        anchors.fill: scrollBar

        drag.axis: Drag.YAxis

        onPressed: {
            pressContentY = flickableItem.contentY
            pressMouseY = mouse.y
            pressScale = scrollBar.pageSize
            scrollBar.state = 'hovered'
        }

        onReleased: {
            scrollBar.state = ''
        }

        onPositionChanged: {
            var targetY = pressContentY + (mouse.y - pressMouseY) / pressScale;
            targetY = Math.max(0, targetY);
            if (scrollBar.position + scrollBar.pageSize >= 1 && targetY >= flickableItem.contentY) {
                return;
            }
            flickableItem.contentY = targetY;
        }
    }

    states: State {
        name: "hovered"
        PropertyChanges { target: handle; color: '#aac7e4'; border.color: '#5488bb' }
    }
}

