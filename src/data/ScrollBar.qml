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

import QtQuick 1.1

Item {
    id: scrollBar

    property Flickable flickableItem
    property int defaultQuantity: 30
    property string moveAction: ''
    property int moveQuantity: defaultQuantity
    property bool moveRepeat: false
    property real position: flickableItem.visibleArea.yPosition
    property real pageSize: flickableItem.visibleArea.heightRatio

    state: pageSize == 1 ? 'collapsed' : ''
    width: 11

    Rectangle {
        id: track

        anchors.top: scrollBar.top
        anchors.left: scrollBar.left
        anchors.topMargin: -1
        border.color: '#c5c5c5'
        border.width: 1
        gradient: Gradient {
            GradientStop {position: 0.0; color: '#ffffff'}
            GradientStop {position: 1.0; color: '#d0d0d0'}
        }
        height: parent.width
        width: parent.height - 2 * ( scrollBar.width - 1 )
        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: track.height
        }

        Rectangle {
            id: handle

            property int desiredHeight: Math.ceil(scrollBar.pageSize * (track.width - 2))

            Rectangle {
                id: handleReflect

                anchors.left: parent.left
                anchors.leftMargin: 3
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 1
                color: '#9cc6ec'
                height: Math.round(parent.height / 2 - 2)
                radius: parent.radius
                smooth: parent.smooth
            }

            border.color: '#2A85d9'
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 1.0; color: '#2061c0' }
                GradientStop { position: 0.2; color: '#afebff' }
            }
            radius: 10
            smooth: true

            height: parent.height
            width: Math.max(desiredHeight, 20)
            x: Math.floor(scrollBar.position * (track.width + desiredHeight - width - 2)) + 1
            y: 0

            states: State {
                name: 'pressed'
                PropertyChanges { target: handle; border.color: '#4e9de6' }
            }
        }
    }

    Image {
        id: buttonUp

        anchors.top: parent.top
        source: 'up-arrow.png'

        MouseArea {
            anchors.fill: parent

            onPressed: {
                moveAction = 'up';
                moveQuantity = -defaultQuantity;
                scrollBar.moveBy(moveQuantity);
            }

            onReleased: {
                moveAction = '';
                moveRepeat = false;
            }
        }
    }

    Image {
        id: buttonDown

        anchors.bottom: parent.bottom
        source: 'down-arrow.png'

        MouseArea {
            anchors.fill: parent

            onPressed: {
                moveAction = 'down';
                moveQuantity = defaultQuantity;
                scrollBar.moveBy(moveQuantity);
            }

            onReleased: {
                moveAction = '';
                moveRepeat = false;
            }
        }
    }

    MouseArea {
        id: clickableArea

        property real pressContentY
        property real pressMouseY
        property real pressPageSize

        anchors.top: buttonUp.bottom
        anchors.bottom: buttonDown.top
        anchors.left: parent.left
        anchors.right: parent.right
        drag.axis: Drag.YAxis

        onPressed: {
            if (mouse.y < handle.x) {
                moveAction = 'up';
                moveQuantity = -flickableItem.height;
                scrollBar.moveBy(moveQuantity);
            } else if (mouse.y > handle.x + handle.width) {
                moveAction = 'down';
                moveQuantity = flickableItem.height;
                scrollBar.moveBy(moveQuantity);
            } else {
                handle.state = 'pressed';
                moveAction = 'drag';
                moveQuantity = 0;
                pressContentY = flickableItem.contentY;
                pressMouseY = mouse.y;
                pressPageSize = scrollBar.pageSize;
            }
        }

        onReleased: {
            handle.state = '';
            moveAction = '';
            moveRepeat = false;
        }

        onPositionChanged: {
            if (moveAction == 'drag') {
                scrollBar.moveBy((pressContentY - flickableItem.contentY) + (mouse.y - pressMouseY) / pressPageSize);
            }
        }
    }

    Timer {
        id: delayTimer

        interval: 350
        running: moveAction == 'up' || moveAction == 'down'

        onTriggered: moveRepeat = true
    }

    Timer {
        id: repeatTimer

        interval: 60
        repeat: true
        running: moveRepeat

        onTriggered: scrollBar.moveBy(moveQuantity)
    }

    function moveBy(delta) {
        // do not exceed bottom
        delta = Math.min((1 - position - pageSize) * flickableItem.contentHeight, delta);

        // do not exceed top
        delta = Math.max(-position * flickableItem.contentHeight, delta);

        // move
        flickableItem.contentY = Math.round(flickableItem.contentY + delta);
    }

    states: State {
        name: 'collapsed'
        PropertyChanges { target: scrollBar; width: 0; opacity: 0 }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Up) {
            scrollBar.moveBy(-defaultQuantity);
            event.accepted = true;
        } else if (event.key == Qt.Key_Down) {
            scrollBar.moveBy(defaultQuantity);
            event.accepted = true;
        } else if (event.key == Qt.Key_PageUp) {
            scrollBar.moveBy(-flickableItem.height);
            event.accepted = true;
        } else if (event.key == Qt.Key_PageDown) {
            scrollBar.moveBy(flickableItem.height);
            event.accepted = true;
        }
    }
}
