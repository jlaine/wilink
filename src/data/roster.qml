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

Column {
    id: blocks

    signal itemClicked(string id)

    anchors.fill: parent

    RosterView {
        id: rooms

        model: roomModel
        title: qsTr('My rooms')
        height: parent.height / 3
        width: parent.width

        Connections {
            onItemClicked: { blocks.itemClicked(model.id); }
        }
    }

    RosterView {
        id: contacts

        model: contactModel
        title: qsTr('My contacts')
        height: 2 * parent.height / 3
        width: parent.width

        Menu {
            id: menu
            opacity: 0
        }

        Connections {
            onItemClicked: { blocks.itemClicked(model.id); }
            onItemContextMenu: {
                menu.model.clear()
                if (model.url != undefined && model.url != '') {
                    menu.model.append({
                        'action': 'profile',
                        'title': qsTr('Show profile'),
                        'url': model.url});
                }
                menu.model.append({
                    'action': 'rename',
                    'name': model.name,
                    'title': qsTr('Rename contact'),
                    'jid': model.id});
                menu.model.append({
                    'action': 'remove',
                    'name': model.name,
                    'title': qsTr('Remove contact'),
                    'jid': model.id});
                menu.x = 16;
                menu.y = point.y - 16;
                menu.state = 'visible';
            }
        }

        Connections {
            target: menu
            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'profile') {
                    Qt.openUrlExternally(item.url);
                } else if (item.action == 'rename') {
                    var box = window.messageBox();
                    box.windowTitle = qsTr('Rename contact');
                    box.text = qsTr("Enter the name for this contact.");
                    box.standardButtons = QMessageBox.Yes | QMessageBox.No;
                    if (box.exec() == QMessageBox.Yes) {
                        console.log("rename " + item.jid);
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

}
