/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

XmlListModel {
    id: xmlModel

    property url url
    property string username
    property string password

    function reload() {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    xmlModel.xml = xhr.responseText;
                } else {
                    console.log("DirectoryXmlModel failed to load " + xmlModel.url + ": " + xhr.status + "/" + xhr.statusText);
                }
            }
        }
        xhr.open('GET', xmlModel.url, true, xmlModel.username, xmlModel.password);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.send();
    }
}
