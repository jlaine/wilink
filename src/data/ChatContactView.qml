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
import wiLink 2.0

ContactView {
    id: block

    property string currentJid
    property url iconSource

    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    delegate: Item {
        id: item

        width: parent.width
        height: 30

        Item {
            anchors.fill: parent
            anchors.margins: 2

            Image {
                id: avatar

                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                asynchronous: true
                width: appStyle.icon.smallSize
                height: appStyle.icon.smallSize
                smooth: true
                source: block.iconSource != '' ? block.iconSource : model.avatar
            }

            Label {
                anchors.left: avatar.right
                anchors.right: bubble.right
                anchors.leftMargin: 3
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: model.participants > 0 ? model.name + ' (' + model.participants + ')' : model.name
            }

            Bubble {
                id: bubble

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: status.left
                anchors.margins: 3
                opacity: model.messages > 0 ? 1 : 0
                text: model.messages > 0 ? model.messages : ''
            }

            StatusPill {
                id: status
                anchors.right: parent.right
                anchors.rightMargin: 5
                anchors.verticalCenter: parent.verticalCenter
                presenceStatus: model.status
                width: 10
                height: 10
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        block.itemClicked(model);
                    } else if (mouse.button == Qt.RightButton) {
                        block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                    }
                }
            }
        }
    }
    onCurrentJidChanged: timer.restart()

    ListHelper {
        id: listHelper
        model: block.model
    }

    Timer {
        id: timer

        interval: 100

        onTriggered: {
            // Update the currently selected item after delay.
            for (var i = 0; i < listHelper.count; i++) {
                if (listHelper.getProperty(i, 'jid') == currentJid) {
                    block.currentIndex = i;
                    return;
                }
            }
            block.currentIndex = -1;
        }
    }

    Connections {
        target: block.model

        onDataChanged: timer.restart()
        onRowsInserted: timer.restart()
        onRowsRemoved: timer.restart()
    }
}
