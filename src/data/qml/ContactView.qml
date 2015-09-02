/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import wiLink 2.4

FocusScope {
    id: block

    property alias count: view.count
    property alias currentIndex: view.currentIndex
    property alias delegate: view.delegate
    property alias headerHeight: header.height
    property alias model: view.model
    property alias moving: view.moving
    property string title

    signal addClicked

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.max(label.height, button.height) + appStyle.margin.normal

        Label {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: appStyle.margin.large
            anchors.right: button.left
            color: '#858585'
            font.bold: true
            font.capitalization: Font.AllUppercase
            style:Text.Sunken
            horizontalAlignment: Text.AlignLeft
            text: block.title
        }

        ChatToolButton {
            id: button

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: appStyle.margin.normal
            iconStyle: 'icon-plus'

            onClicked: block.addClicked()
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        clip: true
        focus: true
        highlight: Highlight {
            width: view.width
        }
        highlightMoveDuration: appStyle.highlightMoveDuration
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}
