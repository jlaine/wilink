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

    property ListModel model: ListModel {
        ListElement { text: 'Foo'; icon: 'remove.png' }
        ListElement { text: 'Bar'; icon: 'add.png' }
    }
    signal itemClicked(int index)

    height: 32
    width: 250

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
            //height: view.height
            width: 40
            radius: 5

            Image {
                id: icon

                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                smooth: true
                source: model.icon
                width: 16
                height: 16
            }

            Text {
                id: itemText

                anchors.top: icon.bottom
                anchors.topMargin: 4
                anchors.horizontalCenter: parent.horizontalCenter
                color: '#000000'
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
