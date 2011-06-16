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

Rectangle {
    id: menu

    property ListModel model: ListModel {}
    signal itemClicked(int index)

    border.color: '#7997c1'
    border.width: 1
    color: '#b0c4de'
    opacity: 0
    radius: 5
    height: model.count * 20 + 1
    width: 150
    z: 10

    // FIXME: this is a hack waiting 'blur' or 'shadow' attribute in qml
    BorderImage {
        id: shadow
        anchors.fill: menu
        anchors { leftMargin: -5; topMargin: -5; rightMargin: -8; bottomMargin: -9 }
        border { left: 10; top: 10; right: 10; bottom: 10 }
        source: 'shadow.png'
        smooth: true
        z: -1
    }

    ListView {
        id: menuList

        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 1
        clip: true
        model: menu.model

        delegate: Rectangle {
            id: menuItem

            property bool enabled: model.enabled != false

            color: 'transparent'
            height: 20
            width: menuList.width
            radius: menu.radius

            Image {
                id: icon

                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                opacity: menuItem.enabled ? 1 : 0.5
                smooth: true
                source: model.icon ? model.icon : ''
                width: 16
                height: 16
            }

            Text {
                id: itemText

                anchors.left: icon.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                color: menuItem.enabled ? 'black' : '#888'
                elide: Text.ElideRight
                text: model.text
                width: parent.width
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: menuItem; color: '#eae7f4fe' }
            }

            MouseArea {
                anchors.fill: parent
                enabled: menuItem.enabled
                hoverEnabled: true

                onClicked: itemClicked(index)
                onEntered: menuItem.state = 'hovered'
                onExited: menuItem.state = ''
            }
        }
    }
}
