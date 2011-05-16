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
    id: menu

    property ListModel model: ListModel {}
    signal itemClicked(int index)

    color: 'transparent'
    opacity: 0
    height: model.count * 20 + 1
    width: 100;
 
    ListView {
        id: menuList

        anchors.fill: parent
        clip: true
        model: menu.model

        delegate: Rectangle {
            id: listViewItem

            color: '#ea2689d6'
            width: menuList.width - 1
            height: 20
     
            Text {
                id: itemText

                anchors.centerIn: parent
                color: '#ffffff'
                elide: Text.ElideRight
                //font.pointSize: itemFontSize
                text: title
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: listViewItem; color: '#eae7f4fe' }
                PropertyChanges { target: itemText; color: '#496275' }
            }

            MouseArea {
                anchors.fill: listViewItem
                hoverEnabled: true

                onClicked: {
                    itemClicked(index)
                    menu.state = ''
                }

                onEntered: listViewItem.state = 'hovered'
                onExited: listViewItem.state = ''
            }
        }
    }

    states: State {
        name: 'visible'
        PropertyChanges { target: menu; opacity: 1 }
    }
}
