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
import wiLink 1.2
import 'utils.js' as Utils

FocusScope {
    id: root
    focus: true

    Item {
        id: appStyle

        property alias font: fontItem

        opacity: 0

        Item {
            id: fontItem

            property int normalSize: textItem.font.pixelSize
            property int smallSize: Math.ceil((normalSize * 11) / 13)
        }

        Text {
            id: textItem
        }
    }

    Dock {
        id: dock

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 1
    }

    PanelSwapper {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: dock.right
        anchors.right: parent.right
        focus: true
    }

    Loader {
        id: dialogLoader

        anchors.centerIn: parent
        z: 10

        function hide() {
            dialogLoader.item.opacity = 0;
            swapper.focus = true;
        }

        function show() {
            dialogLoader.item.opacity = 1;
            dialogLoader.focus = true;
        }

        Connections {
            target: dialogLoader.item
            onRejected: dialogLoader.hide()
        }
    }

    Loader {
        id: menuLoader

        z: 11

        function hide() {
            menuLoader.item.opacity = 0;
        }

        function show(x, y) {
            menuLoader.x = x;
            menuLoader.y = y;
            menuLoader.item.opacity = 1;
        }

        Connections {
            target: menuLoader.item
            onItemClicked: menuLoader.hide()
        }
    }

    Component.onCompleted: swapper.showPanel('ChatPanel.qml')
    Keys.forwardTo: dock
}
