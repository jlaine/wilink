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

FocusScope {
    id: box

    default property alias content: item.children
    property alias header: textItem
    property string title

    Rectangle {
        id: frame

        anchors.fill: parent
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.05)
        color: '#F5F5F5'
        radius: appStyle.margin.small
        smooth: true
    }

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: appStyle.font.normalSize + 2 * appStyle.margin.normal

        Label {
            id: textItem

            anchors.centerIn: parent
            font.bold: true
            text: box.title
        }
    }

    Item {
        id: item

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: appStyle.margin.normal
    }
}
