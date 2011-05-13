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

GridView {
    id: grid

    signal participantClicked(string participant)

    cellWidth: 80
    cellHeight: 44
    width: cellWidth

    delegate: Item {
        id: item
        width: grid.cellWidth
        height: grid.cellHeight

        Column {
            anchors.fill: parent
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                asynchronous: true
                source: model.avatar
                height: 32
                width: 32
             }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                color: '#2689d6' 
                font.pixelSize: 10
                text: model.name
            }
        }

        MouseArea {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: parent
            onClicked: {
                if (mouse.button == Qt.LeftButton) {
                    grid.participantClicked(model.name);
                    menu.opacity = 0;
                } else if (mouse.button == Qt.RightButton) {
                    menu.x = item.x + mouse.x - menu.width + 16;
                    menu.y = item.y + mouse.y - 16;
                    menu.opacity = 0.9;
                }
            }
        }
    }

    Menu {
        id: menu
        opacity: 0
    }

}
