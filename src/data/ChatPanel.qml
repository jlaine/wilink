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
        swapper.showPanel('ChatPanel.qml');
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
            }
            title: qsTr('My rooms')
            height: 150

            Connections {
                onAddClicked: {
                    dialog.source = 'RoomJoinDialog.qml';
                    dialog.item.panel = chatPanel;
                    dialog.show();
                }

                onCurrentJidChanged: {
                    rooms.model.clearPendingMessages(rooms.currentJid);
                }

                onItemClicked: showRoom(model.jid)
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
                        position = Math.min(position, left.height - splitter.height)
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

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: statusBar.top
            currentJid: (Qt.isQtObject(chatSwapper.currentItem) && chatSwapper.currentItem.jid != undefined) ? chatSwapper.currentItem.jid : ''
            model: SortFilterProxyModel {
                dynamicSortFilter: true
                filterRole: RosterModel.SortingRole
                filterRegExp: application.showOfflineContacts ? /.*/ : /^(?!offline).+/
                sortCaseSensitivity: Qt.CaseInsensitive
                sortRole: application.sortContactsByStatus ? RosterModel.SortingRole : RosterModel.NameRole
                sourceModel: window.rosterModel
                Component.onCompleted: sort(0)
            }
            title: qsTr('My contacts')

            Menu {
                id: menu
                opacity: 0
            }

            Connections {
                onAddClicked: {
                    var domain = Utils.jidToDomain(window.client.jid);

                    dialog.source = 'InputDialog.qml';
                    dialog.item.title = qsTr('Add a contact');
                    if (domain == 'wifirst.net') {
                        dialog.item.helpText = qsTr('<b>Tip</b>: your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>').replace('%1', 'https://www.wifirst.net/w/friends?from=wiLink');
                    }
                    dialog.item.labelText = qsTr('Enter the address of the contact you want to add.');
                    dialog.item.textValue = (domain != '') ? '@' + domain : '';
                    dialog.item.accepted.connect(function() {
                        var jid = dialog.item.textValue;
                        if (jid.match(/^[^@/]+@[^@/]+$/)) {
                            console.log("Add contact " + jid);
                            window.client.rosterManager.subscribe(jid);
                            dialog.hide();
                        }
                    });
                    dialog.show();
                }

                onCurrentJidChanged: {
                    window.rosterModel.clearPendingMessages(contacts.currentJid);
                }

                onItemClicked: {
                    menu.hide();
                    showConversation(model.jid);
                }

                onItemContextMenu: {
                    menu.model.clear()
                    if (model.url != undefined && model.url != '') {
                        menu.model.append({
                            'action': 'profile',
                            'icon': 'diagnostics.png',
                            'text': qsTr('Show profile'),
                            'url': model.url});
                    }
                    menu.model.append({
                        'action': 'rename',
                        'icon': 'options.png',
                        'name': model.name,
                        'text': qsTr('Rename contact'),
                        'jid': model.jid});
                    menu.model.append({
                        'action': 'remove',
                        'icon': 'remove.png',
                        'name': model.name,
                        'text': qsTr('Remove contact'),
                        'jid': model.jid});
                    menu.show(16, point.y - 16);
                }
            }

            Connections {
                target: window.client.rosterManager
                onSubscriptionReceived: {
                    var box = window.messageBox();
                    box.windowTitle = qsTr('Invitation from %1').replace('%1', bareJid);
                    box.text = qsTr('%1 has asked to add you to his or her contact list.\n\nDo you accept?').replace('%1', bareJid);
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

            Connections {
                target: menu
                onItemClicked: {
                    var item = menu.model.get(index);
                    if (item.action == 'profile') {
                        Qt.openUrlExternally(item.url);
                    } else if (item.action == 'rename') {
                        dialog.source = 'InputDialog.qml';
                        dialog.item.title = qsTr('Rename contact');
                        dialog.item.labelText = qsTr("Enter the name for this contact.");
                        dialog.item.textValue = item.name;
                        dialog.item.accepted.connect(function() {
                            console.log("Rename contact " + item.jid + ": " + dialog.item.textValue);
                            window.client.rosterManager.renameItem(item.jid, dialog.item.textValue);
                            dialog.hide();
                        });
                        dialog.show();
                    } else if (item.action == 'remove') {
                        var box = window.messageBox();
                        box.windowTitle = qsTr("Remove contact");
                        box.text = qsTr('Do you want to remove %1 from your contact list?').replace('%1', item.name);
                        box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                        if (box.exec() == QMessageBox.Yes) {
                            console.log("Remove contact " + item.jid);
                            window.client.rosterManager.removeItem(item.jid);
                        }
                    }
                }
            }
        }

        StatusBar {
            id: statusBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 32
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
                box.windowTitle = qsTr('Call from %1').replace('%1', contactName);
                box.text = qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', contactName);
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
                box.windowTitle = qsTr('File from %1').replace('%1', contactName);
                box.text = qsTr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?").replace('%1', contactName).replace('%2', job.fileName).replace('%3', job.fileSize);
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
