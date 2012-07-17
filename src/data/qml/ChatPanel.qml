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
import wiLink 2.0
import 'utils.js' as Utils

Panel {
    id: chatPanel

    property alias rooms: roomListModel
    property bool pendingMessages: (roomListModel.pendingMessages + rosterModel.pendingMessages) > 0

    Repeater {
        id: chatClients

        model: ListModel {}

        Item {
            id: item

            property QtObject client: Client {
                logger: appLogger
            }

            Connections {
                target: item.client

                onAuthenticationFailed: {
                    console.log("Failed to authenticate with chat server");
                    if (Utils.jidToDomain(item.client.jid) != 'wifirst.net') {
                        var jid = Utils.jidToBareJid(item.client.jid);
                        dialogSwapper.showPanel('AccountPasswordDialog.qml', {client: item.client, jid: jid});
                    }
                }

                onConflictReceived: {
                    console.log("Received a resource conflict from chat server");
                    Qt.quit();
                }

                // FIXME : this is a hack to replay received messages after
                // adding the appropriate conversation
                onMessageReceived: {
                    var opts = { jid: Utils.jidToBareJid(from) };
                    if (!chatSwapper.findPanel('ConversationPanel.qml', opts)) {
                        chatSwapper.addPanel('ConversationPanel.qml', opts);
                        item.client.replayMessage();
                    }
                }
            }

            Connections {
                target: item.client.callManager
                onCallReceived: {
                    dialogSwapper.showPanel('CallNotification.qml', {
                        'call': call,
                        'panel': chatPanel});
                }
            }

            Connections {
                target: item.client.mucManager

                onInvitationReceived: {
                    dialogSwapper.showPanel('RoomInviteNotification.qml', {
                        'jid': Utils.jidToBareJid(inviter),
                        'panel': chatPanel,
                        'roomJid': roomJid});
                }
            }

            Connections {
                target: item.client.rosterManager

                onSubscriptionReceived: {
                    // If we have a subscription to the requester, accept
                    // reciprocal subscription.
                    //
                    // FIXME: use QXmppRosterIq::Item::To and QXmppRosterIq::Item::Both
                    var subscription = item.client.subscriptionType(bareJid);
                    if (subscription == 2 || subscription == 3) {
                        // accept subscription
                        item.client.rosterManager.acceptSubscription(bareJid);
                        return;
                    }

                    dialogSwapper.showPanel('ContactAddNotification.qml', {jid: bareJid, rosterManager: item.client.rosterManager});
                }
            }

            Connections {
                target: item.client.transferManager
                onFileReceived: {
                    dialogSwapper.showPanel('TransferNotification.qml', {
                        'job': job,
                        'panel': chatPanel});
                }
            }
        }

        onItemAdded: {
            var data = model.get(index);
            if (data.facebookAppId) {
                console.log("connecting to facebook: " + data.facebookAppId);
                item.client.connectToFacebook(data.facebookAppId, data.facebookAccessToken);
                rosterModel.addClient(item.client);
                roomListModel.addClient(item.client);
            } else {
                console.log("connecting to: " + data.jid);
                item.client.connectToServer(data.jid, data.password);
                rosterModel.addClient(item.client);
                roomListModel.addClient(item.client);
                statusBar.addClient(item.client);
            }
        }

        onItemRemoved: {
            console.log("removing client: " + item.client.jid);
            rosterModel.removeClient(item.client);
            roomListModel.removeclient(item.client);
            statusBar.removeClient(item.client);
        }
    }

    /** Convenience method to show a conversation panel.
     */
    function showConversation(jid) {
        swapper.showPanel('ChatPanel.qml');
        chatSwapper.showPanel('ConversationPanel.qml', { jid: Utils.jidToBareJid(jid) });
        if (chatPanel.singlePanel)
            chatPanel.state = 'no-sidebar';
    }

    /** Convenience method to show a chat room panel.
     */
    function showRoom(jid) {
        swapper.showPanel('ChatPanel.qml');
        chatSwapper.showPanel('RoomPanel.qml', { jid: jid });
        if (chatPanel.singlePanel)
            chatPanel.state = 'no-sidebar';
    }

    Item {
        id: sidebar

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: width > 0
        width: chatPanel.singlePanel ? parent.width : appStyle.sidebarWidth
        z: 2

        ChatContactView {
            id: rooms

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            currentJid: (Qt.application.active && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            iconSource: 'image://icon/chat'
            model: RoomListModel {
                id: roomListModel

                onRoomAdded: {
                    var opts = { jid: jid };
                    if (!chatSwapper.findPanel('RoomPanel.qml', opts)) {
                        if (chatSwapper.currentItem) {
                            chatSwapper.addPanel('RoomPanel.qml', opts);
                        } else {
                            chatSwapper.showPanel('RoomPanel.qml', opts);
                        }
                    }
                }
            }
            title: qsTr('My rooms')
            height: headerHeight + 4 + rowHeight * (appStyle.isMobile ? 2 : 4)

            onAddClicked: {
                // FIXME: we only support default client
                dialogSwapper.showPanel('RoomJoinDialog.qml', {client: accountModel.clientForJid()});
            }

            onCurrentJidChanged: {
                rooms.model.clearPendingMessages(rooms.currentJid);
            }

            onItemClicked: showRoom(model.jid)
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
                    mousePressY = mapToItem(sidebar, mouse.x, mouse.y).y
                    roomsPressHeight = rooms.height
                }

                onPositionChanged: {
                    if (mouse.buttons & Qt.LeftButton) {
                        var position =  roomsPressHeight + mapToItem(sidebar, mouse.x, mouse.y).y - mousePressY
                        position = Math.max(position, 0)
                        position = Math.min(position, sidebar.height - splitter.height - statusBar.height)
                        rooms.height = position
                    }
                }
            }
            states: State {
                name: 'hovered'
                PropertyChanges{ target: splitter; color: '#97b0d9' }
            }
        }

        ChatContactView {
            id: contacts

            SortFilterProxyModel {
                id: onlineContacts

                dynamicSortFilter: true
                filterRole: RosterModel.StatusFilterRole
                filterRegExp: /^(?!offline)/
                sourceModel: RosterModel {
                    id: rosterModel
                }
            }

            SortFilterProxyModel {
                id: sortedContacts

                dynamicSortFilter: true
                sortCaseSensitivity: Qt.CaseInsensitive
                sortRole: appSettings.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
                sourceModel: appSettings.showOfflineContacts ? rosterModel : onlineContacts
                Component.onCompleted: sort(0)
            }

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: statusBar.top
            currentJid: (Qt.application.active && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: sortedContacts
            title: qsTr('My contacts')

            onAddClicked: {
                dialogSwapper.showPanel('ContactAddDialog.qml');
            }

            onCurrentJidChanged: {
                rosterModel.clearPendingMessages(contacts.currentJid);
            }

            onItemClicked: {
                showConversation(model.jid);
            }

            onItemContextMenu: {
                var pos = mapToItem(menuLoader.parent, point.x, point.y);
                menuLoader.sourceComponent = contactMenu;
                menuLoader.item.jid = model.jid;
                menuLoader.show(pos.x, pos.y);
            }

        }

        StatusBar {
            id: statusBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        // FIXME : this is a hack to make sure ConversationPanel loads fast
        ConversationPanel {
            opacity: 0
        }

    }

    TabView {
        id: chatTabs

        anchors.top: parent.top
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        panelSwapper: chatSwapper
        z: 1
    }

    PanelSwapper {
        id: chatSwapper

        anchors.top: chatTabs.bottom
        anchors.bottom: parent.bottom
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        focus: true
        visible: width > 0
    }

    onPendingMessagesChanged: {
        for (var i = 0; i < dock.model.count; i++) {
            if (dock.model.get(i).panelSource == 'ChatPanel.qml') {
                dock.model.setProperty(i, 'notified', pendingMessages);
                break;
            }
        }
    }

    states: State {
        name: 'no-sidebar'

        PropertyChanges { target: sidebar; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: sidebar; properties: 'width'; duration: appStyle.animation.normalDuration }
    }

    Component {
        id: contactMenu
        Menu {
            id: menu

            property alias jid: vcard.jid
            property bool profileEnabled: (vcard.url != undefined && vcard.url != '')

            onProfileEnabledChanged: menu.model.setProperty(0, 'enabled', profileEnabled)

            VCard {
                id: vcard
            }

            onItemClicked: {
                var item = menu.model.get(index);
                var client = accountModel.clientForJid(jid);
                if (item.action == 'profile') {
                    Qt.openUrlExternally(vcard.url);
                } else if (item.action == 'rename') {
                    dialogSwapper.showPanel('ContactRenameDialog.qml', {jid: jid, rosterManager: client.rosterManager});
                } else if (item.action == 'remove') {
                    dialogSwapper.showPanel('ContactRemoveDialog.qml', {jid: jid, rosterManager: client.rosterManager});
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'profile',
                    'enabled': profileEnabled,
                    'iconSource': 'image://icon/information',
                    'text': qsTr('Show profile')});
                menu.model.append({
                    'action': 'rename',
                    'iconSource': 'image://icon/options',
                    'name': model.name,
                    'text': qsTr('Rename contact')});
                menu.model.append({
                    'action': 'remove',
                    'iconSource': 'image://icon/remove',
                    'name': model.name,
                    'text': qsTr('Remove contact')});
            }
        }
    }

    onDockClicked: {
        chatPanel.state = (chatPanel.state == 'no-sidebar') ? '' : 'no-sidebar';
    }

    Component.onCompleted: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'facebook') {
                chatClients.model.append({facebookAppId: account.username, facebookAccessToken: account.password});
            } else if (account.type == 'xmpp') {
                chatClients.model.append({jid: account.username, password: account.password});
            }
        }
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_W) {
            if (chatSwapper.model.count > 1)
                chatSwapper.model.get(chatSwapper.currentIndex).panel.close();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Left) {
            chatSwapper.decrementCurrentIndex();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Right) {
            chatSwapper.incrementCurrentIndex();
        }
    }
}
