/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import wiLink 2.4

ScrollView {
    id: block

    signal participantClicked(variant participant, variant mouse, variant mouseArea)

    clip: true
    delegate: Item {
        id: item

        property bool moderator: (model.affiliation >= QXmppMucItem.AdminAffiliation)

        width: 80
        height: appStyle.icon.normalSize + 22

        Column {
            anchors.fill: parent
            anchors.margins: 5

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                asynchronous: true
                source: model.avatar
                sourceSize.height: appStyle.icon.normalSize
                sourceSize.width: appStyle.icon.normalSize
                height: appStyle.icon.normalSize
                width: appStyle.icon.normalSize
             }

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                color: item.ListView.isCurrentItem ? 'white' : (moderator ? '#ff6500' : '#2689d6')
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
            source: 'image://icon/moderator'
            sourceSize.height: appStyle.icon.smallSize
            sourceSize.width: appStyle.icon.smallSize
            visible: moderator
        }

        MouseArea {
            id: mouseArea

            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: parent
            onClicked: {
                block.currentIndex = model.index;
                block.participantClicked({jid: model.jid, name: model.name}, mouse, mouseArea);
            }
        }
    }
}
