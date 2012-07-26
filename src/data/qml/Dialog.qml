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

GroupBox {
    id: dialog

    property alias contents: item
    property int minimumWidth: 360
    property int minimumHeight: 240

    signal accepted
    signal close
    signal rejected

    height: appStyle.isMobile ? root.height : minimumHeight
    width: appStyle.isMobile ? root.width : minimumWidth
    borderColor: '#888888'
    opacity: 0
    radius: appStyle.margin.large
    shadowOpacity: 0.8

    headerColor: 'white'
    headerComponent: Item {
        anchors.fill: parent

        Label {
            id: label

            anchors.centerIn: parent
            font.bold: true
            text: dialog.title
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

    footerComponent: Item {
        anchors.fill: parent

        Row {
            anchors.centerIn: parent
            spacing: appStyle.margin.large

            Button {
                id: acceptButton

                style: 'primary'
                text: qsTr('OK')
                onClicked: dialog.accepted()
            }

            Button {
                id: rejectButton

                text: qsTr('Cancel')
                onClicked: dialog.rejected()
            }
        }
    }

    Item {
        id: item

        anchors.fill: parent
    }

    Image {
        id: resizeButton
        anchors.bottom: dialog.bottom
        anchors.right: dialog.right
        anchors.margins: 2
        parent: dialog
        opacity: 0.5
        source: 'image://icon/resize'
        sourceSize.height: appStyle.icon.tinySize
        sourceSize.width: appStyle.icon.tinySize
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

    Keys.onEnterPressed: dialog.accepted()
    Keys.onEscapePressed: dialog.rejected()
    Keys.onReturnPressed: dialog.accepted()
}

