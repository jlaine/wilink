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

import QtQuick 1.1

FocusScope {
    id: dialog

    property alias contents: item
    property Component footerComponent: dialogFooter
    property alias helpText: help.text
    property alias title: label.text
    property int minimumWidth: 360
    property int minimumHeight: 240
    height: appStyle.isMobile ? root.height : minimumHeight
    width: appStyle.isMobile ? root.width : minimumWidth

    signal accepted
    signal close
    signal rejected

    opacity: 0

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: '#88a4d1'
        border.width: 1
        gradient: Gradient {
            GradientStop { id: backgroundStop1; position: 1.0; color: '#e7effd' }
            GradientStop { id: backgroundStop2; position: 0.0; color: '#cbdaf1' }
        }
        radius: appStyle.margin.large
        smooth: true
    }

    Component {
        id: dialogFooter

        Item {
            anchors.fill: parent

            Row {
                anchors.centerIn: parent
                spacing: appStyle.margin.large

                Button {
                    id: acceptButton

                    text: qsTr('OK')
                    onClicked: dialog.accepted()
                }

                Button {
                    id: rejectButton

                    text: qsTr('Cancel')
                    onClicked: dialog.rejected()
                }

                Keys.onEnterPressed: acceptButton.clicked()
                Keys.onEscapePressed: rejectButton.clicked()
                Keys.onReturnPressed: acceptButton.clicked()
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
        border.color: '#88a4d1'
        border.width: 1
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        height: appStyle.font.normalSize + 2 * appStyle.margin.normal
        radius: background.radius
        smooth: true

        Label {
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
        anchors.margins: appStyle.margin.large
        iconSize: 32
        opacity: text.length > 0 ? 1 : 0
    }

    Item {
        id: item

        anchors.top: (help.opacity == 1) ? help.bottom : header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: appStyle.margin.normal
    }

    Loader {
        id: footer

        anchors.margins: appStyle.margin.normal
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: appStyle.icon.smallSize + 2 * appStyle.margin.normal
        sourceComponent: footerComponent
    }

    Image {
        id: resizeButton
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 2
        opacity: 0.5
        source: 'resize.png'
        height: appStyle.icon.tinySize
        width: appStyle.icon.tinySize
        visible: !appStyle.isMobile

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

    TextInput {
        id: fakeInput
        opacity: 0
    }

    onClose: fakeInput.closeSoftwareInputPanel()
    onRejected: dialog.close()

    Keys.forwardTo: footer.item
}

