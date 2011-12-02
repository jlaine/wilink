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

Item {
    id: checkBox

    signal clicked
    property bool checked: false
    property bool enabled: true
    property string iconSource
    property string text

    height: Math.max(16, textItem.paintedHeight)

    Image {
        id: rect

        anchors.left:  parent.left
        anchors.verticalCenter: parent.verticalCenter
        source: checked ? '16x16/checkbox-checked.png' : '16x16/checkbox.png'
        width: 16
        height: 16
    }

    Image {
        id: image

        property bool collapsed: source == ''

        anchors.left: rect.right
        anchors.leftMargin: collapsed ? 0 : appStyle.spacing.horizontal
        anchors.top: parent.top
        height: collapsed ? 0 : appStyle.icon.smallSize
        width: collapsed ? 0 : appStyle.icon.smallSize
        source: checkBox.iconSource
    }

    Label {
        id: textItem

        anchors.left: image.right
        anchors.leftMargin: appStyle.spacing.horizontal
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        text: checkBox.text
    }

    MouseArea {
        anchors.fill: parent
        enabled: checkBox.enabled
        onClicked: checkBox.clicked()
    }
}
