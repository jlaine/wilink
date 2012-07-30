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
import 'sip.js' as Sip
import 'utils.js' as Utils

GroupBox {
    id: block

    property QtObject contactModel
    property alias model: historyView.model
    signal addressClicked(string address)
    signal addressDoubleClicked(string address)

    title: qsTr('Call history')

    function contactName(address) {
        var phone = Sip.parseAddress(address, sipClient.domain);
        for (var i = 0; i < contactModel.count; ++i) {
            var contact = contactModel.get(i);
            if (contact.phone == phone)
                return contact.name;
        }
        return phone;
    }

    ScrollView {
        id: historyView

        anchors.fill: parent
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
                    source: (model.flags & 1) == SipCall.OutgoingDirection ? 'image://icon/call-outgoing' : 'image://icon/call-incoming'
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
                    text: contactName(model.address)
                }

                Label {
                    id: date

                    anchors.right: duration.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    text: Utils.formatDateTime(Utils.datetimeFromString(model.date))
                    width: appStyle.font.normalSize * 8
                }

                Label {
                    id: duration

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    text: Utils.formatDuration(model.duration);
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
                            menuLoader.show(pos.x, pos.y);
                        }
                    }
                }
                onDoubleClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        block.addressDoubleClicked(model.address);
                    }
                }
            }
        }

        model: PhoneXmlModel {
            query: '/calls/call'

            XmlRole { name: 'id'; query: 'id/string()' }
            XmlRole { name: 'address'; query: 'address/string()' }
            XmlRole { name: 'date'; query: 'date/string()' }
            XmlRole { name: 'duration'; query: 'duration/string()' }
            XmlRole { name: 'flags'; query: 'flags/string()' }
        }

        Component {
            id: phoneHistoryMenu

            Menu {
                id: menu

                property string callAddress
                property int callId

                onItemClicked: {
                    var item = menu.model.get(index);
                    if (item.action == 'call') {
                        block.addressDoubleClicked(callAddress);
                    } else if (item.action == 'contact') {
                        var callPhone = Sip.parseAddress(callAddress, sipClient.domain);
                        var contact = contactModel.getContactByPhone(callPhone);
                        if (contact) {
                            dialogSwapper.showPanel('PhoneContactDialog.qml', {
                                'contactId': contact.id,
                                'contactName': contact.name,
                                'contactPhone': contact.phone,
                                'model': contactModel});
                        } else {
                            dialogSwapper.showPanel('PhoneContactDialog.qml', {
                                'contactPhone': callPhone,
                                'model': contactModel});
                        }
                    } else if (item.action == 'remove') {
                        historyView.model.removeItem(callId);
                    }
                }

                Component.onCompleted: {
                    menu.model.append({
                        action: 'call',
                        iconStyle: 'icon-phone',
                        text: qsTr('Call')});
                    menu.model.append({
                        action: 'contact',
                        iconStyle: 'icon-plus',
                        text: qsTr('Add to contacts')});
                    menu.model.append({
                        action: 'remove',
                        iconStyle: 'icon-minus',
                        text: qsTr('Remove')});
                }
            }
        }
    }
}
