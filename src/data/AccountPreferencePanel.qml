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
    id: panel

    function save() {
        var newJids = [];

        // save passwords for new accounts
        for (var i = 0; i < listPanel.model.count; i++) {
            var entry = listPanel.model.get(i);
            if (entry.password != undefined) {
                wallet.set(entry.jid, entry.password);
            }
            newJids.push(entry.jid);
        }

        // remove password for removed accounts
        var oldJids = application.settings.chatAccounts;
        for (var i = 0; i < oldJids.length; i++) {
            var jid = oldJids[i];
            if (newJids.indexOf(jid) < 0) {
                wallet.remove(jid);
            }
        }

        // save accounts
        application.settings.chatAccounts = newJids;
        application.resetWindows();
    }

    color: 'transparent'

    GroupBox {
        id: accounts

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Chat accounts')

        AccountListPanel {
            id: listPanel

            anchors.fill: accounts.contents

            onAddClicked: {
                panel.state = 'edit';
                addPanel.forceActiveFocus();
            }
        }

        AccountAddPanel {
            id: addPanel

            anchors.fill: accounts.contents
            model: listPanel.model
            opacity: 0

            onAccepted: {
                listPanel.model.append({'jid': jid, 'password': password});
                addPanel.close();
            }
            onClose: panel.state = ''
        }
    }

    Wallet {
        id: wallet
    }
 
    states: State {
        name: 'edit'
        PropertyChanges { target: listPanel; opacity: 0 }
        PropertyChanges { target: addPanel; opacity: 1 }
        StateChangeScript { script: addPanel.forceActiveFocus() }
    }
}

