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
import 'utils.js' as Utils

Dialog {
    id: dialog

    property QtObject client
    property string domain: Qt.isQtObject(client) ? Utils.jidToDomain(client.jid) : ''

    helpText: domain == 'wifirst.net' ? qsTr('Your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>').replace('%1', 'https://www.wifirst.net/w/friends?from=wiLink') : ''
    title: qsTr('Add a contact')

    Item {
        anchors.fill: contents

        Label {
            id: label

            anchors.top: parent.top
            anchors.left:  parent.left
            anchors.right: parent.right
            text: qsTr('Enter the address of the contact you want to add.')
            wrapMode: Text.WordWrap

            onLinkActivated: Qt.openUrlExternally(link)
        }

        InputBar {
            id: bar

            anchors.top: label.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            focus: true
            validator: RegExpValidator {
                regExp: /^[^@/ ]+@[^@/ ]+$/
            }
        }
    }

    onAccepted: {
        if (!bar.acceptableInput)
            return;

        var jid = bar.text;
        console.log("Add contact " + jid);
        dialog.client.rosterManager.subscribe(jid);
        dialog.close();
    }

    Component.onCompleted: {
        if (dialog.domain != '') {
            bar.text = '@' + domain;
            bar.cursorPosition = 0;
        }
    }
}

