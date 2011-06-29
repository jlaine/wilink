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

FocusScope {
    id: dialog

    property alias contents: item
    property Component footerComponent: dialogFooter
    property alias helpText: help.text
    property alias title: label.text
    property int minimumWidth: 360
    property int minimumHeight: 240

    signal accepted
    signal close
    signal rejected

    opacity: 0

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: '#aa567dbc'
        border.width: 1
        gradient: Gradient {
            GradientStop { id: backgroundStop1; position: 1.0; color: '#e7effd' }
            GradientStop { id: backgroundStop2; position: 0.0; color: '#cbdaf1' }
        }
        radius: 10
        smooth: true
    }

    Component {
        id: dialogFooter

        Row {
            spacing: 8

            Button {
                text: qsTr('OK')
                onClicked: dialog.accepted()
            }

            Button {
                text: qsTr('Cancel')
                onClicked: dialog.rejected()
            }
        }
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

    // This MouseArea prevents clicks on items behind dialog
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
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
        radius: background.radius
        smooth: true

        Text {
            id: label

            anchors.centerIn: parent
            font.bold: true
            color: '#ffffff'
        }

        MouseArea {
            property variant mousePress
            property variant dialogPress

            anchors.fill: parent

            onPressed: {
                mousePress = mapToItem(dialog.parent, mouse.x, mouse.y);
                dialogPress = {x: dialog.x, y: dialog.y};
            }

            onPositionChanged: {
                if (mouse.buttons & Qt.LeftButton) {
                    var mouseCurrent = mapToItem(dialog.parent, mouse.x, mouse.y);
                    var minPos = root.mapToItem(dialog.parent, 0, 0);
                    var maxPos = root.mapToItem(dialog.parent, root.width - dialog.width, root.height - dialog.height);

                    var positionX = dialogPress.x + (mouseCurrent.x - mousePress.x);
                    var positionY = dialogPress.y + (mouseCurrent.y - mousePress.y);

                    dialog.x = Math.max(minPos.x, Math.min(positionX, maxPos.x));
                    dialog.y = Math.max(minPos.y, Math.min(positionY, maxPos.y));
                }
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        iconSize: 32
        opacity: text.length > 0 ? 1 : 0
    }

    Item {
        id: item

        anchors.top: (help.opacity == 1) ? help.bottom : header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
    }

    Loader {
        id: footer

        anchors.margins: 8
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        height: 32
        sourceComponent: footerComponent
    }

    Image {
        id: resizeButton
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 2
        opacity: 0.5
        source: 'resize.png'
        height: 16
        width: 16

        MouseArea {
            property variant mousePress
            property variant sizePress

            anchors.fill: parent
            hoverEnabled: true

            onEntered: {
                parent.opacity = 1
            }

            onExited: {
                parent.opacity = 0.5
            }

            onPressed: {
                mousePress = mapToItem(dialog.parent, mouse.x, mouse.y);
                sizePress = {height: dialog.height, width: dialog.width};
            }

            onPositionChanged: {
                if (mouse.buttons & Qt.LeftButton) {
                    var mouseCurrent = mapToItem(dialog.parent, mouse.x, mouse.y);
                    var maxSize = root.mapToItem(dialog.parent, root.width - dialog.x, root.height - dialog.y);

                    var newWidth = sizePress.width + (mouseCurrent.x - mousePress.x);
                    var newHeight = sizePress.height + (mouseCurrent.y - mousePress.y);

                    dialog.width = Math.max(dialog.minimumWidth, Math.min(newWidth, maxSize.x));
                    dialog.height = Math.max(dialog.minimumHeight, Math.min(newHeight, maxSize.y));
                }
            }
        }
    }

    Component.onCompleted: {
        height = minimumHeight;
        width = minimumWidth;
    }

    onRejected: dialog.close()
    Keys.onEscapePressed: dialog.rejected()
}

