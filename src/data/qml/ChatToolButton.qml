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

import QtQuick 1.1

Item {
    id: button

    property int iconSize: iconStyle != '' ? appStyle.icon.tinySize * 1.2 : 0
    property string iconStyle: ''
    property alias enabled: mouseArea.enabled
    signal clicked

    height: iconSize
    width: visible ? iconSize : 0

    // background
    Rectangle {
        id: background

        anchors.fill: parent
        color: 'black'
        opacity: 0.1
        radius: 3

    }

    Icon {
        id: image

        anchors.centerIn: background
        style: iconStyle
        color: '#c7c7c7'
        font.pixelSize: appStyle.font.tinySize
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: button.clicked()
    }

    states: [
        State {
            name: 'disabled'
            when: !enabled
            PropertyChanges { target: image; opacity: 0.5 }
        },
        State {
            name: 'pressed'
            when: mouseArea.pressed
            PropertyChanges { target: background; opacity: 1 }
        },
        State {
            name: 'hovered'
            when: mouseArea.containsMouse
            PropertyChanges { target: background; opacity: 1 }
        }
    ]
}
