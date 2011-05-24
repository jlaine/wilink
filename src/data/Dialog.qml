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
    id: dialog

    property alias contents: item
    property alias title: label.text
    signal accepted

    border.color: '#aa567dbc'
    border.width: 1
    gradient: Gradient {
        GradientStop { id: backgroundStop1; position: 1.0; color: '#e7effd' }
        GradientStop { id: backgroundStop2; position: 0.0; color: '#cbdaf1' }
    }
    radius: 10
    smooth: true
    width: 320
    height: 240
    x: 0
    y: 0
    z: 10

    function show() {
        visible = true;
    }

    function hide() {
        visible = false;
    }

    // FIXME: this is a hack waiting 'blur' or 'shadow' attribute in qml
    BorderImage {
        id: shadow
        anchors.fill: dialog
        anchors { leftMargin: -5; topMargin: -5; rightMargin: -8; bottomMargin: -9 }
        border { left: 10; top: 10; right: 10; bottom: 10 }
        source: 'shadow.png'
        smooth: true
        z: -1
    }

    Rectangle {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        border.color: '#aa567dbc'
        border.width: 1
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        height: 20
        radius: dialog.radius
        smooth: true

        Text {
            id: label

            anchors.centerIn: parent
            font.bold: true
            color: '#ffffff'
        }

        MouseArea {
            property int mousePressX
            property int mousePressY
            property int dialogPressX
            property int dialogPressY

            anchors.fill: parent

            onPressed: {
                mousePressX = mapToItem(dialog.parent, mouse.x, mouse.y).x
                mousePressY = mapToItem(dialog.parent, mouse.x, mouse.y).y
                dialogPressX = dialog.x
                dialogPressY = dialog.y
            }

            onPositionChanged: {
                if (mouse.buttons & Qt.LeftButton) {
                    var positionX = dialogPressX + (mapToItem(dialog.parent, mouse.x, mouse.y).x - mousePressX)
                    positionX = Math.max(positionX, 0)
                    positionX = Math.min(positionX, dialog.parent.width - dialog.width)

                    var positionY = dialogPressY + (mapToItem(dialog.parent, mouse.x, mouse.y).y - mousePressY)
                    positionY = Math.max(positionY, 0)
                    positionY = Math.min(positionY, dialog.parent.height - dialog.height)

                    dialog.x = positionX
                    dialog.y = positionY
                }
            }
        }
    }

    Item {
        id: item

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: footer.top
    }

    Row {
        id: footer

        anchors.margins: 8
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: 32
        spacing: 8

        Button {
            text: qsTr('OK')
            onClicked: {
                dialog.accepted()
                dialog.hide()
            }
        }

        Button {
            text: qsTr('Cancel')
            onClicked: dialog.hide()
        }
    }
}

