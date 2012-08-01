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

Dialog {
    id: dialog

    property alias iconStyle: icon.style
    property QtObject soundJob
    property alias text: label.text

    minimumHeight: 150

    Item {
        anchors.fill: parent

        Icon {
            id: icon

            anchors.top: parent.top
            anchors.left: parent.left
            font.pixelSize: 24
        }

        Label {
            id: label

            anchors.top: parent.top
            anchors.left: icon.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            wrapMode: Text.WordWrap
        }
    }

    onClose: {
        // stop sound
        if (Qt.isQtObject(dialog.soundJob))
            dialog.soundJob.stop();
    }

    Component.onCompleted: {
        // notify alert
        appNotifier.alert();
    }
}

