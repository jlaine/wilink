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
import wiLink 2.4

NotificationDialog {
    id: dialog

    property alias jid: vcard.jid
    property QtObject rosterManager

    iconStyle: 'icon-question-sign'
    text: qsTr('%1 has asked to add you to his or her contact list.\n\nDo you accept?').replace('%1', jid);
    title: qsTr('Invitation from %1').replace('%1', vcard.name)

    resources: [
        VCard {
            id: vcard
        }
    ]

    onAccepted: {
        console.log("Contact accepted " + jid);

        // accept subscription
        dialog.rosterManager.acceptSubscription(jid);

        // request reciprocal subscription
        dialog.rosterManager.subscribe(jid);

        // close dialog
        dialog.close();
    }

    onRejected: {
        console.log("Contact rejected " + jid);

        // refuse subscription
        dialog.rosterManager.refuseSubscription(jid);
    }
}

