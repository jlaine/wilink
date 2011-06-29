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

Dialog {
    id: dialog

    property string jid

    minimumHeight: 250
    title: qsTr('Password required')

    Column {
        anchors.fill: contents
        spacing: appStyle.spacing.vertical

        PanelHelp {
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("Enter the password for your '%1' account.").replace('%1', jid)
        }

        Item {
            id: passwordRow

            anchors.left: parent.left
            anchors.right: parent.right
            height: passwordInput.height

            Text {
                id: passwordLabel

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                font.bold: true
                text: qsTr('Password')
                width: 100
            }

            InputBar {
                id: passwordInput

                anchors.top: parent.top
                anchors.left: passwordLabel.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                echoMode: TextInput.Password
                validator: RegExpValidator {
                    regExp: /.+/
                }
            }
        }

        PanelHelp {
            id: help

            anchors.left: parent.left
            anchors.right: parent.right

            text: qsTr('If you need help, please refer to the <a href="%1">%2 FAQ</a>.').replace('%1', 'https://www.wifirst.net/wilink/faq').replace('%2', application.applicationName)
        }
    }

    onAccepted: {
        var password = passwordInput.text;
        appWallet.set(dialog.jid, password);
        appClient.connectToServer(dialog.jid, password);
        dialogLoader.hide();
    }
}

