/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import wiLink 2.5

ContactView {
    id: block

    property string currentJid
    property int rowHeight: appStyle.icon.smallSize + 6

    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    title: qsTr('My rooms')
    delegate: Item {
        id: item

        width: parent.width
        height: block.rowHeight

        Item {
            anchors.fill: parent

            Rectangle {
                id: avatarFrame

                anchors.left: parent.left
                anchors.leftMargin: 15
                anchors.verticalCenter: parent.verticalCenter
                width: appStyle.icon.smallSize
                height: appStyle.icon.smallSize
                border.color: '#c7c7c7'
                border.width: 1

                Image {
                    id: avatar

                    anchors.topMargin: 1
                    anchors.top: parent.top
                    anchors.leftMargin: 1
                    anchors.left: parent.left
                    width: parent.width - 1
                    height: parent.height - 1
                    asynchronous: true
                    source: 'images/chat.png'
                }

            }

            Label {
                anchors.left: avatarFrame.right
                anchors.leftMargin: 6
                anchors.right: bubble.right
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: model.participants > 0 ? model.name + ' (' + model.participants + ')' : model.name
                font.pixelSize: appStyle.font.tinySize
                color: item.ListView.isCurrentItem ? 'white' : '#c7c7c7'
            }

            Bubble {
                id: bubble

                anchors.right: parent.right
                anchors.rightMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 3
                opacity: model.messages > 0 ? 1 : 0
                text: model.messages > 0 ? model.messages : ''
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button === Qt.LeftButton) {
                        block.itemClicked(model);
                    } else if (mouse.button === Qt.RightButton) {
                        block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                    }
                }
            }
        }
    }
    onCurrentJidChanged: timer.restart()

    resources: [
        Timer {
            id: timer

            interval: 100

            onTriggered: {
                // Update the currently selected item after delay.
                for (var i = 0; i < block.model.count; i++) {
                    if (block.model.get(i).jid === currentJid) {
                        block.currentIndex = i;
                        return;
                    }
                }
                block.currentIndex = -1;
            }
        },

        Connections {
            target: block.model

            onCountChanged: timer.restart()
        }
    ]
}
