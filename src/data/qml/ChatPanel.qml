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
    id: chatPanel

    property alias rooms: roomListModel
    property bool pendingMessages: (roomListModel.pendingMessages + rosterModel.pendingMessages) > 0

    function isFacebook(jid) {
       return Utils.jidToDomain(jid) == 'chat.facebook.com';
    }

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
                    var domain = Utils.jidToDomain(item.client.jid);
                    if (domain != 'wifirst.net' && domain != 'chat.facebook.com') {
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
            } else {
                console.log("connecting to: " + data.jid);
                item.client.connectToServer(data.jid, data.password);
            }
            rosterModel.addClient(item.client);
            roomListModel.addClient(item.client);
            statusBar.addClient(item.client);
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
        anchors.margins: appStyle.margin.normal
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
            height: 32 + rowHeight * (appStyle.isMobile ? 2 : 4)

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
            color: 'transparent'
            height: appStyle.margin.normal

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
                        position = Math.min(position, sidebar.height - splitter.height - statusBar.height - appStyle.margin.normal)
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

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: statusBar.top
            anchors.bottomMargin: appStyle.margin.normal
            currentJid: (Qt.application.active && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: sortedContacts
            title: qsTr('My contacts')

            onAddClicked: {
                var clients = [];
                for (var i = 0; i < chatClients.count; ++i) {
                    var client = chatClients.itemAt(i).client;
                    if (!isFacebook(client.jid))
                        clients.push(client);
                }
                dialogSwapper.showPanel('ContactAddDialog.qml', {clients: clients});
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
            radius: appStyle.margin.normal
        }

        // FIXME : this is a hack to make sure ConversationPanel loads fast
        ConversationPanel {
            opacity: 0
        }

    }

    Rectangle {
        id: tabBox

        anchors.top: parent.top
        anchors.topMargin: appStyle.margin.normal
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        gradient: Gradient {
            GradientStop { position: 0; color: '#FAFAFA' }
            GradientStop { position: 1; color: '#E9E9E9' }
        }
        height: (appStyle.font.largeSize + 2 * appStyle.margin.large)
        property int radius: appStyle.margin.small

        ChatTabs {
            id: tabView

            anchors.fill: parent
            panelSwapper: chatSwapper
            z: 1
        }
    }

    PanelSwapper {
        id: chatSwapper

        anchors.top: tabBox.bottom
        anchors.bottom: parent.bottom
        anchors.left: chatPanel.singlePanel ? parent.left : sidebar.right
        anchors.margins: appStyle.margin.normal
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

    resources: [
        SortFilterProxyModel {
            id: onlineContacts

            dynamicSortFilter: true
            filterRole: RosterModel.StatusRole
            filterRegExp: /^(?!offline)/
            sourceModel: RosterModel {
                id: rosterModel
            }
        },

        SortFilterProxyModel {
            id: sortedContacts

            dynamicSortFilter: true
            sortCaseSensitivity: Qt.CaseInsensitive
            sortRole: appSettings.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
            sourceModel: appSettings.showOfflineContacts ? rosterModel : onlineContacts
            Component.onCompleted: sort(0)
        }
    ]

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
            property bool profileEnabled: (profileUrl != undefined && profileUrl != '')
            property string profileUrl: isFacebook(vcard.jid) ? ('https://www.facebook.com/' + Utils.jidToUser(vcard.jid).substr(1)) : vcard.url

            onProfileEnabledChanged: menu.model.setProperty(0, 'enabled', profileEnabled)

            VCard {
                id: vcard
            }

            onItemClicked: {
                var item = menu.model.get(index);
                var client = accountModel.clientForJid(jid);
                if (item.action == 'profile') {
                    Qt.openUrlExternally(profileUrl);
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
            if (account.type == 'xmpp') {
                chatClients.model.append({jid: account.username, password: account.password});
            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                chatClients.model.append({jid: account.username, password: account.password});
            } else if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                var xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function() {
                    if (xhr.readyState == 4) {
                        if (xhr.status == 200) {
                            var facebookAppId = '',
                                facebookAccessToken = '',
                                xmppJid = '',
                                xmppPassword = '';
                            var doc = xhr.responseXML.documentElement;
                            for (var i = 0; i < doc.childNodes.length; ++i) {
                                var node = doc.childNodes[i];
                                if (node.nodeName == 'id' && node.firstChild) {
                                    xmppJid = node.firstChild.nodeValue;
                                } else if (node.nodeName == 'password' && node.firstChild) {
                                    xmppPassword = node.firstChild.nodeValue;
                                } else if (node.nodeName == 'facebook') {
                                    for (var j = 0; j < node.childNodes.length; ++j) {
                                        var child = node.childNodes[j];
                                        if (child.nodeName == 'app-id' && child.firstChild)
                                            facebookAppId = child.firstChild.nodeValue;
                                        else if (child.nodeName == 'access-token' && child.firstChild)
                                            facebookAccessToken = child.firstChild.nodeValue;
                                    }
                                }
                            }

                            // connect to XMPP
                            if (xmppJid && xmppPassword) {
                                chatClients.model.append({
                                    jid: xmppJid,
                                    password: xmppPassword});
                            }

                            // connect to facebook
                            if (facebookAppId && facebookAccessToken) {
                                chatClients.model.append({
                                    facebookAppId: facebookAppId,
                                    facebookAccessToken: facebookAccessToken});
                            }
                        }
                    }
                };
                xhr.open('GET', 'https://www.wifirst.net/w/wilink/credentials', true, account.username, account.password);
                xhr.setRequestHeader('Accept', 'application/xml');
                xhr.send();
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
