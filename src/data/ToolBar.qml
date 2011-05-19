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
    id: toolbar
    color: 'transparent'

    property ListModel model: ListModel {
        ListElement { text: 'Phone'; icon: 'phone.png' }
        ListElement { text: 'Photos'; icon: 'photos.png' }
        ListElement { text: 'Shares'; icon: 'share.png' }
    }
    signal itemClicked(int index)

    ListView {
        id: view

        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 1
        clip: true
        model: toolbar.model
        orientation: ListView.Horizontal

        delegate: Rectangle {
            id: listViewItem

            color: 'transparent'
            height: 45
            width: itemText.paintedWidth + 24
            radius: 5

            Image {
                id: icon

                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                source: model.icon
                width: 32
                height: 32
            }

            Text {
                id: itemText

                anchors.top: icon.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                color: 'white'
                font.pixelSize: 10
                text: model.text
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: listViewItem; color: '#eae7f4fe' }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onClicked: itemClicked(index)
                onEntered: listViewItem.state = 'hovered'
                onExited: listViewItem.state = ''
            }
        }
    }
}
