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
import wiLink 2.0

Item {
    id: block

    property alias model: view.model
    signal addressClicked(string address)

    ListHelper {
        id: listHelper
        model: view.model
    }

    Rectangle {
        id: background
        anchors.fill: parent
        border.color: '#88a4d1'
        border.width: 1
        color: '#ffffff'
        smooth: true
    }

    Rectangle {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        border.color: '#88a4d1'
        border.width: 1
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        height: textHeader.height
        z: 1

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
        anchors.margins: 4
        clip: true

        delegate: Item {
            id: item
            width: view.width
            height: 32

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: 'transparent'
                color: model.active ? '#dd6666' : 'transparent'

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
                    view.currentIndex = model.index;
                    if (mouse.button == Qt.LeftButton) {
                        block.addressClicked(model.address);
                    } else if (mouse.button == Qt.RightButton) {
                        if (!model.active) {
                            // show context menu
                            var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
                            menuLoader.sourceComponent = phoneHistoryMenu;
                            menuLoader.item.callAddress = model.address;
                            menuLoader.item.callId = model.id;
                            menuLoader.item.callPhone = model.phone;
                            menuLoader.show(pos.x, pos.y);
                        }
                    }
                }
                onDoubleClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        view.model.call(address);
                    }
                }
            }
        }

        highlight: Highlight {}
        highlightMoveDuration: 500
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }

    Component {
        id: phoneHistoryMenu

        Menu {
            id: menu

            property string callAddress
            property int callId
            property string callPhone

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'call') {
                    view.model.call(callAddress)
                } else if (item.action == 'contact') {
                    var contact = addressbook.contactForPhone(callPhone);
                    if (contact != null) {
                        dialogSwapper.showPanel('PhoneContactDialog.qml', {
                            'contactId': contact.id,
                            'contactName': contact.name,
                            'contactPhone': contact.phone,
                            'model': view.model.contactsModel});
                    } else {
                        dialogSwapper.showPanel('PhoneContactDialog.qml', {
                            'contactPhone': callPhone,
                            'model': view.model.contactsModel});
                    }
                } else if (item.action == 'remove') {
                    view.model.removeCall(callId);
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'call',
                    'icon': 'call.png',
                    'text': qsTr('Call')});
                menu.model.append({
                    'action': 'contact',
                    'icon': 'peer.png',
                    'text': qsTr('Modify contact')});
                menu.model.append({
                    'action': 'remove',
                    'icon': 'remove.png',
                    'text': qsTr('Remove')});
            }
        }
    }
}
