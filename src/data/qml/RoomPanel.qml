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
import wiLink 2.4
import 'utils.js' as Utils

Panel {
    id: panel

    property QtObject client
    property string iconSource: 'image://icon/chat'
    property alias jid: participantModel.jid
    property alias room: participantModel.room
    property string title: Utils.jidToUser(jid)
    property string subTitle: Qt.isQtObject(room) ? room.subject : ''
    property string presenceStatus

    RoomModel {
        id: participantModel

        onJidChanged: {
            panel.client = accountModel.clientForJid(jid);
            manager = client.mucManager;
        }
    }

    VCard {
        id: ownCard

        // NOTE: this is a hack so that we don't join the room before
        // the room object was created
        jid: Qt.isQtObject(room) ? Utils.jidToBareJid(client.jid) : ''

        onNickNameChanged: {
            room.nickName = nickName;
            // send join request
            if (room.nickName.length > 0 && !room.isJoined) {
                room.join();
            }
        }
    }

    Item {
        anchors.top: parent.top
        anchors.topMargin: appStyle.margin.large
        anchors.bottom: chatInput.top
        anchors.bottomMargin: appStyle.margin.normal
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
                    var jid = participantModel.jid;
                    var re = new RegExp('@' + Utils.escapeRegExp(room.nickName) + '[,:]');
                    if (text.match(re) && rooms.currentJid != jid) {
                        // show notification
                        if (appSettings.incomingMessageNotification) {
                            var handle = appNotifier.showMessage(Utils.jidToResource(jid), text, qsTranslate('RoomPanel', 'Show this room'));
                            if (handle) {
                                handle.clicked.connect(function() {
                                    window.showAndRaise();
                                    showRoom(jid);
                                });
                            }
                        }

                        // notify alert
                        appNotifier.alert();

                        // play a sound
                        if (appSettings.incomingMessageSound) {
                            appSoundPlayer.play(":/sounds/message-incoming.ogg");
                        }

                        // add pending message
                        roomListModel.addPendingMessage(jid);
                    }
                }
            }
        }

        GroupBox {
            id: participantView

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            headerComponent: null
            width: 95

            RoomParticipantView {
                anchors.fill: parent
                model: participantModel

                onParticipantClicked: chatInput.talkAt(participant)
            }
        }
    }

    ChatEdit {
        id: chatInput

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        menuComponent: Menu {
            id: menu

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'invite') {
                    dialogSwapper.showPanel('RoomInviteDialog.qml', {
                        'contacts': onlineContacts,
                        'room': room,
                    });
                } else if (item.action == 'subject') {
                    dialogSwapper.showPanel('RoomSubjectDialog.qml', {'room': room});
                } else if (item.action == 'configuration') {
                    dialogSwapper.showPanel('RoomConfigurationDialog.qml', {'room': room});
                } else if (item.action == 'permission') {
                    dialogSwapper.showPanel('RoomPermissionDialog.qml', {'room': room});
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    action: 'invite',
                    iconStyle: 'icon-plus',
                    text: qsTr('Invite')});
                if (room.allowedActions & QXmppMucRoom.SubjectAction) {
                    menu.model.append({
                        action: 'subject',
                        iconStyle: 'icon-edit',
                        text: qsTr('Subject')});
                }
                if (room.allowedActions & QXmppMucRoom.ConfigurationAction) {
                    menu.model.append({
                        action: 'configuration',
                        iconStyle: 'icon-wrench',
                        text: qsTr('Options')});
                }
                if (room.allowedActions & QXmppMucRoom.PermissionsAction) {
                    menu.model.append({
                        action: 'permission',
                        iconStyle: 'icon-lock',
                        text: qsTr('Permissions')});
                }
            }
        }
        model: participantModel

        onReturnPressed: {
            var text = chatInput.text;
            if (room.sendMessage(text)) {
                chatInput.text = '';
                appSoundPlayer.play(appSettings.outgoingMessageSound);
            }
        }
    }

    Connections {
        target: room

        onError: {
            dialogSwapper.showPanel('ErrorNotification.qml', {
                title: qsTranslate('RoomPanel', 'Chat room error'),
                text: qsTranslate('RoomPanel', "Sorry, but you cannot join chat room '%1'.\n\n%2").replace('%1', room.jid).replace('%2', ''),
            });
        }

        onJoined: {
            roomListModel.addRoom(room.jid);
        }

        onKicked: {
            dialogSwapper.showPanel('ErrorNotification.qml', {
                title: qsTranslate('RoomPanel', 'Chat room error'),
                text: qsTranslate('RoomPanel', "Sorry, but you were kicked from chat room '%1'.\n\n%2").replace('%1', room.jid).replace('%2', reason),
            });
        }
    }

    Connections {
        target: client
        onConnected: {
            // re-join after disconnect
            if (room.nickName.length > 0 && !room.isJoined) {
                room.join();
            }
        }
    }

    onClose: {
        room.leave();
        roomListModel.removeRoom(room.jid);
    }

    Keys.forwardTo: historyView

    states: State {
        name: 'no-participants'
        when: panel.width < 300
        PropertyChanges { target: participantView; opacity: 0; width: 0 }
    }
}
