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
import 'utils.js' as Utils

FocusScope {
    id: block

    property alias model: historyView.model
    signal addressClicked(string address)

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

        Label {
            id: textHeader
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 4
            color: '#ffffff'
            font.bold: true
            text: qsTr('Call history')
        }
    }

    ScrollView {
        id: historyView

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        clip: true
        focus: true

        delegate: Item {
            id: item
            width: parent.width
            height: appStyle.icon.smallSize + 2 * appStyle.margin.normal

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: 'transparent'
                color: model.active ? '#dd6666' : 'transparent'

                Image {
                    id: image

                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.margin.normal
                    anchors.verticalCenter: parent.verticalCenter
                    source: model.direction == QXmppCall.OutgoingDirection ? 'image://icon/call-outgoing' : 'image://icon/call-incoming'
                    sourceSize.width: appStyle.icon.smallSize
                    sourceSize.height: appStyle.icon.smallSize
                }

                Label {
                    anchors.left: image.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: date.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: model.name
                }

                Label {
                    id: date

                    anchors.right: duration.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    text: Utils.formatDateTime(model.date)
                    width: appStyle.font.normalSize * 8
                }

                Label {
                    id: duration

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: model.duration + 's'
                    width: appStyle.font.normalSize * 3
                }
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    historyView.forceActiveFocus();
                    historyView.currentIndex = model.index;
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
                        historyView.model.call(address);
                    }
                }
            }
        }
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
                    historyView.model.call(callAddress)
                } else if (item.action == 'contact') {
                    var contact = historyView.model.contactsModel.getContactByPhone(callPhone);
                    if (contact) {
                        dialogSwapper.showPanel('PhoneContactDialog.qml', {
                            'contactId': contact.id,
                            'contactName': contact.name,
                            'contactPhone': contact.phone,
                            'model': historyView.model.contactsModel});
                    } else {
                        dialogSwapper.showPanel('PhoneContactDialog.qml', {
                            'contactPhone': callPhone,
                            'model': historyView.model.contactsModel});
                    }
                } else if (item.action == 'remove') {
                    historyView.model.removeCall(callId);
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'call',
                    'icon': 'image://icon/call',
                    'text': qsTr('Call')});
                menu.model.append({
                    'action': 'contact',
                    'icon': 'image://icon/add',
                    'text': qsTr('Add to contacts')});
                menu.model.append({
                    'action': 'remove',
                    'icon': 'image://icon/remove',
                    'text': qsTr('Remove')});
            }
        }
    }
}
