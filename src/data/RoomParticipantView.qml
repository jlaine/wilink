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
    signal participantClicked(string participant)

    // FIXME: a non-static width crashes the program if chat room is empty
    // width: view.cellWidth + scrollBar.width
    width: 95

    ListView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.margins: 2

        delegate: Item {
            id: item

            property bool moderator: (model.affiliation >= QXmppMucItem.AdminAffiliation)

            width: 80
            height: 54

            Highlight {
                id: highlight

                anchors.fill: parent
                state: 'inactive'
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
                    color: moderator ? '#ff6500' : '#2689d6'
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: appStyle.font.smallSize
                    text: model.name
                }
            }

            Image {
                anchors.top: parent.top
                anchors.right: parent.right
                asynchronous: true
                anchors.rightMargin: 10
                source: 'moderator.png'
                smooth: true
                height: appStyle.icon.smallSize
                width: appStyle.icon.smallSize
                visible: moderator
            }

            MouseArea {
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (mouse.button == Qt.LeftButton) {
                        block.participantClicked(model.name);
                    } else if (mouse.button == Qt.RightButton) {
                        var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
                        menuLoader.sourceComponent = participantMenu;
                        menuLoader.item.jid = model.jid;
                        menuLoader.show(pos.x - menuLoader.item.width, pos.y);
                    }
                }
                onEntered: highlight.state = ''
                onExited: highlight.state = 'inactive'
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

    Component {
        id: participantMenu

        Menu {
            id: menu

            property alias jid: vcard.jid
            property bool kickEnabled: (room.allowedActions & QXmppMucRoom.KickAction)
            property bool profileEnabled: (vcard.url != undefined && vcard.url != '')

            VCard {
                id: vcard
            }

            onProfileEnabledChanged: menu.model.setProperty(0, 'enabled', profileEnabled)
            onKickEnabledChanged: menu.model.setProperty(1, 'enabled', kickEnabled)

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'profile') {
                    Qt.openUrlExternally(vcard.url)
                } else if (item.action == 'kick') {
                    dialogSwapper.showPanel('RoomKickDialog.qml', {
                        'jid': jid,
                        'room': room,
                    });
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'profile',
                    'enabled': profileEnabled,
                    'icon': 'information.png',
                    'text': qsTr('Show profile')});
                menu.model.append({
                    'action': 'kick',
                    'enabled': kickEnabled,
                    'icon': 'remove.png',
                    'text': qsTr('Kick user')});
            }
        }
    }
}
