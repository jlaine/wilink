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

    property Flickable flickableItem
    property string moveAction: ''
    property int moveQuantity: 30
    property bool moveRepeat: false
    property real position: flickableItem.visibleArea.yPosition
    property real pageSize: flickableItem.visibleArea.heightRatio

    state: pageSize == 1 ? 'collapsed' : ''
    width: 11

    Image {
        id: track

        anchors.fill: parent
        anchors.topMargin: scrollBar.width - 1
        anchors.bottomMargin: scrollBar.width - 1
        source: 'scrollBarTrack.svg'

        Image {
            id: handle

            property int desiredHeight: Math.ceil(scrollBar.pageSize * (track.height - 2))

            x: 0
            y: Math.floor(scrollBar.position * (track.height + desiredHeight - height - 2)) + 1
            height: Math.max(desiredHeight, 20)
            source: 'scrollBarHandle.svg'
            width: parent.width - 1
        }
    }

    Rectangle {
        id: buttonUp

        anchors.top: parent.top
        border.color: '#0d88a4'
        color: '#bedfe7'
        height: parent.width - 1
        width: parent.width - 1
/*
        Image {
            anchors.fill: parent
            smooth: true
            source: 'back.png'
            transform: Rotation { angle: 90; origin.x: Math.round(scrollBar.width/2); origin.y: Math.round(scrollBar.width/2) }
        }
*/

        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonUp.state = 'pressed';
                moveAction = 'up';
                moveQuantity = -30;
                scrollBar.moveBy(moveQuantity);
            }

            onReleased: {
                buttonUp.state = '';
                moveAction = '';
                moveRepeat = false;
            }
        }

        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonUp; color: '#ffffff' }
        }
    }

    Rectangle {
        id: buttonDown

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        border.color: '#0d88a4'
        color: '#bedfe7'
        height: parent.width - 1
        width: parent.width - 1
/*
        Image {
            anchors.fill: parent
            smooth: true
            source: 'back.png'
            transform: Rotation { angle: -90; origin.x: Math.round(scrollBar.width/2); origin.y: Math.round(scrollBar.width/2) }
        }
*/
        MouseArea {
            anchors.fill: parent

            onPressed: {
                buttonDown.state = 'pressed';
                moveAction = 'down';
                moveQuantity = 30;
                scrollBar.moveBy(moveQuantity);
            }

            onReleased: {
                buttonDown.state = '';
                moveAction = '';
                moveRepeat = false;
            }
        }
        states: State {
            name: 'pressed'
            PropertyChanges { target: buttonDown; color: '#ffffff' }
        }
    }

    MouseArea {
        id: clickableArea

        property real pressContentY
        property real pressMouseY
        property real pressPageSize

        anchors.fill: track
        drag.axis: Drag.YAxis

        onPressed: {
            if (mouse.y < handle.y) {
                moveAction = 'up';
                moveQuantity = -flickableItem.height;
                scrollBar.moveBy(moveQuantity);
            } else if (mouse.y > handle.y + handle.height) {
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
}
