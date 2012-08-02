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
    id: button

    property int iconSize: iconStyle != '' ? appStyle.icon.smallSize : 0
    property string iconStyle: ''
    property alias text: label.text
    property alias enabled: mouseArea.enabled
    signal clicked

    height: iconSize + (label.text ? 16 : 0)
    width: visible ? 64 : 0

    Gradient {
        id: hoverGradient
        GradientStop { id: hoverStop1; position: 0.0; color: 'transparent' }
        GradientStop { id: hoverStop2; position: 0.5; color: 'transparent' }
        GradientStop { id: hoverStop3; position: 1.0; color: 'transparent' }
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

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        Item {
            id: image
            anchors.horizontalCenter: parent.horizontalCenter
            width: iconSize
            height: iconSize

            Icon {
                anchors.centerIn: parent
                style: iconStyle
            }
        }

        Label {
            id: label

            anchors.left: parent.left
            anchors.right: parent.right
            color: 'white'
            elide: Text.ElideRight
            font.pixelSize: appStyle.font.smallSize
            horizontalAlignment: Text.AlignHCenter
            visible: button.text
        }
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
            PropertyChanges { target: label; opacity: 0.5 }
        },
        State {
            name: 'pressed'
            when: mouseArea.pressed
            PropertyChanges { target: hoverStop2; color: 'white' }
            PropertyChanges { target: background; opacity: 0.8 }
        },
        State {
            name: 'hovered'
            when: mouseArea.containsMouse
            PropertyChanges { target: hoverStop2; color: 'white' }
            PropertyChanges { target: background; opacity: 0.4 }
        }
    ]
}
