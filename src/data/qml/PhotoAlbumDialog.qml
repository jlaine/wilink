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

    property QtObject model

    minimumHeight: 150
    title: qsTr('Create an album')

    Item {
        anchors.fill: parent

        Label {
            id: label

            anchors.top: parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            text: qsTr('Album name:')
            wrapMode: Text.WordWrap
        }

        InputBar {
            id: bar

            anchors.top: label.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            focus: true
        }
    }

    onAccepted: {
        var name = bar.text;
        if (name.length > 0) {
            dialog.model.createAlbum(name);
            dialog.close();
        }
    }
}

