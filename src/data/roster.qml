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

Item {
    id: root

    Item {
        id: left

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 220

        Rectangle {
            id: toolbar

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 46
            gradient: Gradient {
                GradientStop { position: 0; color: '#6ea1f1' }
                GradientStop { position: 1; color: '#567dbc' }
            }

            Row {
                anchors.top: parent.top
                anchors.left: parent.left

                ToolButton {
                    text: 'Phone'
                    icon: 'phone.png'
                    onClicked: Qt.openUrlExternally('sip://')
                }

                ToolButton {
                    text: 'Photos'
                    icon: 'photos.png'
                    visible: false
                }

                ToolButton {
                    text: 'Shares'
                    icon: 'share.png'
                    visible: client.shareServer != ''
                }
            }
        }

        Loader {
            id: dialog
            z: 10
        }

        RosterView {
            id: rooms

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: toolbar.bottom
            enabled: client.mucServer != ''
            model: roomModel
            title: qsTr('My rooms')
            height: 150

            Connections {
                onAddClicked: {
                    dialog.source = 'RoomJoinDialog.qml';
                    dialog.item.show();
                }
                onItemClicked: {
                    var url = 'xmpp://' + window.objectName + '/' + model.jid + '?join';
                    Qt.openUrlExternally(url);
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
            anchors.bottom: parent.bottom
            model: contactModel
            title: qsTr('My contacts')

            Menu {
                id: menu
                opacity: 0
            }

            Connections {
                onAddClicked: {
                    var domain = window.objectName.split('@')[1];
                    var tip = (domain == 'wifirst.net') ? '<p>' + qsTr('<b>Tip</b>: your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>').replace('%1', 'https://www.wifirst.net/w/friends?from=wiLink') + '</p>' : '';

                    var dialog = window.inputDialog();
                    dialog.windowTitle = qsTr('Add a contact');
                    dialog.labelText = tip + '<p>' + qsTr('Enter the address of the contact you want to add.') + '</p>';
                    dialog.textValue = '@' + domain;

                    var jid = '';
                    while (!jid.match(/^[^@/]+@[^@/]+$/)) {
                        if (!dialog.exec())
                            return;
                        jid = dialog.textValue;
                    }
                    console.log("add " + jid);
                    client.rosterManager.subscribe(jid);
                }

                onItemClicked: {
                    menu.hide();
                    var url = 'xmpp://' + window.objectName + '/' + model.jid + '?message';
                    Qt.openUrlExternally(url);
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
                target: client.rosterManager
                onSubscriptionReceived: {
                    var box = window.messageBox();
                    box.windowTitle = qsTr('Invitation from %1').replace('%1', bareJid);
                    box.text = qsTr('%1 has asked to add you to his or her contact list.\n\nDo you accept?').replace('%1', bareJid);
                    box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                    if (box.exec() == QMessageBox.Yes) {
                        // accept subscription
                        client.rosterManager.acceptSubscription(bareJid);

                        // request reciprocal subscription
                        client.rosterManager.subscribe(bareJid);
                    } else {
                        // refuse subscription
                        client.rosterManager.refuseSubscription(bareJid);
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
                        var dialog = window.inputDialog();
                        dialog.windowTitle = qsTr('Rename contact');
                        dialog.labelText = qsTr("Enter the name for this contact.");
                        dialog.textValue = item.name;
                        if (dialog.exec()) {
                            console.log("rename " + item.jid + ": " + dialog.textValue);
                            client.rosterManager.renameItem(item.jid, dialog.textValue);
                        }
                    } else if (item.action == 'remove') {
                        var box = window.messageBox();
                        box.windowTitle = qsTr("Remove contact");
                        box.text = qsTr('Do you want to remove %1 from your contact list?').replace('%1', item.name);
                        box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                        if (box.exec() == QMessageBox.Yes) {
                            console.log("remove " + item.jid);
                            client.rosterManager.removeItem(item.jid);
                        }
                    }
                }
            }
        }

        Connections {
            target: client.callManager
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
                    var url = 'xmpp://' + window.objectName + '/' + call.jid.split('/')[0] + '?message';
                    Qt.openUrlExternally(url);
                    call.accept();
                } else {
                    call.hangup();
                }
            }
        }

        Connections {
            target: client.transferManager
            onFileReceived: {
                var contactName = job.jid;
                console.log("File received: " + contactName);

                // prompt user
                var box = window.messageBox();
                box.windowTitle = qsTr('File from %1').replace('%1', contactName);
                box.text = qsTr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?").replace('%1', contactName).replace('%2', job.fileName).replace('%3', job.fileSize);
                box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                if (box.exec()) {
                    var url = 'xmpp://' + window.objectName + '/' + job.jid.split('/')[0] + '?message';
                    Qt.openUrlExternally(url);
                    job.accept();
                } else {
                    job.abort();
                }
            }
        }
    }

    Item {
        id: swapper

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: left.right
        anchors.right: parent.right
    }
}
