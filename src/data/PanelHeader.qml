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
    id: header

    property alias iconSource: iconImage.source
    property alias title: titleText.text

    signal itemClicked(int index)

    gradient: Gradient {
        GradientStop { position: 0; color: '#9bbdf4' }
        GradientStop { position: 1; color: '#90acd8' }
    }
    height: 46
    width: 200

    Image {
        id: iconImage

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        smooth: true
        height: 32
        width: 32
    }

    Text {
        id: titleText

        anchors.left: iconImage.right
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        color: 'white'
        width: 150
    }
}

