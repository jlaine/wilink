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

Rectangle {
    id: button

    property bool enabled: true
    property int iconSize: iconStyle != '' ? appStyle.icon.smallSize : 0
    property alias iconColor: icon.color
    property string iconStyle: ''
    property int margins: (text != '') ? appStyle.margin.large : appStyle.margin.normal;
    property string style: ''
    property string text: ''
    property string iconColor: '#333333'

    signal clicked
    signal pressed
    signal released

    height: button.visible ? (Math.max(iconSize, Math.round(labelHelper.height*1.2)) + 2 * appStyle.margin.normal) : 0
    width: button.visible ? Math.round(labelHelper.width*1.2 + iconSize + ((iconStyle != '' && text != '') ? 3 : 2) * margins) : 0
    border.color: '#b3b3b3'
    gradient: Gradient {
        GradientStop { id: stop1; position: 0.0; color: '#ffffff' }
        GradientStop { id: stop2; position: 1.0; color: '#e6e6e6' }
    }
    radius: appStyle.margin.normal
    smooth: true

    Item {
        id: image

        anchors.left: parent.left
        anchors.leftMargin: margins
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.bottom: parent.bottom
        width: iconSize

        Icon {
            id: icon
            anchors.centerIn: parent
            style: iconStyle
            color: iconColor
        }
    }

    Label {
        id: label

        anchors.left: image.right
        anchors.leftMargin: iconStyle != '' ? margins : 0
        anchors.right: parent.right
        anchors.rightMargin: margins
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        text: button.text

        Label {
            id: labelShadow

            property int yOffset: 1

            anchors.fill: parent
            anchors.topMargin: yOffset
            anchors.bottomMargin: yOffset
            color: Qt.rgba(255, 255, 255, 0.75)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            z: -1
        }
    }

    Label {
        id: labelHelper

        opacity: 0
        text: button.text
    }

    Rectangle {
        id: overlay

        anchors.fill: button
        radius: button.radius
        smooth: true
        visible: false

        states: [
            State {
                name: 'disabled'
                when: !button.enabled
                PropertyChanges { target: overlay; color: '#99ffffff'; visible: true }
            },
            State {
                name: 'pressed'
                when: mouseArea.pressedButtons & Qt.LeftButton
                PropertyChanges { target: overlay; color: '#33000000'; visible: true }
            }
        ]
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        enabled: button.enabled

        onClicked: button.clicked()
        onPressed: button.pressed()
        onReleased: button.released()
    }

    state: button.style
    states: [
        State {
            name: 'primary'
            PropertyChanges { target: button; border.color: '#0074cc' }
            PropertyChanges { target: label; color: 'white' }
            PropertyChanges { target: labelShadow; color: Qt.rgba(0, 0, 0, 0.25); yOffset: -1 }
            PropertyChanges { target: stop1; color: '#0088cc' }
            PropertyChanges { target: stop2; color: '#0055cc' }
        }
    ]
}

