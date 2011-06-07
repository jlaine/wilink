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
import QXmpp 0.4

Item {
    id: block

    property alias model: view.model
    signal participantClicked(string participant)

    // FIXME: a non-static width crashes the program if chat room is empty
    // width: view.cellWidth + scrollBar.width
    width: 91

    GridView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cellWidth: 80
        cellHeight: 54

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Highlight {
                id: highlight

                anchors.fill: parent
                opacity: 0
            }

            Column {
                anchors.fill: parent
                anchors.margins: 5
                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    asynchronous: true
                    source: model.avatar
                    height: 32
                    width: 32
                 }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    color: '#2689d6'
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 10
                    text: model.name
                }
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        // hide context menu
                        menu.hide();
                        block.participantClicked(model.name);
                    } else if (mouse.button == Qt.RightButton) {
                        // show context menu
                        menu.model.clear()
                        if (model.url != undefined && model.url != '')
                            menu.model.append({
                                'action': 'profile',
                                'icon': 'diagnostics.png',
                                'text': qsTr('Show profile'),
                                'url': model.url})
                        if (room.allowedActions & QXmppMucRoom.KickAction)
                            menu.model.append({
                                'action': 'kick',
                                'icon': 'remove.png',
                                'text': qsTr('Kick user'),
                                'jid': model.jid})
                        var mousePress = mapToItem(view, mouse.x - menu.width + 16, mouse.y - 16)
                        menu.show(mousePress.x, mousePress.y);
                    }
                }
                onEntered: {
                    parent.state = 'hovered'
                }
                onExited: {
                    parent.state = ''
                }
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: highlight; opacity: 0.5 }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }

    Menu {
        id: menu
        opacity: 0
    }

    Connections {
        target: menu
        onItemClicked: {
            var item = menu.model.get(index);
            if (item.action == 'profile') {
                Qt.openUrlExternally(item.url)
            } else if (item.action == 'kick') {
                dialog.source = 'InputDialog.qml';
                dialog.item.title = qsTr('Kick user');
                dialog.item.labelText = qsTr('Enter the reason for kicking the user from the room.');
                dialog.item.accepted.connect(function() {
                    room.kick(item.jid, dialog.item.textValue);
                    dialog.item.hide();
                });
            }
        }
    }
}
