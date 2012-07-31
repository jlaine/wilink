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

ContactView {
    id: block

    signal itemClicked(variant model)

    title: qsTr('My contacts')

    delegate: Item {
        id: item

        height: appStyle.icon.smallSize + 16
        width: parent.width

        Image {
            id: avatar

            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            asynchronous: true
            source: 'image://icon/peer'
            sourceSize.width: appStyle.icon.smallSize
            sourceSize.height: appStyle.icon.smallSize
        }

        Column {
            anchors.left: avatar.right
            anchors.right: callButton.left
            anchors.leftMargin: 3
            anchors.verticalCenter: parent.verticalCenter

            Label {
                id: name

                anchors.left: parent.left
                anchors.right: parent.right
                elide: Text.ElideRight
                text: model.name
            }

            Label {
                id: phone

                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.right: parent.right
                elide: Text.ElideRight
                font.italic: true
                text: model.phone
                visible: false
            }
        }

        Button {
            id: callButton

            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            iconStyle: 'icon-phone'
            visible: view.currentItem == item
            z: 1

            onClicked: {
                block.itemClicked(model);
            }
        }

        MouseArea {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: parent

            onClicked: {
                view.forceActiveFocus();
                if (mouse.button == Qt.LeftButton) {
                    view.currentIndex = model.index;
                } else if (mouse.button == Qt.RightButton) {
                    // show context menu
                    var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
                    menuLoader.sourceComponent = phoneContactMenu;
                    menuLoader.item.contactId = model.id;
                    menuLoader.item.contactName = model.name;
                    menuLoader.item.contactPhone = model.phone;
                    menuLoader.show(pos.x, pos.y);
                }
            }

            onDoubleClicked: {
                view.currentIndex = model.index;
                block.itemClicked(model);
            }
        }

        states: [
            State {
                name: 'no-avatar'
                when: view.width < 32
                PropertyChanges { target: avatar; visible: false }
            },
            State {
                name: 'expanded'
                when: view.currentItem == item
                PropertyChanges { target: name; font.bold: true }
                PropertyChanges { target: phone; visible: true }
            }
        ]
    }

    model: PhoneXmlModel {
        id: xmlModel

        function getContactByPhone(phone) {
            for (var i = 0; i < xmlModel.count; ++i) {
                var contact = xmlModel.get(i);
                if (contact.phone == phone)
                    return contact;
            }
        }

        query: '/contacts/contact'

        XmlRole { name: 'id'; query: 'id/string()' }
        XmlRole { name: 'name'; query: 'name/string()' }
        XmlRole { name: 'phone'; query: 'phone/string()' }
    }

    onAddClicked: {
        dialogSwapper.showPanel('PhoneContactDialog.qml', {'model': block.model});
    }

    resources: [
        Component {
            id: phoneContactMenu

            Menu {
                id: menu

                property int contactId
                property string contactName
                property string contactPhone

                onItemClicked: {
                    var item = menu.model.get(index);
                    if (item.action == 'edit') {
                        dialogSwapper.showPanel('PhoneContactDialog.qml', {
                            'contactId': contactId,
                            'contactName': contactName,
                            'contactPhone': contactPhone,
                            'model': block.model});
                    } else if (item.action == 'remove') {
                        block.model.removeItem(contactId);
                    }
                }

                Component.onCompleted: {
                    menu.model.append({
                        action: 'edit',
                        iconStyle: 'icon-edit',
                        text: qsTr('Modify')});
                    menu.model.append({
                        action: 'remove',
                        iconStyle: 'icon-minus',
                        text: qsTr('Remove')});
                }
            }
        }
    ]
}
