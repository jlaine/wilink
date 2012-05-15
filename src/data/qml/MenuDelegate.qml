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
    id: menuItem

    signal clicked
    property bool enabled: model.enabled != false

    color: 'transparent'
    height: appStyle.icon.tinySize + 4
    width: parent.width
    radius: 5

    Image {
        id: icon

        anchors.left: parent.left
        anchors.leftMargin: model.iconSource ? 4 : 0
        anchors.verticalCenter: parent.verticalCenter
        opacity: menuItem.enabled ? 1 : 0.5
        source: model.iconSource ? model.iconSource : ''
        sourceSize.width: appStyle.icon.tinySize
        sourceSize.height: appStyle.icon.tinySize
    }

    Label {
        id: itemText

        anchors.left: icon.right
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: menuItem.enabled ? 'black' : '#888'
        elide: Text.ElideRight
        text: model.text
    }

    states: State {
        name: 'hovered'
        PropertyChanges { target: menuItem; color: '#eae7f4fe' }
    }

    MouseArea {
        anchors.fill: parent
        enabled: menuItem.enabled
        hoverEnabled: true

        onClicked: menuItem.clicked()
        onEntered: menuItem.state = 'hovered'
        onExited: menuItem.state = ''
    }
}
