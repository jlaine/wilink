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
import 'utils.js' as Utils

Plugin {
    name: 'Wifirst'
    summary: qsTr('Access Wifirst services.')
    description: qsTr('This plugin allows you to access Wifirst services.')
    imageSource: 'wiLink.png'

    Timer {
        id: timer

        repeat: false

        onTriggered: {
            if (Utils.jidToDomain(appClient.jid) != 'wifirst.net')
                return;

            var req = new XMLHttpRequest();
            req.onreadystatechange = function() {
                if (req.readyState == XMLHttpRequest.DONE) {
                    if (req.status != 200) {
                        // retry in 5mn
                        var refresh = 300;
                        console.log("Menu failed, retry in " + refresh + "s");
                        timer.interval = refresh * 1000;
                        timer.start();
                        return;
                    }

                    var root = req.responseXML.documentElement;

                    // parse menu
                    var menu = Utils.getElementsByTagName(root, 'menu')[0];
                    var entries = Utils.getElementsByTagName(menu, 'menu');
                    for (var i in entries) {
                        var image = Utils.getElementsByTagName(entries[i], 'image')[0].firstChild.nodeValue;
                        var label = Utils.getElementsByTagName(entries[i], 'label')[0].firstChild.nodeValue;
                        var link = Utils.getElementsByTagName(entries[i], 'link')[0].firstChild.nodeValue;
                        //console.log("Menu entry '" + label + "' -> " + link);

                        // check for "home" chat room
                        var roomCap = link.match(/xmpp:([^?]+)\?join/);
                        if (roomCap) {
                            var panel = swapper.findPanel('ChatPanel.qml');
                            panel.rooms.addRoom(roomCap[1]);
                        }
                    }

                    // parse preferences
                    var prefs = Utils.getElementsByTagName(root, 'preferences')[0];
                    var refresh = parseInt(Utils.getElementsByTagName(prefs, 'refresh')[0].firstChild.nodeValue);
                    if (refresh > 0) {
                        console.log("Menu succeeded, refresh in " + refresh + "s");
                        timer.interval = refresh * 1000;
                        timer.start();
                    }
                }
            }
            req.open('GET', 'https://www.wifirst.net/wilink/menu/1');
            req.setRequestHeader('Accept', 'application/xml');
            req.send();
        }
    }

    Connections {
        target: appClient
        onConnected: timer.triggered()
    }
}

