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

Dialog {
    id: dialog

    title: qsTr('About %1').replace('%1', application.applicationName)

    Item {
        anchors.fill: dialog.contents

        Image {
            id: appIcon
            anchors.left: parent.left
            source: 'wiLink-64.png'
        }

        Text {
            id: appName

            anchors.left: appIcon.right
            anchors.top: appIcon.top
            anchors.margins: 6
            font.bold: true
            font.pixelSize: appStyle.font.largeSize
            text: application.applicationName
        }

        Text {
            anchors.left: appIcon.right
            anchors.top: appName.bottom
            anchors.margins: 6
            text: qsTr('version %1').replace('%1', application.applicationVersion)
        }
    }
}

