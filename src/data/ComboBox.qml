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
    id: block

    property alias model: view.model
    signal elementClicked(string name)

    border.width: 1
    border.color: '#ffffff'
    color: '#567dbc'
    height: 25
    radius: 3
    smooth: true

    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 5
        color: 'white'
        opacity: block.state == 'expanded' ? 0 : 1
        text: '<html>&#9167;</html>'
        z: 1
    }

    ListView {
        id: view

        anchors.fill: parent
        interactive: false
        clip: true

        delegate: Item {
            id: item

            height: 25
            width: parent.width

            Rectangle {
                id: background

                anchors.fill: parent
                border.width: 1
                border.color: 'transparent'
                color: 'transparent'
                radius: block.radius
                smooth: block.smooth
            }

            StatusPill {
                id: statusPill

                anchors.left:  parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 5
                height: 16
                width: 16
                presenceStatus: model.status
            }

            Text {
                id: text

                anchors.left: statusPill.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 5
                color: 'white'
                text: model.name
            }

            MouseArea {
                property variant positionPressed
                property int pressedIndex

                anchors.fill: parent
                hoverEnabled: true

                onEntered: {
                    item.state = 'hovered'
                }
                onExited: {
                    item.state = ''
                }

                onPressed: {
                    if (block.state != 'expanded') {
                        block.state = 'expanded'
                    } else {
                        positionPressed = mapToItem(view, mouse.x, mouse.y)
                        pressedIndex = view.indexAt(positionPressed.x, positionPressed.y)
                        block.state = ''
                        view.positionViewAtIndex(pressedIndex, ListView.Beginning)
                        elementClicked(model.name)
                    }
                }
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: background; color: '#90acd8' }
            }
        }
    }

    states: State {
        name: 'expanded'
        PropertyChanges {
            target: block
            height: 25 * view.count
        }
    }
}
