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

Item {

    Component.onCompleted: {
        var req = new XMLHttpRequest();
        req.onreadystatechange = function() {
            if (req.readyState == XMLHttpRequest.DONE) {
                var root = req.responseXML.documentElement;

                // parse preferences
                var prefs = Utils.getElementsByTagName(root, 'preferences')[0];
                var refresh = Utils.getElementsByTagName(prefs, 'refresh')[0].firstChild.nodeValue;
                console.log("menu refresh in " + refresh);

                // parse menu
                var menu = Utils.getElementsByTagName(root, 'menu')[0];
                var entries = Utils.getElementsByTagName(menu, 'menu');
                for (var i in entries) {
                    var image = Utils.getElementsByTagName(entries[i], 'image')[0].firstChild.nodeValue;
                    var label = Utils.getElementsByTagName(entries[i], 'label')[0].firstChild.nodeValue;
                    var link = Utils.getElementsByTagName(entries[i], 'link')[0].firstChild.nodeValue;
                    console.log("menu entry '" + label + "' -> " + link);
                }
            }
        }
        console.log("fetching menu");
        req.open('GET', 'https://www.wifirst.net/wilink/menu/1');
        req.setRequestHeader('Accept', 'application/xml');
        req.send();
    }
}

