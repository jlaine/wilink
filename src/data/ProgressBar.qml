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
    property variant orientation: Qt.Horizontal
    property real minimumValue: 0
    property real maximumValue: 100
    property real value: 0

    width: orientation == Qt.Horizontal ? 100 : 16
    height: orientation == Qt.Horizontal ? 16 : 100
    border.color: '#4a9ddd'
    color: '#e7f4fe'
    radius: 4
    smooth: true

    Rectangle {
        property real position: (value - minimumValue) / (maximumValue - minimumValue)

        color: '#4a9ddd'
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        radius: 4
        smooth: true
        width: orientation == Qt.Horizontal ? parent.width * position : parent.width
        height: orientation == Qt.Horizontal ? parent.height : parent.height * position
    }
}

