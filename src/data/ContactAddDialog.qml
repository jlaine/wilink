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

InputDialog {
    id: dialog

    property string domain: Utils.jidToDomain(window.client.jid)

    helpText: (domain == 'wifirst.net') ? qsTr('<b>Tip</b>: your wAmis are automatically added to your chat contacts, so the easiest way to add Wifirst contacts is to <a href=\"%1\">add them as wAmis</a>').replace('%1', 'https://www.wifirst.net/w/friends?from=wiLink') : ''
    labelText: qsTr('Enter the address of the contact you want to add.')
    textValue: (domain != '') ? '@' + domain : ''
    title: qsTr('Add a contact')

    onAccepted: {
        var jid = dialog.textValue;
        if (jid.match(/^[^@/]+@[^@/]+$/)) {
            console.log("add " + jid);
            window.client.rosterManager.subscribe(jid);
            dialog.hide();
        }
    }
}

