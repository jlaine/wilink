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

FocusScope {
    id: root

    focus: true

    Style {
        id: appStyle
    }

    Rectangle {
        color: 'white'
        anchors.fill: parent
    }

    AccountAddPanel {
        id: panel

        anchors.top: parent.top
        anchors.bottom: help.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        domain: 'wifirst.net'
        focus: true
        model: ListModel {}
        opacity: 1

        onAccepted: {
            wallet.set(jid, password);
            application.chatAccounts = [jid];
            application.resetWindows();
        }
        onRejected: application.quit()
    }

    PanelHelp {
        id: help

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8

        text: qsTr('If you need help, please refer to the <a href="%1">%2 FAQ</a>.').replace('%1', 'https://www.wifirst.net/wilink/faq').replace('%2', application.applicationName)
    }

    Wallet {
        id: wallet
    }
}
