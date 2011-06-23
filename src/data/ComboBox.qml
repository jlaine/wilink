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

    property variant model
    property int currentIndex: -1

    height: 21

    Component {
        id: comboMenu

        Menu {
            model: block.model
            width: view.width

            onItemClicked: {
                block.currentIndex = index;
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.rightMargin: 1
        anchors.bottomMargin: 1
        border.width: 1
        border.color: '#ffffff'
        color: '#567dbc'
        radius: 3
        smooth: true
    }

    Image {
        id: eject

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        source: 'eject.png'
        z: 1
    }

    MenuDelegate {
        id: view

        property variant model

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: eject.right
        anchors.rightMargin: 16
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            var pos = view.mapToItem(menuLoader.parent, 0, 0);
            menuLoader.sourceComponent = comboMenu;
            menuLoader.show(pos.x, pos.y);
        }
    }

    onCurrentIndexChanged: {
        view.model = block.model.get(currentIndex);
    }
}
