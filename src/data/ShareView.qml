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

    clip: true

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

    ListView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left

        delegate: Item {
            id: item
            width: view.width - 1
            height: 24

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

            Image {
                id: thumbnail

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.margins: 4
                width: 24
                height: 24
                source: 'file.png'
            }

            Text {
                id: text

                anchors.left: thumbnail.right
                anchors.right: sizeText.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 4
                text: model.name
            }

            Text {
                id: sizeText
        
                anchors.right: parent.right
                anchors.margins: 4
                anchors.verticalCenter: parent.verticalCenter
                text: model.size
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onDoubleClicked: {
                    crumbs.append({'url': view.model.rootUrl})
                }

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

        anchors.top: parent.top
        anchors.bottom: parent.top
        anchors.right: parent.right
        flickableItem: view
    }
}

