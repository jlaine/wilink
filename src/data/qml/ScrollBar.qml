/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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
    property variant orientation: Qt.Vertical

    property real contentPos: orientation == Qt.Horizontal ? flickableItem.contentX : flickableItem.contentY
    property real position: orientation == Qt.Horizontal ? flickableItem.visibleArea.xPosition : flickableItem.visibleArea.yPosition
    property real pageSize: orientation == Qt.Horizontal ? flickableItem.visibleArea.widthRatio : flickableItem.visibleArea.heightRatio

    clip: true
    state: pageSize >= 1 ? 'collapsed' : ''
    height: orientation == Qt.Horizontal ? 11 : 100
    width: orientation == Qt.Horizontal ? 100 : 11

    Item {
        id: container

        x: orientation == Qt.Horizontal ? 0 : 11
        width: orientation == Qt.Horizontal ? parent.width : parent.height
        height: orientation == Qt.Horizontal ? parent.height : parent.width

        transform: Rotation {
            angle: orientation == Qt.Horizontal ? 0: 90
        }

        Image {
            id: buttonLeft

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            source: 'image://icon/left-arrow'

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

        Rectangle {
            id: track

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: buttonLeft.right
            anchors.right: buttonRight.left
            border.color: '#c5c5c5'
            border.width: 1
            gradient: Gradient {
                GradientStop {position: 0.0; color: '#ffffff'}
                GradientStop {position: 1.0; color: '#d0d0d0'}
            }
            z: -1

            Rectangle {
                id: handle

                property int desiredWidth: Math.ceil(scrollBar.pageSize * (track.width - 2))

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
                width: Math.max(desiredWidth, 20)
                x: Math.floor(scrollBar.position * (track.width + desiredWidth - width - 2)) + 1
                y: 0

                states: State {
                    name: 'pressed'
                    PropertyChanges { target: handle; border.color: '#4e9de6' }
                }
            }

            MouseArea {
                id: clickableArea

                property real pressContentPos
                property real pressMouseX
                property real pressPageSize

                anchors.fill: parent
                drag.axis: Drag.YAxis

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
                        handle.state = 'pressed';
                        moveAction = 'drag';
                        moveQuantity = 0;
                        pressContentPos = scrollBar.contentPos;
                        pressMouseX = mouse.x;
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
                        scrollBar.moveBy((pressContentPos - scrollBar.contentPos) + (mouse.x - pressMouseX) / pressPageSize);
                    }
                }
            }
        }

        Image {
            id: buttonRight

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            source: 'image://icon/right-arrow'

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
