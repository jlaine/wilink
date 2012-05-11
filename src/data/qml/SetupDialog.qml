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
import 'utils.js' as Utils

Dialog {
    id: dialog

    title: qsTr('Add an account')


    Item {
        anchors.fill: dialog.contents

        PanelHelp {
            id: help

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("Enter the username and password for your '%1' account.").replace('%1', 'wifirst.net')
        }

        Item {
            id: usernameRow

            anchors.top: help.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            height: usernameInput.height

            Label {
                id: usernameLabel

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                font.bold: true
                text: qsTr('Username')
                width: 100
            }

            InputBar {
                id: usernameInput

                anchors.top: parent.top
                anchors.left: usernameLabel.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                validator: RegExpValidator {
                    regExp: /.+/
                }
            }
        }

        Item {
            id: passwordRow

            anchors.top: usernameRow.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            height: passwordInput.height

            Label {
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
    }

    onAccepted: {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', 'https://dev.wifirst.net/w/wilink/credentials', true, usernameInput.text, passwordInput.text);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                console.log("zob: " + xhr.status);
                console.log("zob text: " + xhr.responseText);
            }
        }
        xhr.send();
    }

    onRejected: application.quit()
}
