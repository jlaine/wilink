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
    signal addressClicked(string address)

    Rectangle {
        id: header
        anchors.left: view.left
        anchors.right: scrollBar.right
        color: '#7091c8'
        height: textHeader.height

        Text {
            id: textHeader
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 4
            color: '#ffffff'
            font.bold: true
            text: qsTr('Call history')
        }
    }

    ListView {
        id: view

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: header.bottom
        anchors.bottomMargin: 4
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        clip: true

        delegate: Item {
            id: item
            width: view.width
            height: 32

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: '#ffffff'
                color: model.active ? '#dd6666' : 'white'

                Image {
                    id: image

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    source: model.direction == QXmppCall.OutgoingDirection ? 'call-outgoing.png' : 'call-incoming.png'
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: image.right
                    anchors.right: date.left
                    elide: Text.ElideRight
                    text: model.name
                }

                Text {
                    id: date

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: duration.left
                    text: Qt.formatDateTime(model.date)
                    width: 150
                }

                Text {
                    id: duration

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: model.duration + 's'
                    width: 60
                }
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        menu.hide();
                        block.addressClicked(model.address);
                    } else if (mouse.button == Qt.RightButton) {
                        if (model.active) {
                            menu.hide();
                            return;
                        }

                        // show context menu
                        menu.model.clear();
                        if (!view.model.currentCalls)
                            menu.model.append({
                                'action': 'call',
                                'icon': 'call.png',
                                'text': qsTr('Call'),
                                'address': model.address});
                        menu.model.append({
                            'action': 'remove',
                            'icon': 'remove.png',
                            'text': qsTr('Remove'),
                            'id': model.id});

                        var point = mapToItem(block, mouse.x, mouse.y);
                        menu.show(point.x, point.y);
                    }
                }
                onDoubleClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        menu.hide();
                        view.model.call(address)
                    }
                }
            }
        }
    }

    Menu {
        id: menu
        opacity: 0

        onItemClicked: {
            var item = menu.model.get(index);
            if (item.action == 'call') {
                view.model.call(item.address)
            } else if (item.action == 'remove') {
                console.log("remove " + item.id);
            }
        }

    }

    ScrollBar {
        id: scrollBar

        anchors.top: view.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}
