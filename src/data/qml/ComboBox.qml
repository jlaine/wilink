/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

import QtQuick 1.1

Item {
    id: block

    property alias delegate: view.sourceComponent
    property variant model
    property int currentIndex: -1

    height: appStyle.icon.smallSize

    Component {
        id: comboMenu

        Menu {
            delegate: block.delegate
            model: block.model
            width: view.width

            onItemClicked: {
                block.currentIndex = index;
            }
        }
    }

    QtObject {
        id: emptyItem

        property string text: ''
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

    Loader {
        id: view

        sourceComponent: MenuDelegate {
            onClicked: itemClicked(index)
        }

        property QtObject model: emptyItem

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: eject.left
        anchors.rightMargin: 8
    }

    Icon {
        id: eject

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        style: 'icon-eject'
        z: 1
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
        if (currentIndex >= 0 && currentIndex < block.model.count) {
            view.model = block.model.get(currentIndex);
        } else {
            view.model = emptyItem;
        }
    }
}
