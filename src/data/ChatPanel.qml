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
import 'utils.js' as Utils

Panel {
    id: chatPanel

    /** Convenience method to show a conversation panel.
     */
    function showConversation(jid) {
        swapper.showPanel('ChatPanel.qml');
        chatSwapper.showPanel('ConversationPanel.qml', {'jid': Utils.jidToBareJid(jid)});
    }

    /** Convenience method to show a chat room panel.
     */
    function showRoom(jid) {
        chatSwapper.showPanel('RoomPanel.qml', {'jid': jid})
    }

    Item {
        id: left

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 200

        RosterView {
            id: rooms

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            currentJid: (Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            enabled: window.client.mucServer != ''
            model: RoomListModel {
                id: roomListModel
                client: window.client

                onRoomAdded: {
                    if (!chatSwapper.findPanel('RoomPanel.qml', {'jid': jid})) {
                        if (chatSwapper.currentItem) {
                            chatSwapper.addPanel('RoomPanel.qml', {'jid': jid});
                        } else {
                            chatPanel.showRoom(jid);
                        }
                    }
                }
            }
            title: qsTr('My rooms')
            height: 24 + (count > 0 ? 4 : 0) + 30 * Math.min(count, 8);

            onAddClicked: {
                dialogLoader.source = 'RoomJoinDialog.qml';
                dialogLoader.item.panel = chatPanel;
                dialogLoader.show();
            }

            onCurrentJidChanged: {
                rooms.model.clearPendingMessages(rooms.currentJid);
            }

            onItemClicked: showRoom(model.jid)

            Connections {
                target: window.client.mucManager

                onInvitationReceived: {
                    var jid = Utils.jidToBareJid(inviter);
                    var box = window.messageBox();
                    box.windowTitle = qsTranslate('ChatPanel', 'Invitation from %1').replace('%1', jid);
                    box.text = qsTranslate('ChatPanel', "%1 has invited you to join the '%2' chat room.\n\nDo you accept?").replace('%1', jid).replace('%2', roomJid);
                    box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                    if (box.exec() == QMessageBox.Yes) {
                        swapper.showPanel('ChatPanel.qml');
                        showRoom(roomJid);
                    }
                }
            }
        }

        Rectangle {
            id: splitter

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: rooms.bottom
            color: '#567dbc'
            height: 5

            MouseArea {
                property int mousePressY
                property int roomsPressHeight

                anchors.fill: parent

                hoverEnabled: true
                onEntered: parent.state = 'hovered'
                onExited: parent.state = ''

                onPressed: {
                    mousePressY = mapToItem(left, mouse.x, mouse.y).y
                    roomsPressHeight = rooms.height
                }

                onPositionChanged: {
                    if (mouse.buttons & Qt.LeftButton) {
                        var position =  roomsPressHeight + mapToItem(left, mouse.x, mouse.y).y - mousePressY
                        position = Math.max(position, 0)
                        position = Math.min(position, left.height - splitter.height - statusBar.height)
                        rooms.height = position
                    }
                }
            }
            states: State {
                name: 'hovered'
                PropertyChanges{ target: splitter; color: '#97b0d9' }
            }
        }

        RosterView {
            id: contacts

            SortFilterProxyModel {
                id: onlineContacts

                dynamicSortFilter: true
                filterRole: RosterModel.StatusFilterRole
                filterRegExp: /^(?!offline)/
                sourceModel: RosterModel {
                    id: rosterModel
                    client: window.client
                }
            }

            SortFilterProxyModel {
                id: sortedContacts

                dynamicSortFilter: true
                sortCaseSensitivity: Qt.CaseInsensitive
                sortRole: application.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
                sourceModel: application.showOfflineContacts ? rosterModel : onlineContacts
                Component.onCompleted: sort(0)
            }

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: statusBar.top
            currentJid: (Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: sortedContacts
            title: qsTr('My contacts')

            onAddClicked: {
                dialogLoader.source = 'ContactAddDialog.qml';
                dialogLoader.show();
            }

            onCurrentJidChanged: {
                rosterModel.clearPendingMessages(contacts.currentJid);
            }

            onItemClicked: {
                showConversation(model.jid);
            }

            onItemContextMenu: {
                var pos = mapToItem(menuLoader.parent, point.x, point.y);
                menuLoader.source = 'ContactMenu.qml';
                menuLoader.item.jid = model.jid;
                menuLoader.show(pos.x, pos.y);
            }

            Connections {
                target: window.client.rosterManager
                onSubscriptionReceived: {
                    // If we have a subscription to the requester, accept
                    // reciprocal subscription.
                    //
                    // FIXME: use QXmppRosterIq::Item::To and QXmppRosterIq::Item::Both
                    var subscription = window.client.rosterManager.subscriptionType(bareJid);
                    if (subscription == 2 || subscription == 3) {
                        // accept subscription
                        window.client.rosterManager.acceptSubscription(bareJid);
                        return;
                    }

                    var box = window.messageBox();
                    box.windowTitle = qsTranslate('ChatPanel', 'Invitation from %1').replace('%1', bareJid);
                    box.text = qsTranslate('ChatPanel', '%1 has asked to add you to his or her contact list.\n\nDo you accept?').replace('%1', bareJid);
                    box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                    if (box.exec() == QMessageBox.Yes) {
                        // accept subscription
                        window.client.rosterManager.acceptSubscription(bareJid);

                        // request reciprocal subscription
                        window.client.rosterManager.subscribe(bareJid);
                    } else {
                        // refuse subscription
                        window.client.rosterManager.refuseSubscription(bareJid);
                    }
                }
            }
        }

        StatusBar {
            id: statusBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 28
        }

        // FIXME : this is a hack to replay received messages after
        // adding the appropriate conversation
        Connections {
            target: window.client
            onMessageReceived: {
                var jid = Utils.jidToBareJid(from);
                if (!chatSwapper.findPanel('ConversationPanel.qml', {'jid': jid})) {
                    chatSwapper.addPanel('ConversationPanel.qml', {'jid': jid});
                    window.client.replayMessage();
                }
            }
        }

        Connections {
            target: window.client.callManager
            onCallReceived: {
                var contactName = call.jid;
                console.log("Call received: " + contactName);

                // start incoming tone
                //int soundId = wApp->soundPlayer()->play(":/call-incoming.ogg", true);
                //m_callQueue.insert(call, soundId);

                // prompt user
                var box = window.messageBox();
                box.windowTitle = qsTranslate('ChatPanel', 'Call from %1').replace('%1', contactName);
                box.text = qsTranslate('ChatPanel', '%1 wants to talk to you.\n\nDo you accept?').replace('%1', contactName);
                box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                if (box.exec()) {
                    showConversation(call.jid);
                    call.accept();
                } else {
                    call.hangup();
                }
            }
        }

        Connections {
            target: window.client.transferManager
            onFileReceived: {
                var contactName = job.jid;
                console.log("File received: " + contactName);

                // prompt user
                var box = window.messageBox();
                box.windowTitle = qsTranslate('ChatPanel', 'File from %1').replace('%1', contactName);
                box.text = qsTranslate('ChatPanel', "%1 wants to send you a file called '%2' (%3).\n\nDo you accept?").replace('%1', contactName).replace('%2', job.fileName).replace('%3', job.fileSize);
                box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                if (box.exec()) {
                    // FIXME: this silently overwrite existing files!
                    var path = application.downloadsLocation + '/' + job.fileName;
                    console.log("File accepted: " + path);
                    showConversation(job.jid);
                    job.accept(path);
                } else {
                    job.abort();
                }
            }
        }
    }

    PanelSwapper {
        id: chatSwapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: left.right
        anchors.right: parent.right
        focus: true

        Connections {
            target: application
            onMessageClicked: chatSwapper.setCurrentItem(context)
        }
    }

    Loader {
        id: wifirst

        Connections {
            target: window.client
            onConnected: {
                wifirst.source = (Utils.jidToDomain(window.client.jid) == 'wifirst.net') ? 'Wifirst.qml' : '';
            }
        }
    }

    states: State {
        name: 'noroster'

        PropertyChanges { target: left; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: left; properties: 'width'; duration: 200 }
    }
}
