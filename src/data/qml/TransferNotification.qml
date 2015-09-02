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

import QtQuick 2.3
import wiLink 2.4
import 'scripts/utils.js' as Utils

NotificationDialog {
    id: dialog

    property QtObject job
    property Item panel

    iconStyle: 'icon-file'
    text: Qt.isQtObject(job) ? qsTr("%1 wants to send you a file called '%2' (%3).\n\nDo you accept?").replace('%1', vcard.name).replace('%2', job.fileName).replace('%3', Utils.formatSize(job.fileSize)) : ''
    title: qsTr('File from %1').replace('%1', vcard.name)

    resources: [
        VCard {
            id: vcard
            jid: Qt.isQtObject(job) ? job.jid : ''
        },
        Connections {
            target: job
            onStateChanged: dialog.close()
        }
    ]

    onAccepted: {
        // FIXME: this silently overwrite existing files!
        var path = appSettings.downloadsLocation + '/' + job.fileName;
        console.log("File accepted: " + path);
        panel.showConversation(Utils.jidToBareJid(job.jid));
        job.accept(path);

        dialog.close();
    }

    onRejected: job.abort()
}

