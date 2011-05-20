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

Rectangle {
    width: 320
    height: 400

    VCard {
        id: ownCard
        jid: client.jid

        onNameChanged: {
            room.nickName = name;
            if (!room.isJoined) {
                // clear history
                historyModel.clear();

                // send join request
                room.join();
            }
        }
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        icon: 'chat.png'
        title: '<b>' + room.jid.split('@')[0] + '</b>' + '<br/>' + room.subject
        z: 1
    }

    Item {
        anchors.top: header.bottom
        anchors.bottom: chatInput.top
        anchors.left: parent.left
        anchors.right: parent.right
    
        HistoryView {
            id: historyView

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: participantView.left
            model: historyModel

            onParticipantClicked: chatInput.talkAt(participant)
        }

        RoomParticipantView {
            id: participantView

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            model: participantModel
            width: 80

            onParticipantClicked: chatInput.talkAt(participant)
        }
    }

    ChatEdit {
        id: chatInput

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        model: participantModel

        onReturnPressed: {
            var text = chatInput.text;
            if (room.sendMessage(text))
                chatInput.text = '';
        }
    }

    Connections {
        target: room

        onError: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTr('Chat room error');
            // FIXME: get reason
            box.text = qsTr("Sorry, but you cannot join chat room '%1'.\n\n%2").replace('%1', room.jid).replace('%2', '');
            box.show();
        }

        onKicked: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTr('Chat room error');
            box.text = qsTr("Sorry, but you were kicked from chat room '%1'.\n\n%2").replace('%1', room.jid).replace('%2', reason);
            box.show();
        }
    }
}
