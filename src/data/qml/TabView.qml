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
import wiLink 2.4

ListView {
    id: tabView

    property QtObject panelSwapper

    clip: true
    currentIndex: Qt.isQtObject(panelSwapper) ? panelSwapper.currentIndex : -1
    height: appStyle.icon.smallSize
    highlightMoveDuration: appStyle.highlightMoveDuration
    model: panelSwapper.model
    orientation: ListView.Horizontal
    spacing: 2

    delegate: Item {
        id: rect

        height: tabView.height
        width: 200
        z: model.panel.z

        Rectangle {
            id: frame

            anchors.fill: parent
            anchors.bottomMargin: -radius
            border.color: '#DDDDDD'
            border.width: 1
            opacity: 0
            radius: 4
            height: tabView.height

            Behavior on opacity {
                PropertyAnimation { duration: appStyle.animation.normalDuration }
            }
        }

        Image {
            id: icon

            anchors.left: parent.left
            anchors.leftMargin: appStyle.margin.large
            anchors.verticalCenter: parent.verticalCenter
            smooth: true
            source: model.panel.iconSource
            sourceSize.height: appStyle.icon.tinySize
            sourceSize.width: appStyle.icon.tinySize
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
            id: mouseArea

            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                panelSwapper.currentIndex = model.index;
            }
        }

        ToolButton {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: appStyle.margin.large
            anchors.verticalCenter: parent.verticalCenter
            iconStyle: 'icon-remove'
            width: iconSize
            visible: tabView.model.count > 1

            onClicked: model.panel.close()
        }

        states: State {
            name: 'current'
            when: (model.index == tabView.currentIndex)

            PropertyChanges { target: frame; opacity: 1 }
        }
    }
}
