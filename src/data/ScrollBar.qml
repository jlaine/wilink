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
    property bool autoMove: false
    property bool dragToBottomEnabled
    property string moveAction: ''
    property real position: flickableItem.visibleArea.yPosition
    property real pageSize: flickableItem.visibleArea.heightRatio

    width: 17

    Rectangle {
        id: track

        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        color: '#dfdfdf'

        Rectangle {
            id: handle

            property int minHeight: 20

            border.color: '#c3c3c3'
            border.width: 1
            color: '#c3c3c3'
            x: 0
            y: Math.floor(scrollBar.position * (track.height - 2)) + 1
            height: Math.ceil(scrollBar.pageSize * (track.height - 2))
            width: parent.width - 1
            radius: 6
        }
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
            }

            onReleased: {
                buttonUp.state = ''
                scrollBar.moveUp()
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
                scrollBar.moveDown()
            }
        }
        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonDown; color: '#aaaaaa' }
        }
    }

    states: State {
        name: 'hovered'
        PropertyChanges { target: handle; color: '#aac7e4'; border.color: '#5488bb' }
    }

    MouseArea {
        id: clickableArea

        property int mousePressY

        anchors.fill: track
        drag.axis: Drag.YAxis

        onPressed: {
            scrollBar.state = 'hovered'
            mousePressY = mouse.y

            if (mouse.y < handle.y) {
                scrollBar.moveUp();
                moveAction = 'up';
            } else if(mouse.y > handle.y + handle.height) {
                scrollBar.moveDown();
                moveAction = 'down';
            } else {
                moveAction = 'drag';
            }
        }

        onReleased: {
            autoMove = false
            scrollBar.state = ''

            if (moveAction == 'drag') {
                scrollBar.dropHandle(mousePressY, mouse.y)
            }

            moveAction = ''
        }

        onPositionChanged: {
            if (moveAction == 'drag') {
                scrollBar.dragHandle(mousePressY, mouse.y)
            }
        }
    }

    Timer {
        id: delayTimer

        interval: 350
        running: moveAction == 'up' || moveAction == 'down'

        onTriggered: autoMove=true
    }

    Timer {
        id: repeatTimer

        interval: 60
        repeat: true
        running: autoMove

        onTriggered: {
            if (moveAction == 'up')
                scrollBar.moveUp();
            else if (moveAction == 'down')
                scrollBar.moveDown();
        }
    }

    function moveDown() {
        var delta = Math.max(0, Math.min((1 - position - pageSize) * flickableItem.contentHeight, 30));
        flickableItem.contentY += delta;
    }

    function moveUp() {
        var delta = Math.max(0, Math.min(position * flickableItem.contentHeight, 30));
        flickableItem.contentY = Math.round(flickableItem.contentY - delta);
    }

    function dragHandle(origin, target)
    {
        var density = flickableItem.count / scrollBar.height
        var targetIndex = flickableItem.currentIndex + density * (target - origin)

        if (targetIndex < 0) {
            flickableItem.positionViewAtIndex(0, ListView.Start);
        } else if (targetIndex > flickableItem.count - 1 ) {
            flickableItem.positionViewAtIndex(flickableItem.count - 1, ListView.End);
        } else {
            flickableItem.positionViewAtIndex(targetIndex, ListView.Visible);
        }
    }

    function dropHandle(origin, target)
    {
        var density = flickableItem.count / scrollBar.height
        var targetIndex = flickableItem.currentIndex + density * (target - origin)

        if (targetIndex < 0) {
            flickableItem.currentIndex = 0
        } else if (targetIndex > flickableItem.count - 1 ) {
            flickableItem.currentIndex = flickableItem.count - 1
        } else {
            flickableItem.currentIndex = targetIndex
        }
    }
}
