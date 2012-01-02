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

import QtQuick 1.1

FocusScope {
    id: box

    property alias contents: item
    property alias header: textItem
    property string title

    Rectangle {
        id: frame

        anchors.fill: parent
        border.width: 1
        border.color: '#7091c8'
        color: '#cbd9f0'
        radius: appStyle.margin.large
        smooth: true
    }

    Rectangle {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        border.color: '#7091c8'
        border.width: 1
        color: '#9fb7dd'
        height: textItem.paintedHeight
        radius: appStyle.margin.large
        smooth: true
    }

    Label {
        id: textItem

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        text: box.title    
    }

    Item {
        id: item

        anchors.top: textItem.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: appStyle.margin.large
    }
}
