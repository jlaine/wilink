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

import QtQuick 1.1
import wiLink 2.0

NotificationDialog {
    id: dialog

    property alias jid: vcard.jid
    property Item panel
    property string roomJid

    iconSource: vcard.avatar
    text: qsTr("%1 has invited you to join the '%2' chat room.\n\nDo you accept?").replace('%1', vcard.name).replace('%2', roomJid)
    title: qsTr('Invitation from %1').replace('%1', vcard.name)

    VCard {
        id: vcard
    }

    onAccepted: {
        panel.showRoom(roomJid);
        dialog.close();
    }
}

