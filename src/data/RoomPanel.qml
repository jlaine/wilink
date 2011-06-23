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
        manager: appClient.mucManager
    }

    VCard {
        // NOTE: this is a hack so that we don't join the room before
        // the room object was created
        jid: Qt.isQtObject(room) ? Utils.jidToBareJid(appClient.jid) : ''

        onNameChanged: {
            room.nickName = name;
            // send join request
            if (room.nickName.length > 0 && !room.isJoined) {
                room.join();
            }
        }
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'chat.png'
        title: '<b>' + Utils.jidToUser(jid) + '</b>' + '<br/>' + (Qt.isQtObject(room) ? room.subject : '')
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                iconSource: 'invite.png'
                text: qsTr('Invite')

                onClicked: {
                    dialogLoader.source = 'RoomInviteDialog.qml';
                    dialogLoader.item.contacts = onlineContacts;
                    dialogLoader.item.room = room;
                    dialogLoader.show()
                }
            }

            ToolButton {
                iconSource: 'chat.png'
                text: qsTr('Subject')
                visible: Qt.isQtObject(room) && (room.allowedActions & QXmppMucRoom.SubjectAction)

                onClicked: {
                    dialogLoader.source = 'InputDialog.qml';
                    dialogLoader.item.title = qsTr('Change subject');
                    dialogLoader.item.labelText = qsTr('Enter the new room subject.');
                    dialogLoader.item.textValue = room.subject
                    dialogLoader.item.accepted.connect(function() {
                        room.subject = dialogLoader.item.textValue;
                        dialogLoader.hide();
                    });
                    dialogLoader.show()
                }
            }

            ToolButton {
                iconSource: 'options.png'
                text: qsTr('Options')
                visible: false //room.allowedActions & QXmppMucRoom.ConfigurationAction
            }

            ToolButton {
                iconSource: 'permissions.png'
                text: qsTr('Permissions')
                visible: room.allowedActions & QXmppMucRoom.PermissionsAction

                onClicked: {
                    dialogLoader.source = 'RoomPermissionDialog.qml';
                    dialogLoader.show();
                }
            }

            ToolButton {
                iconSource: 'close.png'
                text: qsTr('Close')
                onClicked: {
                    room.leave();
                    roomListModel.removeRoom(room.jid);
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
                    var re = new RegExp('@' + Utils.escapeRegExp(room.nickName) + '[,:]');
                    if (text.match(re) && (!window.isActiveWindow || panel.opacity == 0)) {
                        panel.notify(Utils.jidToResource(jid), text);
                        if (panel.opacity == 0)
                            roomListModel.addPendingMessage(participantModel.jid);
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

    Connections {
        target: room

        onError: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTranslate('RoomPanel', 'Chat room error');
            // FIXME: get reason
            box.text = qsTranslate('RoomPanel', "Sorry, but you cannot join chat room '%1'.\n\n%2").replace('%1', jid).replace('%2', '');
            box.show();
        }

        onJoined: {
            roomListModel.addRoom(room.jid);
        }

        onKicked: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.windowTitle = qsTranslate('RoomPanel', 'Chat room error');
            box.text = qsTranslate('RoomPanel', "Sorry, but you were kicked from chat room '%1'.\n\n%2").replace('%1', jid).replace('%2', reason);
            box.show();
        }
    }

    Connections {
        target: appClient
        onConnected: {
            // re-join after disconnect
            if (room.nickName.length > 0 && !room.isJoined) {
                room.join();
            }
        }
    }

    Keys.forwardTo: historyView
}
