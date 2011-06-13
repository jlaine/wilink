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

Item {
    id: root
    focus: true

    Item {
        id: appStyle

        property alias font: fontItem

        Item {
            id: fontItem

            property int normalSize: textItem.font.pixelSize
            property int smallSize: Math.round((normalSize * 11) / 13)
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
    }

    Loader {
        id: dialog

        focus: dialog.item ? dialog.item.visible : false
        x: 100
        y: 100
        z: 10
    }

    Component.onCompleted: swapper.showPanel('ChatPanel.qml')
    Keys.forwardTo: dock
}
