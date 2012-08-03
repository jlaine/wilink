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

    property int animationDuration: 200
    property QtObject panelSwapper

    currentIndex: Qt.isQtObject(panelSwapper) ? panelSwapper.currentIndex : -1
    height: (appStyle.font.largeSize + 2 * appStyle.margin.large)
    highlightMoveDuration: appStyle.highlightMoveDuration
    model: panelSwapper.model
    orientation: ListView.Horizontal
    spacing: 2

    delegate: Item {
        id: rect

        height: tabView.height
        width: 200
        z: model.panel.z
        opacity: 0.3

        Rectangle {
            id: frame

            anchors.fill: parent
            border.color: 'white'
            border.width: 1
            opacity: 0
            radius: 0
            height: tabView.height
            color: 'white'

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
            height: appStyle.icon.normalSize
            width: appStyle.icon.normalSize

            Behavior on height {
                NumberAnimation { duration: animationDuration }
            }
            Behavior on width {
                NumberAnimation { duration: animationDuration }
            }

            StatusPill {
                id: statusPill

                property int iconOffset: -width * 0.25

                anchors.right: parent.right
                anchors.rightMargin: iconOffset
                anchors.bottom: parent.bottom
                anchors.bottomMargin: iconOffset
                height: appStyle.icon.tinySize
                width: appStyle.icon.tinySize
                opacity: 0
                presenceStatus: model.panel.presenceStatus

                Behavior on opacity {
                    NumberAnimation { duration: animationDuration }
                }
            }
        }

        SocialIcon {
            id: socialIcon

            email: model.panel.subTitle
            anchors.left: icon.right
            anchors.margins: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            anchors.left: socialIcon.right
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
            visible: tabView.model.count > 1
            width: iconSize
            opacity: (closeButton.state == "hovered") || (closeButton.state == "pressed") ? 1 : 0.5
            onClicked: model.panel.close()

        }


        states: State {
            name: 'current'
            when: (model.index == tabView.currentIndex)

            PropertyChanges { target: statusPill; opacity: 1 }
            PropertyChanges { target: rect; opacity: 1 }
            PropertyChanges { target: frame; opacity: 1 }

        }
    }
}
