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

Dialog {
    id: dialog
    title: qsTr("Preferences")

    Item {
        anchors.fill: contents
        anchors.margins: 6

        ListView {
            id: tabList
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 100

            model: ListModel {
                ListElement {
                    avatar: 'options.png'
                    name: 'General'
                }
                ListElement {
                    avatar: 'audio-output.png'
                    name: 'Sound'
                }
                ListElement {
                    avatar: 'share.png'
                    name: 'Shares'
                }
            }
            delegate: Row {
                spacing: 4
                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: model.avatar
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.name
                }
            }
        }

        Text {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: tabList.left
            anchors.right: parent.right
            text: application.applicationName
        }
        
    }

}

