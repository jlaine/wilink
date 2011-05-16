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
    id: scrollBarView

    property int minHeight
    property real position
    property real pageSize

    Rectangle {
        id: track

        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        color: '#dfdfdf'
    }

    Rectangle {
        id: handle

        property int size: scrollBarView.pageSize * (track.height - 2)
        property int position: scrollBarView.position * (track.height - 2) + track.anchors.topMargin + 1

        border.color: '#c3c3c3'
        border.width: 1
        color: '#c3c3c3'
        x: 0
        y: size < minHeight ? position - scrollBarView.position * (minHeight - size) : position
        height: size < minHeight ? minHeight : size
        width: parent.width - 1
        radius: 6
    }

    Rectangle {
        id: buttonUp

        anchors.top: parent.top
        height: parent.width - 1
        width: parent.width - 1
        border.color: '#999999'
        color: '#cccccc'

        Image {
            anchors.fill: parent
            smooth: true
            source: 'back.png'
            transform: Rotation { angle: 90; origin.x: 8; origin.y: 8 }
        }

        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonUp.state = 'pressed'
                dragToBottomEnabled = false
            }

            onReleased: {
                buttonUp.state = ''
                scrollBarView.moveUp()
            }
        }

        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonUp; color: '#aaaaaa' }
        }
    }

    Rectangle {
        id: buttonDown

        anchors.bottom: parent.bottom
        height: parent.width - 1
        width: parent.width - 1
        border.color: '#999999'
        color: '#cccccc'

        Image {
            anchors.fill: parent
            smooth: true
            source: 'back.png'
            transform: Rotation { angle: -90; origin.x: 8; origin.y: 8 }
        }

        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonDown.state = 'pressed'
            }

            onReleased: {
                buttonDown.state = ''
                scrollBarView.moveDown()
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

    MouseArea {
        id: clickableArea

        property int mousePressY

        anchors.fill: track
        drag.axis: Drag.YAxis

        onPressed: {
            scrollBarView.state = 'hovered'
            mousePressY = mouse.y

            if( mouse.y < handle.y ) {
                dragToBottomEnabled = false
                moveAction = "up"
            } else if( mouse.y > handle.y + handle.height ) {
                moveAction = "down"
            } else {
                dragToBottomEnabled = false
                moveAction = "drag"
            }
        }

        onReleased: {
            autoMove = false
            scrollBarView.state = ''

            switch ( moveAction )
            {
                case "up":
                    scrollBarView.moveUp()
                break
                case "down":
                    scrollBarView.moveDown()
                break
                case "drag":
                    scrollBarView.dropHandle(mousePressY, mouse.y)
                break
            }

            moveAction = ''
        }

        onPositionChanged: {
            if (moveAction == "drag") {
                scrollBarView.dragHandle(mousePressY, mouse.y)
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
        onTriggered: moveAction == "up" ? scrollBarView.moveUp() : scrollBarView.moveDown()
    }

    // should be reimplemented by the parent
    function moveUp() {}
    function moveDown() {}
    function dragHandle() {}
    function dropHandle() {}
}
