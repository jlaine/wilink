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
import 'scripts/utils.js' as Utils

Dialog {
    id: dialog

    property string jid

    property QtObject client

    title: qsTr('Add a contact')

    resources: [
        VCard {
            id: vcard
            jid: dialog.jid
        }
    ]

    Column {
        anchors.fill: parent
        spacing: 8

        PanelHelp {
            id: help

            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr('The contact you tried to reach is not in your contact list.')
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 25

            Image {
                id: image

                anchors.top: parent.top
                anchors.left: parent.left
                source: vcard.avatar
            }

            Label {
                anchors.left: image.right
                anchors.leftMargin: 8
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 15
                anchors.bottom: parent.bottom
                horizontalAlignment: Text.Center
                verticalAlignment: Text.Center
                text: qsTr('Send a subscription request to %1?').replace('%1', vcard.name);
                wrapMode: Text.WordWrap
            }
        }
    }

    onAccepted: {
        console.log("Add contact " + jid + " to account " + client.jid);
        client.rosterManager.subscribe(jid);
        dialog.close();
    }
}

