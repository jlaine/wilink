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
import QtWebKit 1.0
import wiLink 2.0

Panel {
    id: panel

    property variant url

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'web.png'
        title: (tabSwapper.currentItem && tabSwapper.currentItem.title) ? tabSwapper.currentItem.title : qsTr('Web')
        z: 3
    }

    ListView {
        id: tabView

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: appStyle.icon.smallSize
        model: tabSwapper.model
        orientation: ListView.Horizontal
        spacing: 2

        delegate: Rectangle {
            id: rect

            height: tabView.height
            width: 200
            color: '#9fc4f3'

            Image {
                id: icon

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: appStyle.icon.tinySize
                width: appStyle.icon.tinySize
                smooth: true
                source: 'web.png'
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
                    tabSwapper.setCurrentItem(model.panel);
                }
            }

            Button {
                id: closeButton

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconSize: appStyle.icon.tinySize
                iconSource: 'close.png'
                visible: tabView.model.count > 1
 
                onClicked: model.panel.close()
            }

            states: State {
                name: 'current'
                when: (model.panel == tabSwapper.currentItem)

                PropertyChanges {
                    target: rect
                    color: '#dfdfdf'
                }
            }
        }
    }

    PanelSwapper {
        id: tabSwapper

        anchors.top: tabView.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    Component.onCompleted: {
        tabSwapper.showPanel('WebTab.qml', {'url': 'http://www.google.com/'})
        tabSwapper.showPanel('WebTab.qml', {'url': 'https://www.wifirst.net/'})
    }
}
