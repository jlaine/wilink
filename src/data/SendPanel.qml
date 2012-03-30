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

Item {
    width: 600
    height: 100

    Style {
        id: appStyle
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#888'
    }

    ToolBar {
        anchors.centerIn: parent
        spacing: appStyle.icon.normalSize

        ToolButton {
            iconSize: 64
            iconSource: '64x64/file.png'
            height: appStyle.icon.normalSize
            width: 96
            text: qsTr('Send a file')
        }

        ToolButton {
            iconSize: 64
            iconSource: '64x64/photos.png'
            height: appStyle.icon.normalSize
            width: 96
            text: qsTr('Send a photo')
        }
    }
}
