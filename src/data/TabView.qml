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

ListView {
    id: tabView

    property QtObject panelSwapper

    currentIndex: Qt.isQtObject(panelSwapper) ? panelSwapper.currentIndex : -1
    height: appStyle.icon.smallSize
    highlightMoveDuration: appStyle.highlightMoveDuration
    model: panelSwapper.model
    orientation: ListView.Horizontal
    spacing: -8

    delegate: BorderImage {
        id: rect

        border { left: 32; right: 32 }
        height: tabView.height
        width: 200
        source: 'image://icon/tab'
        z: model.panel.z

        Image {
            id: icon

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
            smooth: true
            source: model.panel.iconSource
        }

        Label {
            anchors.margins: appStyle.margin.normal
            anchors.left: icon.right
            anchors.right: closeButton.left
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: model.panel.title
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                panelSwapper.currentIndex = model.index;
            }
        }

        ToolButton {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            iconSize: appStyle.icon.tinySize
            iconSource: 'image://icon/close'
            width: iconSize
            visible: tabView.model.count > 1

            onClicked: model.panel.close()
        }

        states: State {
            name: 'current'
            when: (model.index == tabView.currentIndex)

            PropertyChanges {
                target: rect
                source: 'image://icon/tab-active'
            }
        }
    }

    Rectangle {
        id: background

        anchors.margins: -1
        anchors.bottomMargin: 0.5
        anchors.fill: parent
        border.color: '#8499bd'
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: '#9bbdf4' }
            GradientStop { position: 1; color: '#90acd8' }
        }
        z: -1
    }
}
