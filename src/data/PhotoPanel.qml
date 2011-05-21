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

Panel {
    id: panel

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        icon: 'photos.png'
        title: '<b>' + qsTr('Photos') + '</b>'
        z: 1
    }

    GridView {
        id: view

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: header.bottom
        cellWidth: 130
        cellHeight: 150

        model: ListModel {
            ListElement { name: 'Album1'; type: 'album' }
            ListElement { name: 'Album2'; type: 'album' }
            ListElement { name: 'Album3'; type: 'album' }
            ListElement { name: 'Photo1'; type: 'photo'; source: 'file-128.png' }
            ListElement { name: 'Photo2'; type: 'photo'; source: 'file-128.png' }
            ListElement { name: 'Photo3'; type: 'photo'; source: 'file-128.png' }
        }

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: '#00ffffff'
                border.width: 1
                gradient: Gradient {
                    GradientStop { id: stop1; position: 0.0; color: '#00ffffff'  }
                    GradientStop { id: stop2; position: 1.0; color: '#00ffffff'  }
                }
            }

            Column {
                anchors.fill: parent

                Image {
                    id: image
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: model.type == 'photo' ? model.source : 'album-128.png'
                }

                Text {
                    id: text
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: model.name
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    parent.state = 'hovered'
                }
                onExited: {
                    parent.state = ''
                }
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: itemBackground; border.color: '#b0e2ff' }
                PropertyChanges { target: stop1;  color: '#ffffff' }
                PropertyChanges { target: stop2;  color: '#b0e2ff' }
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
