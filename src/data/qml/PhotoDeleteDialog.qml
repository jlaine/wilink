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
import wiLink 2.0

Dialog {
    id: dialog

    property url avatar
    property int index: -1
    property variant model
    property string name

    minimumHeight: 150
    title: qsTr('Delete photo');

    Item {
        anchors.fill: parent

        Image {
            id: image

            anchors.top: parent.top
            anchors.left: parent.left
            fillMode: Image.PreserveAspectFit
            width: 64
            height: 64
            smooth: true
            source: dialog.avatar
        }

        Label {
            anchors.top: parent.top
            anchors.left: image.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            text: qsTr('Do you want to delete %1 from your photos?').replace('%1', '<b>' + dialog.name + '</b>')
            wrapMode: Text.WordWrap
        }
    }

    onAccepted: {
        model.remove(index);
        dialog.close();
    }
}

