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

    property alias count: view.count
    property alias model: view.model
    property alias title: titleText.text

    signal addClicked
    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    Rectangle {
        id: background

        width: parent.height
        height: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width

        gradient: Gradient {
            GradientStop { id: backgroundStop1; position: 0.0; color: '#e7effd' }
            GradientStop { id: backgroundStop2; position: 1.0; color: '#cbdaf1' }
        }

        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: background.height
        }
    }

    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        border.color: '#88a4d1'
        border.width: 1
        height: 24
        z: 1

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            color: '#ffffff'
            font.bold: true
        }

        Button {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            iconSize: appStyle.icon.tinySize
            iconSource: 'add.png'

            onClicked: block.addClicked()
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        delegate: Item {
            id: item

            height: 30
            width: parent.width

            Column {
                anchors.left: parent.left
                anchors.right: callButtonLoader.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    id: name

                    anchors.left: parent.left
                    anchors.right: parent.right
                    text: model.name
                }

                Text {
                    id: phone

                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.right: parent.right
                    font.italic: true
                    text: model.phone
                    visible: false
                }
            }

            Loader {
                id: callButtonLoader

                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                z: 1
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        if (item.state == 'expanded') {
                            item.state = '';
                        } else {
                            item.state = 'expanded';
                        }
                    } else if (mouse.button == Qt.RightButton) {
                        block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                    }
                }

                onEntered: {
                    callButtonComponent.model = model;
                    callButtonLoader.sourceComponent = callButtonComponent;
                }

                onExited: {
                    callButtonLoader.sourceComponent = undefined;
                }
            }

            states: State {
                name: 'expanded'
                PropertyChanges { target: item; height: 40 }
                PropertyChanges { target: phone; visible: true }
            }
        }
    }

    Component {
        id: callButtonComponent

        property variant model

        Button {
            iconSize: appStyle.icon.tinySize
            iconSource: 'call.png'
            smooth: true

            onClicked: {
                block.itemClicked(model);
            }
        }
    }
}
