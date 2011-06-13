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

Rectangle {
    id: listViewItem

    property alias iconSource: image.source
    property alias text: label.text
    property bool enabled: true
    signal clicked

    color: 'transparent'
    height: 40
    state: mouseArea.pressed ? 'pressed' : (mouseArea.hovered ? 'hovered' : '')
    width: visible ? (label.paintedWidth + 24) : 0

    Gradient {
        id: hoverGradient
        GradientStop { id: hoverStop1; position: 0.0; color: '#00ffffff' }
        GradientStop { id: hoverStop2; position: 0.5; color: '#00ffffff' }
        GradientStop { id: hoverStop3; position: 1.0; color: '#00ffffff' }
    }

    // background
    Rectangle {
        id: background
        anchors.fill: parent
        gradient: hoverGradient
    }

    // left border
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        gradient: hoverGradient
        width: 1
    }

    // right border
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        gradient: hoverGradient
        width: 1
    }

    Image {
        id: image

        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: listViewItem.enabled ? 1 : 0.5
        smooth: true
        width: 24
        height: 24
    }

    Text {
        id: label

        anchors.top: image.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: listViewItem.enabled ? 1 : 0.5
        color: 'white'
        font.pixelSize: appStyle.font.smallSize
    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges { target: hoverStop2; color: 'white' }
            PropertyChanges { target: background; opacity: 0.4 }
        },
        State {
            name: 'pressed'
            PropertyChanges { target: hoverStop2; color: 'white' }
            PropertyChanges { target: background; opacity: 0.8 }
        }
    ]

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        property bool pressed: false
        property bool hovered: false

        onClicked: {
            if (listViewItem.enabled) {
                listViewItem.clicked();
            }
        }
        onPressed: {
            if (listViewItem.enabled) {
                pressed = true;
            }
        }
        onReleased: pressed = false
        onEntered: {
            if (listViewItem.enabled) {
                hovered = true;
            }
        }
        onExited: hovered = false
    }
}
