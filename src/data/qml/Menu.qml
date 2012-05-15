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

Rectangle {
    id: menu

    property alias delegate: repeater.delegate
    property ListModel model: ListModel {}
    signal itemClicked(int index)

    border.color: '#7997c1'
    border.width: 1
    color: '#b0c4de'
    opacity: 0
    radius: 5
    height: model.count * (appStyle.icon.tinySize + 4) + 1
    width: 150
    z: 10

    // FIXME: this is a hack waiting 'blur' or 'shadow' attribute in qml
    BorderImage {
        id: shadow
        anchors.fill: menu
        anchors { leftMargin: -5; topMargin: -5; rightMargin: -8; bottomMargin: -9 }
        border { left: 10; top: 10; right: 10; bottom: 10 }
        source: 'image://icon/shadow'
        smooth: true
        z: -1
    }

    Column {
        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 1

        Repeater {
            id: repeater
            model: menu.model

            delegate: MenuDelegate {
                onClicked: itemClicked(index)
            }
        }
    }
}
