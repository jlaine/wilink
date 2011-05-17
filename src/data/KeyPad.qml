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

    cellWidth: 40
    cellHeight: 40
    interactive: false
    width: cellWidth * 3
    height: cellHeight * 4

    delegate: Rectangle {
        id: button

        border.width: 1
        border.color: '#adacab'
        gradient: Gradient {
            GradientStop { id: stop1; position: 0.0; color: '#ffffff' }
            GradientStop { id: stop2; position: 1.0; color: '#edeceb' }
        }

        color: '#edeceb'
        width: grid.cellWidth - 6
        height: grid.cellHeight - 6
        radius: 5
        smooth: true

        Text  {
            anchors.centerIn: parent
            text: model.name
        }

        MouseArea {
            anchors.fill: parent

            onPressed: {
                button.state = 'pressed'
            }

            onReleased: {
                button.state = ''
            }
        }

        states: State {
            name: 'pressed'
            PropertyChanges { target: button; border.color: '#b0e2ff' }
            PropertyChanges { target: stop2;  color: '#b0e2ff' }
            PropertyChanges { target: stop1;  color: '#ffffff' }
        }
    }
    model: ListModel {
        ListElement { name: '1' }
        ListElement { name: '2' }
        ListElement { name: '3' }
        ListElement { name: '4' }
        ListElement { name: '5' }
        ListElement { name: '6' }
        ListElement { name: '7' }
        ListElement { name: '8' }
        ListElement { name: '9' }
        ListElement { name: '*' }
        ListElement { name: '0' }
        ListElement { name: '#' }
    }
}

