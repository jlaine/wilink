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

Item {
    id: root

    ListModel {
        id: panels
    }

    /** Convenience method to show a conversation panel.
     */
    function showConversation(jid) {
        showPanel('ConversationPanel.qml', {'jid': Utils.jidToBareJid(jid)});
    }

    /** Convenience method to show a chat room panel.
     */
    function showRoom(jid) {
        showPanel('RoomPanel.qml', {'jid': jid})
    }

    function showPanel(source, properties) {
        if (properties == undefined)
            properties = {};

        function propDump(a) {
            var dump = '';
            for (var key in a)
                dump += (dump.length > 0 ? ', ' : '') + key + ': ' + a[key];
            return '{' + dump + '}';
        }

        function propEquals(a, b) {
            if (a.length != b.length)
                return false;
            for (var key in a) {
                if (a[key] != b[key])
                    return false;
            }
            return true;
        }

        // if the panel already exists, show it
        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).source == source &&
                propEquals(panels.get(i).properties, properties)) {
                console.log("focusing panel " + source + " " + propDump(properties));
                swapper.setCurrentItem(panels.get(i).panel);
                return;
            }
        }

        // otherwise create the panel
        console.log("creating panel " + source + " " + propDump(properties));
        var component = Qt.createComponent(source);
        // FIXME: when Qt Quick 1.1 becomes available,
        // let createObject assign the properties itself.
        var panel = component.createObject(swapper);
        for (var key in properties) {
            panel[key] = properties[key];
        }

        panel.close.connect(function() {
            var wasVisible = (panel.opacity > 0);
            for (var i = 0; i < panels.count; i += 1) {
                if (panels.get(i).panel == panel) {
                    console.log("removing panel " + panels.get(i).source + " " + propDump(panels.get(i).properties));

                    // if the panel was visible, show last remaining panel
                    if (swapper.currentItem == panel) {
                        if (panels.count == 1)
                            swapper.setCurrentItem(null);
                        else if (i == panels.count - 1)
                            swapper.setCurrentItem(panels.get(i - 1).panel);
                        else
                            swapper.setCurrentItem(panels.get(i + 1).panel);
                    }

                    // destroy panel
                    panels.remove(i);
                    panel.destroy();
                    break;
                }
            }

        })
        panels.append({'source': source, 'properties': properties, 'panel': panel});
        swapper.setCurrentItem(panel);
        return panel;
    }

    Item {
        id: left

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 250

        Rectangle {
            id: toolbarBackground

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            color: '#6ea1f1'
            width: 24
        }

        Rectangle {
            id: toolbar

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            z: 1

            Column {
                id: control
                anchors.top: parent.top
                anchors.left: parent.left

                ToolButton {
                    color: '#6ea1f1'
                    icon: 'diagnostics.png'
                    onClicked: showPanel('DiagnosticPanel.qml')
                    text: state != '' ? qsTr('Diagnostics') : ''
                }

/*
                ToolButton {
                    text: 'Debugging'
                    icon: 'options.png'
                    onClicked: showPanel('LogPanel.qml')
                }

                ToolButton {
                    text: 'Discovery'
                    icon: 'options.png'
                    onClicked: showPanel('DiscoveryPanel.qml')
                }

                ToolButton {
                    text: 'Media'
                    icon: 'start.png'
                    onClicked: showPanel('PlayerPanel.qml')
                }
*/

                ToolButton {
                    color: '#6ea1f1'
                    icon: 'phone.png'
                    onClicked: showPanel('PhonePanel.qml')
                    text: state != '' ? qsTr('Phone') : ''
                }

                ToolButton {
                    color: '#6ea1f1'
                    icon: 'photos.png'
                    onClicked: {
                        var domain = Utils.jidToDomain(window.client.jid);
                        if (domain == 'wifirst.net')
                            showPanel('PhotoPanel.qml', {'url': 'wifirst://www.wifirst.net/w'});
                        else if (domain == 'gmail.com')
                            showPanel('PhotoPanel.qml', {'url': 'picasa://default'});
                    }
                    text: state != '' ? qsTr('Photos') : ''
                }

                ToolButton {
                    color: '#6ea1f1'
                    icon: 'share.png'
                    text: state != '' ? qsTr('Shares') : ''
                    visible: window.client.shareServer != ''
                }
            }
        }

        Loader {
            id: dialog
            z: 10
        }

        RosterView {
            id: rooms

            anchors.left: toolbarBackground.right
            anchors.right: parent.right
            anchors.top: parent.top
            currentJid: swapper.currentItem ? swapper.currentItem.jid : ''
            enabled: window.client.mucServer != ''
            model: RoomListModel {
                client: window.client
            }
            title: qsTr('My rooms')
            height: 150

            Connections {
                onAddClicked: {
                    dialog.source = 'RoomJoinDialog.qml';
                    dialog.item.show();
                }
                onItemClicked: showRoom(model.jid)
            }
        }

        Rectangle {
            id: splitter

            anchors.left: toolbarBackground.right
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

            anchors.left: toolbarBackground.right
            anchors.right: parent.right
            anchors.top: splitter.bottom
            anchors.bottom: parent.bottom
            currentJid: swapper.currentItem ? swapper.currentItem.jid : ''
            model: contactModel
            title: qsTr('My contacts')

            Menu {
                id: menu
                opacity: 0
            }

            Connections {
                onAddClicked: {
                    var domain = Utils.jidToDomain(window.client.jid);
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
                    window.client.rosterManager.subscribe(jid);
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
                        var dialog = window.inputDialog();
                        dialog.windowTitle = qsTr('Rename contact');
                        dialog.labelText = qsTr("Enter the name for this contact.");
                        dialog.textValue = item.name;
                        if (dialog.exec()) {
                            console.log("rename " + item.jid + ": " + dialog.textValue);
                            window.client.rosterManager.renameItem(item.jid, dialog.textValue);
                        }
                    } else if (item.action == 'remove') {
                        var box = window.messageBox();
                        box.windowTitle = qsTr("Remove contact");
                        box.text = qsTr('Do you want to remove %1 from your contact list?').replace('%1', item.name);
                        box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                        if (box.exec() == QMessageBox.Yes) {
                            console.log("remove " + item.jid);
                            window.client.rosterManager.removeItem(item.jid);
                        }
                    }
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
                    showConversation(job.jid);
                    job.accept();
                } else {
                    job.abort();
                }
            }
        }
    }

    Item {
        id: swapper

        property Item currentItem

        function setCurrentItem(panel) {
            if (panel == currentItem)
                return;

            // show new item
            if (panel)
                panel.opacity = 1;
            else
                background.opacity = 1;

            // hide old item
            if (currentItem)
                currentItem.opacity = 0;
            else
                background.opacity = 0;

            currentItem = panel;
        }

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: left.right
        anchors.right: parent.right

        Rectangle {
            id: background
            anchors.fill: parent
        }
    }
}
