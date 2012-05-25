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

XmlListModel {
    id: xmlModel

    property url url
    property string username
    property string password

    function reload() {
        console.log("reload");
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    console.log(xhr.responseText);
                    xmlModel.xml = xhr.responseText;
                }
            }
        };
        xhr.open('GET', xmlModel.url, true, xmlModel.username, xmlModel.password);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.send();
    }

    function addContact(name, phone) {
        console.log("add contact '" + name + "' (" + phone + ")");
        var data = 'name=' + encodeURIComponent(name);
        data += '&phone=' + encodeURIComponent(phone);

        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            console.log("xhr.readyState " + xhr.readyState);
            // FIXME: for some reason, we never get past this state
            if (xhr.readyState == 3) {
                xhr.abort();
                xmlModel.reload();
            }
        };
        xhr.open('POST', xmlModel.url, true, xmlModel.username, xmlModel.password);
        xhr.setRequestHeader("Accept", "application/xml");
        xhr.setRequestHeader("Connection", "close");
        xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr.send(data);
    }

    function removeContact(id) {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    console.log("done");
                    xmlModel.reload();
                } else {
                    console.log("Failed to delete contact: " + id + " " + xhr.status + "/" + xhr.statusText);
                }
            }
        }
        // FIXME: why does QML not like the "DELETE" method?
        var url = xmlModel.url + id + '?_method=delete';
        console.log("POST url: " + url);
        xhr.open('POST', url, true, xmlModel.username, xmlModel.password);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.send();
    }

    function updateContact(id, name, phone) {
        console.log("update contact " +id + " '" + name + "' (" + phone + ")");
    }

    query: '/contacts/contact'

    XmlRole { name: 'id'; query: 'id/string()'; isKey: true }
    XmlRole { name: 'name'; query: 'name/string()' }
    XmlRole { name: 'phone'; query: 'phone/string()' }

    onStatusChanged: {
        console.log("status:" + xmlModel.status + " " + xmlModel.errorString());
    }
}
