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
import 'utils.js' as Utils

Panel {
    id: panel

    property alias jid: participantModel.jid
    property alias room: participantModel.room

    RoomModel {
        id: participantModel
        manager: window.client.mucManager
    }

    VCard {
        // NOTE: this is a hack so that we don't join the room before
        // the room object was created
        jid: Qt.isQtObject(room) ? Utils.jidToBareJid(window.client.jid) : ''

        onNameChanged: {
            room.nickName = name;
            if (!room.isJoined) {
                // clear history
                participantModel.historyModel.clear();

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
        title: '<b>' + Utils.jidToUser(jid) + '</b>' + '<br/>' + (Qt.isQtObject(room) ? room.subject : '')
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'invite.png'
                text: qsTr('Invite')

                onClicked: {
                    inviteDialog.show()
                }
            }

            ToolButton {
                icon: 'chat.png'
                text: qsTr('Subject')
                visible: Qt.isQtObject(room) && (room.allowedActions & QXmppMucRoom.SubjectAction)

                onClicked: {
                    var dialog = window.inputDialog();
                    dialog.windowTitle = qsTr('Change subject');
                    dialog.labelText = qsTr('Enter the new room subject.');
                    dialog.textValue = room.subject
                    if (dialog.exec()) {
                        room.subject = dialog.textValue;
                    }
                }
            }

            ToolButton {
                icon: 'options.png'
                text: qsTr('Options')
                visible: false //room.allowedActions & QXmppMucRoom.ConfigurationAction
            }

            ToolButton {
                icon: 'permissions.png'
                text: qsTr('Permissions')
                visible: false //room.allowedActions & QXmppMucRoom.PermissionsAction
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
                onClicked: {
                    room.leave();
                    roomListModel.removeRoom(room.jid);
                    participantModel.unbookmark();
                    panel.close();
                }
            }
        }
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
            model: participantModel.historyModel

            onParticipantClicked: chatInput.talkAt(participant)

            Connections {
                target: historyView.model
                onMessageReceived: {
                    if (panel.opacity == 0) {
                        roomListModel.addPendingMessage(participantModel.jid);
                        application.soundPlayer.play(application.incomingMessageSound);
                    }
                }
            }
        }

        RoomParticipantView {
            id: participantView

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            model: participantModel

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
            if (room.sendMessage(text)) {
                chatInput.text = '';
                application.soundPlayer.play(application.outgoingMessageSound);
            }
        }
    }

    RoomInviteDialog {
        id: inviteDialog

        x: 10
        y: 110
        model: contactModel
        visible: false

        onAccepted: {
            for (var i in selection) {
                console.log("inviting " + selection[i]);
                room.sendInvitation(selection[i], reason);
            }
            selection = [];
        }
    }

    Connections {
        target: room

        onError: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTr('Chat room error');
            // FIXME: get reason
            box.text = qsTr("Sorry, but you cannot join chat room '%1'.\n\n%2").replace('%1', jid).replace('%2', '');
            box.show();
        }

        onJoined: {
            participantModel.bookmark();
        }

        onKicked: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTr('Chat room error');
            box.text = qsTr("Sorry, but you were kicked from chat room '%1'.\n\n%2").replace('%1', jid).replace('%2', reason);
            box.show();
        }
    }
}
