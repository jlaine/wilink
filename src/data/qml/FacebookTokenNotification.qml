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

    iconStyle: 'icon-facebook'
    text: qsTr('In order to chat with your Facebook friends, you need to add the Wifirst Facebook application then restart wiLink.\n\nDo you accept?')
    title: qsTr('Access to your Facebook chat')

    onAccepted: {
        Qt.openUrlExternally("https://apps.wifirst.net/auth/facebook");
        dialog.close();
    }
}

