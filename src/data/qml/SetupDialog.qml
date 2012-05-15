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

    property string webRealm
    property string webUsername
    property string webPassword
    property string xmppUsername
    property string xmppPassword

    AccountModel {
        id: accountModel
    }

    Client {
        id: testClient

        function testCredentials(jid, password) {
            if (jid.indexOf('@') < 0) {
                dialog.state = 'incomplete';
                return;
            }

            // check for duplicate account
            for (var i = 0; i < accountModel.count; i++) {
                var account = accountModel.get(i);
                if (account.type == 'xmpp' && account.realm == Utils.jidToDomain(jid)) {
                    dialog.state = 'dupe';
                    return;
                }
            }

            dialog.state = 'testing';
            dialog.xmppUsername = jid;
            dialog.xmppPassword = password;
            console.log("connecting: " + dialog.xmppUsername);
            testClient.connectToServer(dialog.xmppUsername + '/AccountCheck', dialog.xmppPassword);
        }

        onConnected: {
            if (dialog.state == 'testing') {
                accountModel.append({type: 'xmpp', username: dialog.xmppUsername, password: dialog.xmppPassword, realm: Utils.jidToDomain(dialog.xmppUsername)});
                if (dialog.webRealm)
                    accountModel.append({type: 'web', username: dialog.webUsername, password: dialog.webPassword, realm: dialog.webRealm});
                accountModel.submit();

                dialog.close();
            }
        }

        onDisconnected: {
            if (dialog.state == 'testing') {
                dialog.state = 'authError';
            }
        }
    }

    Item {
        anchors.fill: dialog.contents
        anchors.leftMargin: appStyle.margin.normal
        anchors.rightMargin: appStyle.margin.normal

        PanelHelp {
            id: help

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr('Enter the address and password for your account.')
        }

        Item {
            id: accountRow

            anchors.top: help.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            height: accountCombo.height

            Label {
                id: accountLabel

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                font.bold: true
                text: qsTr('Account')
                width: 100
            }

            ComboBox {
                id: accountCombo

                anchors.top: parent.top
                anchors.left: accountLabel.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                model: ListModel {}

                Component.onCompleted: {
                    model.append({iconSource: 'image://icon/wiLink', text: 'Wifirst', type: 'wifirst'});
                    model.append({iconSource: 'image://icon/google', text: 'Google', type: 'google'});
                    model.append({iconSource: 'image://icon/peer', text: qsTr('Other'), type: 'other'});
                    accountCombo.currentIndex = 0;
                }
            }
        }

        Item {
            id: usernameRow

            anchors.top: accountRow.bottom
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
                text: qsTr('Address')
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
            property string dupeText: qsTr("You already have an account for '%1'.").replace('%1', Utils.jidToDomain(usernameInput.text));
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

        var accountType = accountCombo.model.get(accountCombo.currentIndex).type;
        if (accountType == 'wifirst') {
            dialog.webRealm = 'www.wifirst.net';
            dialog.webUsername = usernameInput.text;
            if (dialog.webUsername.indexOf('@') < 0) {
                dialog.webUsername += '@wifirst.net';
                usernameInput.text = dialog.webUsername;
            }
            dialog.webPassword = passwordInput.text;

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
                            testClient.testCredentials(jid, passwordInput.text);
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
            xhr.open('GET', 'https://www.wifirst.net/w/wilink/credentials', true, dialog.webUsername, dialog.webPassword);
            xhr.setRequestHeader('Accept', 'application/xml');
            xhr.send();
        } else if (accountType == 'google') {
            dialog.webRealm = 'www.google.com';
            dialog.webUsername = usernameInput.text;
            dialog.webPassword = passwordInput.text;

            testClient.testCredentials(usernameInput.text, passwordInput.text);
        } else {
            testClient.testCredentials(usernameInput.text, passwordInput.text);
        }
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
            name: 'dupe'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.dupeText }
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
