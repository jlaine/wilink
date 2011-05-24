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

Item {

    Component.onCompleted: {
        var doc = new XMLHttpRequest();
        doc.onreadystatechange = function() {
            if (doc.readyState == XMLHttpRequest.DONE) {
                var a = doc.responseXML.documentElement;
                for (var ii = 0; ii < a.childNodes.length; ++ii) {
                    console.log(a.childNodes[ii].nodeName);
                }
                console.log("Headers -->");
                console.log(doc.getAllResponseHeaders ());
                console.log("Last modified -->");
                console.log(doc.getResponseHeader ("Last-Modified"));
            }
        }
        console.log("fetching menu");
        doc.open('GET', 'https://www.wifirst.net/wilink/menu/1');
        doc.setRequestHeader('Accept', 'application/xml');
        doc.send();
    }
}

