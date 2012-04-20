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
import 'utils.js' as Utils

Plugin {
    id: plugin

    property string accountJid

    name: 'Wifirst'
    description: qsTr('This plugin allows you to access Wifirst services.')
    imageSource: 'image://icon/wiLink'

    XmlListModel {
        id: listModel
        query: '/hash/menu/menu'

        XmlRole { name: 'image'; query: 'image/string()' }
        XmlRole { name: 'label'; query: 'label/string()' }
        XmlRole { name: 'link'; query: 'link/string()' }

        onStatusChanged: {
            if (listModel.source == '')
                return;

            switch (status) {
            case XmlListModel.Error:
                // retry in 5mn
                var refresh = 300;
                console.log("Menu failed, retry in " + refresh + "s");
                timer.interval = refresh * 1000;
                timer.start();
                break;
            case XmlListModel.Ready:
                for (var i = 0; i < listModel.count; i++) {
                    // check for "home" chat room
                    var roomCap = listModel.get(i).link.match(/xmpp:([^?]+)\?join/);
                    if (roomCap) {
                        var panel = swapper.findPanel('ChatPanel.qml', {'accountJid': plugin.accountJid});
                        panel.rooms.addRoom(roomCap[1]);
                    }
                }

                // refresh in 15mn
                var refresh = 900;
                console.log("Menu succeeded, refresh in " + refresh + "s");
                timer.interval = refresh * 1000;
                timer.start();
                break;
            }
        }
    }

    Timer {
        id: timer

        repeat: false
        onTriggered: {
            if (listModel.source == '') {
                listModel.source = 'https://www.wifirst.net/wilink/menu/1';
            } else {
                listModel.reload();
            }
        }
    }

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'wifirst') {
                plugin.accountJid = account.jid;

                // FIXME: there is a race condition here, we don't know when
                // the ChatPanel will be ready!
                timer.interval = 2500;
                timer.start();
                break;
            }
        }
    }
}

