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

    minimumWidth: 280
    minimumHeight: 150

    ListView {
        id: view

        anchors.fill: contents
        clip: true

        model: ListModel {
            ListElement { nickName: 'User1' }
            ListElement { nickName: 'User2' }
            ListElement { nickName: 'User3' }
            ListElement { nickName: 'User4' }
        }

        delegate: Item {
            id: item

            width: parent.width
            height: text.paintedHeight

            Text {
                id: text

                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                text: model.nickName
                verticalAlignment: Text.AlignVCenter
            }

            ComboBox {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                currentIndex: 1
                model: ListModel {
                    ListElement { text: 'Owner' }
                    ListElement { text: 'Administrator' }
                    ListElement { text: 'Member' }
                    ListElement { text: 'None' }
                }
            }
        }
    }
}
