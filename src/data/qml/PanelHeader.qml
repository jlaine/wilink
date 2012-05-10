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
    id: header

    property alias toolBar: toolBarLoader.sourceComponent
    property int iconMargin: 8
    property int iconSize: appStyle.icon.normalSize
    property alias iconSource: iconImage.source
    property alias title: titleText.text
    property alias subTitle: subTitleText.text

    signal itemClicked(int index)

    clip: true
    height: appStyle.icon.normalSize + 2 * iconMargin
    z: 1

    Rectangle {
        id: background

        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: '#9bbdf4' }
            GradientStop { position: 1; color: '#90acd8' }
        }
    }

    Image {
        id: iconImage

        anchors.left: parent.left
        anchors.leftMargin: iconMargin
        anchors.verticalCenter: parent.verticalCenter
        sourceSize.height: iconSize
        sourceSize.width: iconSize

        states: State {
            name: 'collapsed'
            when: header.width < 400
            PropertyChanges { target: iconImage; opacity: 0 }
            PropertyChanges { target: column; anchors.left: parent.left }
        }
    }

    Column {
        id: column
        anchors.left: iconImage.right
        anchors.leftMargin: iconMargin
        anchors.right: toolBarLoader.left
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: titleText

            anchors.left: parent.left
            anchors.right: parent.right
            color: 'white'
            elide: Text.ElideRight
            font.bold: true
        }

        Label {
            id: subTitleText

            anchors.left: parent.left
            anchors.right: parent.right
            color: 'white'
            elide: Text.ElideRight
            visible: text != ''
        }
    }

    Loader {
        id: toolBarLoader

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
}

