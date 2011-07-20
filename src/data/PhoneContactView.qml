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

Item {
    id: block

    property alias count: view.count
    property QtObject model

    signal itemClicked(variant model)

    function contactForPhone(phone) {
        for (var i = 0; i < listHelper.count; i++) {
            if (listHelper.getProperty(i, 'phone') == phone) {
                return listHelper.get(i);
            }
        }
        return null;
    }

    ListHelper {
        id: listHelper

        model: block.model
    }

    Rectangle {
        id: background

        width: parent.height
        height: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width

        gradient: Gradient {
            GradientStop { id: backgroundStop1; position: 0.0; color: '#e7effd' }
            GradientStop { id: backgroundStop2; position: 1.0; color: '#cbdaf1' }
        }

        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: background.height
        }
    }

    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        border.color: '#88a4d1'
        border.width: 1
        height: 24
        z: 1

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            color: '#ffffff'
            font.bold: true
            text: qsTr('My contacts')
        }

        Button {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            iconSize: appStyle.icon.tinySize
            iconSource: 'add.png'

            onClicked: {
                dialogSwapper.showPanel('PhoneContactDialog.qml', {'model': block.model});
            }
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 2
        model: SortFilterProxyModel {
            dynamicSortFilter: true
            sortCaseSensitivity: Qt.CaseInsensitive
            sortRole: PhoneContactModel.NameRole
            sourceModel: block.model
            Component.onCompleted: sort(0)
        }

        delegate: Item {
            id: item

            height: 40
            width: parent.width

            Image {
                id: avatar

                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                asynchronous: true
                width: appStyle.icon.smallSize
                height: appStyle.icon.smallSize
                smooth: true
                source: 'peer.png'
            }

            Column {
                anchors.left: avatar.right
                anchors.right: callButtonLoader.left
                anchors.leftMargin: 3
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    id: name

                    anchors.left: parent.left
                    anchors.right: parent.right
                    elide: Text.ElideRight
                    text: model.name
                }

                Text {
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

            Loader {
                id: callButtonLoader

                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                z: 1
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
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

                onEntered: {
                    callButtonLoader.sourceComponent = callButtonComponent;
                }

                onExited: {
                    callButtonLoader.sourceComponent = undefined;
                }
            }

            states: State {
                name: 'expanded'
                when: view.currentItem == item
                PropertyChanges { target: name; font.bold: true }
                PropertyChanges { target: phone; visible: true }
            }
        }

        highlight: Highlight {}
        highlightMoveDuration: 500
    }

    Component {
        id: callButtonComponent

        Button {
            iconSize: appStyle.icon.tinySize
            iconSource: 'call.png'
            smooth: true

            onClicked: {
                block.itemClicked(block.model);
            }
        }
    }

    Component {
        id: phoneContactMenu

        Menu {
            id: menu

            property int contactId

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'edit') {
                    var contact = block.model.getContact(contactId);
                    dialogSwapper.showPanel('PhoneContactDialog.qml', {
                        'contactId': contactId,
                        'contactName': contact.name,
                        'contactPhone': contact.phone,
                        'model': block.model});
                } else if (item.action == 'remove') {
                    block.model.removeContact(contactId);
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'edit',
                    'icon': 'options.png',
                    'text': qsTr('Modify')});
                menu.model.append({
                    'action': 'remove',
                    'icon': 'remove.png',
                    'text': qsTr('Remove')});
            }
        }
    }
}
