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

    property alias icon: image.source
    property alias text: label.text
    property bool enabled: true
    signal clicked

    color: '#6ea1f1'
    height: 24
    width: 24
    radius: 20
    state: mouseArea.pressed ? 'pressed' : (mouseArea.hovered ? 'hovered' : '')

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
        opacity: 0
        color: 'white'
        font.pixelSize: 11
    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges { target: label; opacity: 1 }
            PropertyChanges { target: image; height: 32; width: 32 }
            PropertyChanges { target: listViewItem; height: 32 + label.paintedHeight; width: 32 + label.paintedWidth }
        },
        State {
            name: 'pressed'
            PropertyChanges { target: label; opacity: 1 }
            PropertyChanges { target: image; height: 32; width: 32 }
            PropertyChanges { target: listViewItem; color: '#beceeb'; height: 32 + label.paintedHeight; width: 32 + label.paintedWidth }
        }
    ]

    transitions: Transition {
            from: ''
            to: 'hovered'
            reversible: true
            PropertyAnimation { target: label; properties: 'opacity' }
            PropertyAnimation { target: image; properties: 'height,width'; duration: 150 }
            PropertyAnimation { target: listViewItem; properties: 'height,width'; duration: 150 }
        }

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
