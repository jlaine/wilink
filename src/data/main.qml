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

    Style {
        id: appStyle
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

    MouseArea {
        id: cancelArea

        anchors.fill: parent
        enabled: false

        onClicked: menuLoader.hide()
    }

    Loader {
        id: dialogLoader

        z: 10

        function hide() {
            dialogLoader.item.opacity = 0;
            swapper.focus = true;
        }

        function show() {
            dialogLoader.item.opacity = 1;
            dialogLoader.focus = true;
        }

        onLoaded: {
            x = Math.floor((parent.width - width) / 2);
            y = Math.floor((parent.height - height) / 2);
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
            cancelArea.enabled = false;
            menuLoader.item.opacity = 0;
        }

        function show(x, y) {
            cancelArea.enabled = true;
            menuLoader.x = x;
            menuLoader.y = y;
            menuLoader.item.opacity = 1;
        }

        Connections {
            target: menuLoader.item
            onItemClicked: {
                console.log("clicked");
                menuLoader.hide()
            }
        }
    }

    Component.onCompleted: swapper.showPanel('ChatPanel.qml')
    Keys.forwardTo: dock
}
