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
import wiLink 1.2

Item {
    id: block
    clip: true

    property string currentJid
    property alias model: view.model
    property alias title: titleText.text

    signal addClicked
    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    ListHelper {
        id: listHelper
        model: view.model
    }

    Rectangle {
        id: background

        width: parent.height
        height: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width
        border.color: '#597fbe'
        border.width: 1

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
//        color: '#b0c4de'
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        border.color: '#aa567dbc'
        border.width: 1

        width: parent.width
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
            anchors.rightMargin:2
            anchors.verticalCenter: parent.verticalCenter
            height: 20
            width: 20
            iconSource: 'add.png'

            onClicked: block.addClicked()
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.margins: 2
        focus: true

        delegate: Item {
            id: item

            width: parent.width
            height: 30

            Item {
                anchors.fill: parent
                anchors.margins: 2

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
                    anchors.right: bubble.right
                    anchors.leftMargin: 3
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: model.name
                }

                Rectangle {
                    id: bubble

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: status.left
                    anchors.margins: 3
                    border.color: '#597fbe'
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: '#66597fbe' }
                        GradientStop { position: 0.6; color: '#597fbe' }
                        GradientStop { position: 1.0; color: '#66597fbe' }
                    }
                    height: label.paintedHeight + 6
                    width: label.paintedWidth + 6
                    opacity: model.messages > 0 ? 1 : 0
                    radius: 10
                    smooth: true

                    Text {
                        id: label

                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: 'white'
                        font.bold: true
                        font.pixelSize: 10
                        text: model.messages
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                StatusPill {
                    id: status
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    presenceStatus: model.status
                    width: 10
                    height: 10
                }

                MouseArea {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (mouse.button == Qt.LeftButton) {
                            block.itemClicked(model);
                        } else if (mouse.button == Qt.RightButton) {
                            block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                        }
                    }
                }
            }
        }

        highlight: Highlight {}

        Connections {
            target: block

            onCurrentJidChanged: {
                for (var i = 0; i < listHelper.count; i++) {
                    if (listHelper.get(i).jid == currentJid) {
                        view.currentIndex = i;
                        return;
                    }
                }
                view.currentIndex = -1;
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
