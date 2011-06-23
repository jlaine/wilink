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
import wiLink 1.2

Dialog {
    id: dialog

    property alias room: permissionModel.room

    minimumWidth: 280
    minimumHeight: 150
    title: qsTr('Chat room permissions')

    RoomPermissionModel {
        id: permissionModel
    }

    Item {
        anchors.fill: contents

        ListView {
            id: view

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: scrollBar.left
            clip: true

            model: permissionModel
            delegate: Item {
                id: item

                width: parent.width
                height: combo.height

                Text {
                    id: label

                    anchors.left: parent.left
                    anchors.right: combo.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: model.jid
                    verticalAlignment: Text.AlignVCenter
                }

                ComboBox {
                    id: combo

                    anchors.top: parent.top
                    anchors.right: parent.right
                    currentIndex: 1
                    model: ListModel {}
                    width: 150
                    Component.onCompleted: {
                        model.append({'text': qsTr('member'), 'value': QXmppMucItem.MemberAffiliation});
                        model.append({'text': qsTr('administrator'), 'value': QXmppMucItem.AdminAffiliation});
                        model.append({'text': qsTr('owner'), 'value': QXmppMucItem.OwnerAffiliation});
                        model.append({'text': qsTr('banner'), 'value': QXmppMucItem.OutcastAffiliation});
                    }
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
    }
}
