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
    id: block

    property alias model: view.model
    property alias title: titleText.text

    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#cccccc'
        width: parent.width
        height: 24
        z: 1

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        clip: true
        focus: true

        delegate: Item {
            id: item

            width: parent.width
            height: 30
            state: view.currentItem == item ? 'selected' : ''

            Rectangle {
                id: rect

                anchors.fill: parent
                anchors.margins: 2
                border.color: 'transparent'
                border.width: 1
                gradient: Gradient {
                    GradientStop { id:stop1; position:0.0; color: '#ffffff' }
                    GradientStop { id:stop2; position:0.5; color: '#ffffff' }
                    GradientStop { id:stop3; position:1.0; color: '#ffffff' }
                }
                radius: 5
                smooth: true

                Image {
                    id: avatar

                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    smooth: true
                    source: model.avatar
                }

                Text {
                    anchors.left: avatar.right
                    anchors.leftMargin: 6
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                    text: model.name
                }

                MouseArea {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (mouse.button == Qt.LeftButton) {
                            view.currentIndex = index;
                            block.itemClicked(model);
                        } else if (mouse.button == Qt.RightButton) {
                            block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                        }
                    }
                }
            }

            states: State {
                name: 'selected'
                PropertyChanges { target: rect; border.color: '#ffb0c4de' }
                PropertyChanges { target: stop1; color: '#33b0c4de' }
                PropertyChanges { target: stop2; color: '#ffb0c4de' }
                PropertyChanges { target: stop3; color: '#33b0c4de' }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}
