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
import QXmpp 0.4
import wiLink 1.2

Rectangle {
    width: 320
    height: 400

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        icon: 'diagnostics.png'
        title: qsTr('Service discovery')
        Row {
            id: toolBar

            anchors.top: parent.top
            anchors.right: parent.right

            ToolButton {
                icon: 'back.png'
                text: qsTr('Go back')
            }

            ToolButton {
                icon: 'refresh.png'
                text: qsTr('Refresh')
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
            }
        }
    }

    ListView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom

        model: DiscoveryModel {
            manager: client.discoveryManager
        }
    }
}

