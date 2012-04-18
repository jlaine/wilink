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
import QXmpp 0.4
import wiLink 2.0
import 'utils.js' as Utils

Panel {
    id: chatPanel

    property string accountJid
    property alias client: appClient
    property alias rooms: roomListModel
    property bool pendingMessages: (roomListModel.pendingMessages + rosterModel.pendingMessages) > 0

    Client {
        id: appClient

        logger: appLogger

        onAuthenticationFailed: {
            console.log("Failed to authenticate with chat server");
            var jid = Utils.jidToBareJid(appClient.jid);
            dialogSwapper.showPanel('AccountPasswordDialog.qml', {client: appClient, jid: jid});
        }

        onConflictReceived: {
            console.log("Received a resource conflict from chat server");
            application.quit();
        }
    }

    /** Convenience method to show a conversation panel.
     */
    function showConversation(jid) {
        swapper.showPanel('ChatPanel.qml', {accountJid: chatPanel.accountJid});
        chatSwapper.showPanel('ConversationPanel.qml', {'jid': Utils.jidToBareJid(jid)});
        if (chatPanel.singlePanel)
            chatPanel.state = 'no-sidebar';
    }

    /** Convenience method to show a chat room panel.
     */
    function showRoom(jid) {
        swapper.showPanel('ChatPanel.qml', {accountJid: chatPanel.accountJid});
        chatSwapper.showPanel('RoomPanel.qml', {'jid': jid})
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
            currentJid: (window.isActiveWindow && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            enabled: appClient.mucServer != ''
            iconSource: 'chat.png'
            model: RoomListModel {
                id: roomListModel
                client: appClient

                onRoomAdded: {
                    if (!chatSwapper.findPanel('RoomPanel.qml', {'jid': jid})) {
                        if (chatSwapper.currentItem) {
                            chatSwapper.addPanel('RoomPanel.qml', {'jid': jid});
                        } else {
                            chatSwapper.showPanel('RoomPanel.qml', {'jid': jid});
                        }
                    }
                }
            }
            title: qsTr('My rooms')
            // FIXME: in Qt 4.7.4, making height depend on count gives a buggy behaviour
            // height: headerHeight + (count > 0 ? 4 : 0) + rowHeight * Math.min(count, 8);
            height: headerHeight + 4 + rowHeight * (appStyle.isMobile ? 2 : 4)

            onAddClicked: {
                dialogSwapper.showPanel('RoomJoinDialog.qml', {client: appClient});
            }

            onCurrentJidChanged: {
                rooms.model.clearPendingMessages(rooms.currentJid);
            }

            onItemClicked: showRoom(model.jid)

            Connections {
                target: appClient.mucManager

                onInvitationReceived: {
                    dialogSwapper.showPanel('RoomInviteNotification.qml', {
                        'jid': Utils.jidToBareJid(inviter),
                        'panel': chatPanel,
                        'roomJid': roomJid});
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
                    client: appClient
                }
            }

            SortFilterProxyModel {
                id: sortedContacts

                dynamicSortFilter: true
                sortCaseSensitivity: Qt.CaseInsensitive
                sortRole: application.settings.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
                sourceModel: application.settings.showOfflineContacts ? rosterModel : onlineContacts
                Component.onCompleted: sort(0)
            }

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: statusBar.top
            currentJid: (window.isActiveWindow && swapper.currentItem == chatPanel && Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: sortedContacts
            title: qsTr('My contacts')

            onAddClicked: {
                dialogSwapper.showPanel('ContactAddDialog.qml', {client: appClient});
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

            Connections {
                target: appClient.rosterManager
                onSubscriptionReceived: {
                    // If we have a subscription to the requester, accept
                    // reciprocal subscription.
                    //
                    // FIXME: use QXmppRosterIq::Item::To and QXmppRosterIq::Item::Both
                    var subscription = appClient.subscriptionType(bareJid);
                    if (subscription == 2 || subscription == 3) {
                        // accept subscription
                        appClient.rosterManager.acceptSubscription(bareJid);
                        return;
                    }

                    dialogSwapper.showPanel('ContactAddNotification.qml', {jid: bareJid, rosterManager: appClient.rosterManager});
                }
            }
        }

        StatusBar {
            id: statusBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            client: appClient
        }

        // FIXME : this is a hack to replay received messages after
        // adding the appropriate conversation
        Connections {
            target: appClient
            onMessageReceived: {
                var jid = Utils.jidToBareJid(from);
                if (!chatSwapper.findPanel('ConversationPanel.qml', {'jid': jid})) {
                    chatSwapper.addPanel('ConversationPanel.qml', {'jid': jid});
                    appClient.replayMessage();
                }
            }
        }

        Connections {
            target: appClient.callManager
            onCallReceived: {
                dialogSwapper.showPanel('CallNotification.qml', {
                    'call': call,
                    'panel': chatPanel});
            }
        }

        Connections {
            target: appClient.transferManager
            onFileReceived: {
                dialogSwapper.showPanel('TransferNotification.qml', {
                    'job': job,
                    'panel': chatPanel});
            }
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
            if (dock.model.get(i).panelSource == 'ChatPanel.qml' &&
                dock.model.get(i).panelProperties.accountJid == chatPanel.accountJid) {
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
                if (item.action == 'profile') {
                    Qt.openUrlExternally(vcard.url);
                } else if (item.action == 'rename') {
                    dialogSwapper.showPanel('ContactRenameDialog.qml', {jid: jid, rosterManager: appClient.rosterManager});
                } else if (item.action == 'remove') {
                    dialogSwapper.showPanel('ContactRemoveDialog.qml', {jid: jid, rosterManager: appClient.rosterManager});
                }
            }

            Component.onCompleted: {
                menu.model.append({
                    'action': 'profile',
                    'enabled': profileEnabled,
                    'icon': 'information.png',
                    'text': qsTr('Show profile')});
                menu.model.append({
                    'action': 'rename',
                    'icon': 'options.png',
                    'name': model.name,
                    'text': qsTr('Rename contact')});
                menu.model.append({
                    'action': 'remove',
                    'icon': 'remove.png',
                    'name': model.name,
                    'text': qsTr('Remove contact')});
            }
        }
    }

    onAccountJidChanged: {
        var jid = chatPanel.accountJid;
        var password = accountModel.getPassword(jid);
        appClient.connectToServer(jid, password);
    }

    onDockClicked: {
        chatPanel.state = (chatPanel.state == 'no-sidebar') ? '' : 'no-sidebar';
    }

    Keys.onPressed: {
        if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Left) {
            chatSwapper.decrementCurrentIndex();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Right) {
            chatSwapper.incrementCurrentIndex();
        }
    }
}
