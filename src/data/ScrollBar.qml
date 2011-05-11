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
    property string moveAction: ""
    property bool autoMove: false

    width: 16

    Rectangle {
        id: track

        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        color: '#dfdfdf'
    }

    Rectangle {
        id: handle

        border.color: '#c3c3c3'
        border.width: 1
        color: '#c3c3c3'
        x: 0
        y: scrollBar.position * (track.height - minHeight - 2) + track.anchors.topMargin + 1
        height: scrollBar.pageSize * (track.height - minHeight - 2) + minHeight
        width: parent.width - 1
        radius: 6
    }

    MouseArea {
        id: clickableArea

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

            if( mouse.y < handle.y ) {
                moveAction = "up"
            } else if( mouse.y > handle.y + handle.height ) {
                moveAction = "down"
            } else {
                moveAction = "drag"
            }
        }

        onReleased: {
            autoMove = false
            scrollBar.state = ''

            switch ( moveAction )
            {
                case "up":
                    moveUp()
                break
                case "down":
                    moveDown()
                break
            }

            moveAction = ''
        }

        onPositionChanged: {
            if( moveAction == "drag" ) {
                var targetY = pressContentY + (mouse.y - pressMouseY) / pressScale;
                if (mouse.y - pressMouseY > 0 && scrollBar.position + scrollBar.pageSize >= 1) {
                    flickableItem.positionViewAtIndex(flickableItem.count - 1, ListView.End);
                } else if(mouse.y - pressMouseY < 0 && scrollBar.position - scrollBar.pageSize <= 0) {
                    flickableItem.positionViewAtIndex(0, ListView.Start);
                } else {
                    flickableItem.contentY = targetY
                }
            }
        }

        Timer {
            running: moveAction == "up" || moveAction == "down"
            interval: 350
            onTriggered: autoMove=true
        }

        Timer {
            running: autoMove
            interval: 60
            repeat: true
            onTriggered: moveAction == "up" ? clickableArea.moveUp() : clickableArea.moveDown()
        }

        function moveDown() {
            var targetY = flickableItem.contentY + ( 30 / scrollBar.pageSize )
            if (scrollBar.position + scrollBar.pageSize < 1) {
                flickableItem.contentY = targetY
            }
            else
            {
                autoMove = false
                flickableItem.positionViewAtIndex(flickableItem.count - 1, ListView.End);
            }
        }

        function moveUp() {
            var targetY = flickableItem.contentY - 30 / scrollBar.pageSize
            if (scrollBar.position > 0) {
                flickableItem.contentY = targetY
            }
            else
            {
                autoMove = false
                flickableItem.positionViewAtIndex(0, ListView.Start)
            }
        }
    }

    Rectangle {
        id: buttonUp

        anchors.top: scrollBar.top
        height: 16
        width: 16
        border.color: '#999999'
        color: '#cccccc'

        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonUp.state = 'pressed'
            }

            onReleased: {
                buttonUp.state = ''
                clickableArea.moveUp()
            }
        }

        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonUp; color: '#aaaaaa' }
        }
    }

    Rectangle {
        id: buttonDown

        anchors.bottom: scrollBar.bottom
        height: 16
        width: 16
        border.color: '#999999'
        color: '#cccccc'

        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonDown.state = 'pressed'
            }

            onReleased: {
                buttonDown.state = ''
                clickableArea.moveDown()
            }
        }

        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonDown; color: '#aaaaaa' }
        }
    }

    states: State {
        name: "hovered"
        PropertyChanges { target: handle; color: '#aac7e4'; border.color: '#5488bb' }
    }
}

