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

ContactView {
    id: block

    property QtObject contactsModel

    signal itemClicked(variant model)

    title: qsTr('My contacts')

    model: SortFilterProxyModel {
        dynamicSortFilter: true
        sortCaseSensitivity: Qt.CaseInsensitive
        sortRole: PhoneContactModel.NameRole
        sourceModel: contactsModel
        Component.onCompleted: sort(0)
    }

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
            iconSize: appStyle.icon.smallSize
            iconSource: 'image://icon/call'
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

    onAddClicked: {
        dialogSwapper.showPanel('PhoneContactDialog.qml', {'model': contactsModel});
    }

    Component {
        id: phoneContactMenu

        Menu {
            id: menu

            property int contactId

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'edit') {
                    var contact = contactsModel.getContact(contactId);
                    dialogSwapper.showPanel('PhoneContactDialog.qml', {
                        'contactId': contactId,
                        'contactName': contact.name,
                        'contactPhone': contact.phone,
                        'model': contactsModel});
                } else if (item.action == 'remove') {
                    contactsModel.removeContact(contactId);
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'edit',
                    'iconSource': 'image://icon/options',
                    'text': qsTr('Modify')});
                menu.model.append({
                    'action': 'remove',
                    'iconSource': 'image://icon/remove',
                    'text': qsTr('Remove')});
            }
        }
    }
}
