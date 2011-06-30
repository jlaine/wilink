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

Item {
    id: header

    property alias toolBar: toolBarLoader.sourceComponent
    property alias iconSource: iconImage.source
    property alias title: titleText.text
    property alias subTitle: subTitleText.text

    signal itemClicked(int index)

    height: 46
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
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        smooth: true
        height: appStyle.icon.normalSize
        width: appStyle.icon.normalSize
    }

    Column {
        anchors.left: iconImage.right
        anchors.leftMargin: 8
        anchors.right: toolBarLoader.left
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.right: parent.right
            color: 'white'
            elide: Text.ElideRight
            font.bold: true
        }

        Text {
            id: subTitleText

            anchors.left: parent.left
            anchors.right: parent.right
            color: 'white'
            elide: Text.ElideRight
/*
            states: State {
                name: 'collapsed'
                when: subTitleText.text.length === 0
                PropertyChanges { target: subTitleText; height: 0 }
            }
*/
        }
    }

    Loader {
        id: toolBarLoader

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
    }
}

