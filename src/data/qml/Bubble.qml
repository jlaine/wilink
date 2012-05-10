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
    id: bubble

    property alias text: label.text

    border.color: '#de0000'
    border.width: 1
    gradient: Gradient {
        GradientStop { position: 0.0; color: '#dea1a1' }
        GradientStop { position: 0.6; color: '#de0000' }
        GradientStop { position: 1.0; color: '#dea1a1' }
    }
    height: label.paintedHeight + 6
    width: Math.max(label.paintedWidth + 6, label.paintedHeight + 6)
    radius: 10
    smooth: true

    Label {
        id: label

        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        color: 'white'
        font.bold: true
        font.pixelSize: appStyle.font.smallSize
        verticalAlignment: Text.AlignVCenter
    }
}

