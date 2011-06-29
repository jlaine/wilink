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
import wiLink 1.2

Dialog {
    id: dialog

    property QtObject call
    property bool callHandled: false
    property Item panel
    property int soundId: 0

    title: qsTr('Call from %1').replace('%1', vcard.name)

    VCard {
        id: vcard

        jid: Qt.isQtObject(call) ? call.jid : ''
    }

    Item {
        anchors.fill: contents

        Image {
            id: image

            anchors.top: parent.top
            anchors.left: parent.left
            source: vcard.avatar
        }

        Text {
            anchors.top: parent.top
            anchors.left: image.right
            anchors.leftMargin: 8
            anchors.right: parent.right
            text: qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', vcard.name)
            wrapMode: Text.WordWrap
        }
    }

    onAccepted: {
        panel.showConversation(call.jid);
        dialog.call.accept();
        dialog.callHandled = true;
        dialog.close();
    }

    onClose: {
        // stop sound
        application.soundPlayer.stop(dialog.soundId);

        if (!dialog.callHandled) {
            dialog.call.hangup();
        }
    }

    Component.onCompleted: {
        // alert window
        window.alert();

        // play a sound
        dialog.soundId = application.soundPlayer.play(":/call-incoming.ogg", true);
    }
}

