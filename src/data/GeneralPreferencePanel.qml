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

Panel {
    id: panel

    function save() {
        if (openAtLogin.enabled)
            application.settings.openAtLogin = openAtLogin.checked;
        application.settings.incomingMessageNotification = incomingMessageNotification.checked;
        application.settings.showOfflineContacts = showOfflineContacts.checked;
        application.settings.sortContactsByStatus = sortContactsByStatus.checked;
    }

    color: 'transparent'

    GroupBox {
        id: general

        anchors.fill: parent
        title: qsTr('General options')

        Column {
            anchors.fill: general.contents
            spacing: appStyle.spacing.vertical

            CheckBox {
                id: openAtLogin

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.settings.openAtLogin
                enabled: application.isInstalled
                text: qsTr('Open at login')
                onClicked: checked = !checked
            }

            CheckBox {
                id: showOfflineContacts

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.settings.showOfflineContacts
                text: qsTr('Show offline contacts')
                onClicked: checked = !checked
            }

            CheckBox {
                id: sortContactsByStatus

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.settings.sortContactsByStatus
                text: qsTr('Sort contacts by status')
                onClicked: checked = !checked
            }

            CheckBox {
                id: incomingMessageNotification

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.settings.incomingMessageNotification
                text: qsTr('Display a notification for incoming messages')
                onClicked: checked = !checked
            }
        }
    }
}

