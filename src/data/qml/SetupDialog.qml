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

    AccountModel {
        id: accountModel
    }

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

        Label {
            id: statusLabel

            property string authErrorText: qsTr('Could not connect, please check your username and password.')
            property string unknownErrorText: qsTr('An unknown error occured.')
            property string incompleteText: qsTr('Please enter a valid username and password.')
            property string testingText: qsTr('Checking your username and password..')

            anchors.top: passwordRow.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            wrapMode: Text.WordWrap
        }
    }

    onAccepted: {
        if (!usernameInput.acceptableInput || !passwordInput.acceptableInput) {
            dialog.state = 'incomplete';
            return;
        }
        dialog.state = 'testing';

        var webUsername = usernameInput.text;
        var webPassword = passwordInput.text;
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    var jid, password;
                    var doc = xhr.responseXML.documentElement;
                    for (var i = 0; i < doc.childNodes.length; ++i) {
                        var node = doc.childNodes[i];
                        if (node.nodeName == 'id') {
                            jid = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'password') {
                            password = node.firstChild.nodeValue;
                        }
                    }
                    if (jid && password) {
                        accountModel.append({type: 'chat', username: jid, password: password, realm: Utils.jidToDomain(jid)});
                        accountModel.append({type: 'web', username: webUsername, password: webPassword, realm: 'www.wifirst.net'});
                        accountModel.submit();
                        dialog.close();
                    } else {
                        dialog.state = 'unknownError';
                    }
                } else if (xhr.status == 401) {
                    dialog.state = 'authError';
                } else {
                    dialog.state = 'unknownError';
                }
            }
        }
        console.log("try u:" + webUsername + ", p:" + webPassword);
        xhr.open('GET', 'https://dev.wifirst.net/w/wilink/credentials', true, webUsername, webPassword);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.send();
    }

    states: [
        State {
            name: 'authError'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.authErrorText }
        },

        State {
            name: 'unknownError'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.unknownErrorText }
        },

        State {
            name: 'incomplete'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.incompleteText }
        },

        State {
            name: 'testing'
            PropertyChanges { target: statusLabel; text: statusLabel.testingText }
            PropertyChanges { target: usernameInput; enabled: false }
            PropertyChanges { target: passwordInput; enabled: false }
        }
    ]

    onActiveFocusChanged: {
        if (activeFocus && !usernameInput.activeFocus) {
            usernameInput.forceActiveFocus();
        }
    }

    onRejected: application.quit()

    Keys.onReturnPressed: dialog.accepted()

    Keys.onEscapePressed: dialog.rejected()

    Keys.onTabPressed: {
        if (usernameInput.activeFocus)
            passwordInput.forceActiveFocus();
        else
            usernameInput.forceActiveFocus();
    }
}
