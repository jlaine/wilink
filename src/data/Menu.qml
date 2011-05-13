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

    property ListModel model: ListModel {
            ListElement {name: "Red Item 0"}
            ListElement {name: "Red Item 1"}
    }

    color: "#336699"
    width: 100; height: model.count * 20
 
    ListView {
        id: menuList

        anchors.fill: parent
        highlight: Rectangle { color: "lightsteelblue" }
        clip: true
        model: menu.model
        delegate: Rectangle {
            id: listViewItem
     
            color: "transparent"
            border { width: 1; color: "black" }
            width: menuList.width - 1
            height: 20
     
            Text {
                anchors.centerIn: parent
                id: itemText
                text: name
                //font.pointSize: itemFontSize
                elide: Text.ElideRight
            }
        }
    }
}
