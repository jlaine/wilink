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

.pragma library

function formatNumber(size) {
    var KILO = 1000;
    var MEGA = KILO * 1000;
    var GIGA = MEGA * 1000;
    var TERA = GIGA * 1000;

    if (size < KILO)
        return size + ' ';
    else if (size < MEGA)
        return Math.floor(size / KILO) + ' K';
    else if (size < GIGA)
        return Math.floor(size / MEGA) + ' M';
    else if (size < TERA)
        return Math.floor(size / GIGA) + ' G';
    else
        return Math.floor(size / TERA) + ' T';
}

function formatSpeed(size) {
    return formatNumber(size * 8) + 'b/s';
}

function formatSize(size) {
    return formatNumber(size) + 'B';
}

function jidToBareJid(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return jid;
    return jid.substring(0, pos);
}

function jidToDomain(jid) {
    var bareJid = jidToBareJid(jid);
    var pos = bareJid.indexOf('@');
    if (pos < 0)
        return bareJid;
    return bareJid.substring(pos + 1);
}

function jidToResource(jid) {
    var pos = jid.indexOf('/');
    if (pos < 0)
        return '';
    return jid.substring(pos + 1);
}

function jidToUser(jid) {
    var pos = jid.indexOf('@');
    if (pos < 0)
        return '';
    return jid.substring(0, pos);
}

// There's no getElementsByTagName method in QML
// ([http://www.mail-archive.com/qt-qml@trolltech.com/msg01024.html]), so we'll
// implement our own version.
function getElementsByTagName(rootElement, tagName) {
    var childNodes = rootElement.childNodes;
    var elements = [];
    for (var i = 0; i < childNodes.length; i++) {
        if (childNodes[i].nodeName == tagName) {
            elements.push(childNodes[i]);
        }
    }
    return elements;
}
