/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

import QtQuick 2.3

Item {
    id: scrollBar

    property Flickable flickableItem
    property int defaultQuantity: 30
    property string moveAction: ''
    property int moveQuantity: defaultQuantity
    property bool moveRepeat: false
    property variant orientation: Qt.Vertical

    property real contentPos: orientation == Qt.Horizontal ? flickableItem.contentX : flickableItem.contentY
    property real position: orientation == Qt.Horizontal ? flickableItem.visibleArea.xPosition : flickableItem.visibleArea.yPosition
    property real pageSize: Math.min(1, orientation == Qt.Horizontal ? (flickableItem.visibleArea.widthRatio > 0 ? flickableItem.visibleArea.widthRatio : 1) : (flickableItem.visibleArea.heightRatio > 0 ? flickableItem.visibleArea.heightRatio : 1))

    clip: true
    height: orientation == Qt.Horizontal ? 8 : 100
    width: orientation == Qt.Horizontal ? 100 : 8

    Item {
        id: container

        x: orientation == Qt.Horizontal ? 0 : 8
        width: orientation == Qt.Horizontal ? parent.width : parent.height
        height: 8

        transform: Rotation {
            angle: orientation == Qt.Horizontal ? 0: 90
        }

        Item {
            id: track

            anchors.fill: parent

            Rectangle {
                id: handle

                property int desiredWidth: Math.ceil(scrollBar.pageSize * (track.width - 2))

                color: '#7f7f7f'
                opacity: 0.3
                radius: 10
                smooth: true

                height: 5
                width: Math.max(desiredWidth, 20)
                x: Math.floor(scrollBar.position * (track.width + desiredWidth - width - 2)) + 1
                y: 2

                states: State {
                    name: 'active'
                    when: mouseArea.pressed || mouseArea.containsMouse || flickableItem.moving
                    PropertyChanges { target: handle; opacity: 0.7 }
                }

                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
            }

            MouseArea {
                id: mouseArea

                property real pressContentPos
                property real pressMouseX
                property real pressPageSize

                anchors.fill: parent
                drag.axis: Drag.YAxis
                hoverEnabled: true

                onPressed: {
                    if (mouse.x < handle.x) {
                        moveAction = 'up';
                        moveQuantity = -flickableItem.height;
                        scrollBar.moveBy(moveQuantity);
                    } else if (mouse.x > handle.x + handle.width) {
                        moveAction = 'down';
                        moveQuantity = flickableItem.height;
                        scrollBar.moveBy(moveQuantity);
                    } else {
                        moveAction = 'drag';
                        moveQuantity = 0;
                        pressContentPos = scrollBar.contentPos;
                        pressMouseX = mouse.x;
                        pressPageSize = scrollBar.pageSize;
                    }
                }

                onReleased: {
                    moveAction = '';
                    moveRepeat = false;
                }

                onPositionChanged: {
                    if (moveAction == 'drag') {
                        scrollBar.moveBy((pressContentPos - scrollBar.contentPos) + (mouse.x - pressMouseX) / pressPageSize);
                    }
                }
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
        var contentSize = (orientation == Qt.Horizontal) ? flickableItem.contentWidth : flickableItem.contentHeight;

        // do not exceed bottom
        delta = Math.min((1 - position - pageSize) * contentSize, delta);

        // do not exceed top
        delta = Math.max(-position * contentSize, delta);

        // move
        if (orientation == Qt.Horizontal)
            flickableItem.contentX = Math.round(flickableItem.contentX + delta);
        else
            flickableItem.contentY = Math.round(flickableItem.contentY + delta);
    }

    states: State {
        name: 'collapsed'
        when: pageSize == 1
        PropertyChanges { target: scrollBar; width: 0; opacity: 0 }
    }

    Keys.onPressed: {
        if (event.modifiers != Qt.NoModifier && event.modifiers != Qt.KeypadModifier)
            return;

        if (scrollBar.orientation == Qt.Horizontal) {
            if (event.key == Qt.Key_Left) {
                scrollBar.moveBy(-defaultQuantity);
                event.accepted = true;
            } else if (event.key == Qt.Key_Right) {
                scrollBar.moveBy(defaultQuantity);
                event.accepted = true;
            }
        } else {
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
}
