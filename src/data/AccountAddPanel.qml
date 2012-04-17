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
import QXmpp 0.4
import wiLink 2.0
import 'utils.js' as Utils

FocusScope {
    id: panel

    property string domain: ''
    property QtObject model
    property string testJid
    property string testPassword

    signal accepted(string jid, string password)
    signal close

    function forceActiveFocus() {
        jidInput.forceActiveFocus();
    }

    PanelHelp {
        id: help

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        text: panel.domain != '' ? qsTr("Enter the username and password for your '%1' account.").replace('%1', panel.domain) : qsTr('Enter the address and password for the account you want to add.')
    }

    Item {
        id: jidRow

        anchors.top: help.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        height: jidInput.height

        Label {
            id: jidLabel

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            font.bold: true
            text: panel.domain != '' ? qsTr('Username') : qsTr('Address')
            width: 100
        }

        InputBar {
            id: jidInput

            anchors.top: parent.top
            anchors.left: jidLabel.right
            anchors.leftMargin: appStyle.spacing.horizontal
            anchors.right: parent.right
            validator: RegExpValidator {
                regExp: panel.domain != '' ? /^[^@/ ]+$/ : /^[^@/ ]+@[^@/ ]+$/
            }

            onTextChanged: {
                if (panel.state == 'badJid' || panel.state == 'dupe') {
                    panel.state = '';
                }
            }
        }
    }

    Item {
        id: passwordRow

        anchors.top: jidRow.bottom
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

            onTextChanged: {
                if (panel.state == 'badPassword') {
                    panel.state = '';
                }
            }
        }
    }

    Label {
        id: statusLabel

        property string badJidText: panel.domain != '' ? qsTr('Please enter a valid username.') : qsTr('Please enter a valid address.')
        property string badPasswordText: qsTr('Please enter your password.')
        property string dupeText: qsTr("You already have an account for '%1'.").replace('%1', Utils.jidToDomain(jidInput.text));
        property string failedText: qsTr('Could not connect, please check your username and password.')
        property string testingText: qsTr('Checking your username and password..')

        anchors.top: passwordRow.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
    }

    Row {
        anchors.top: statusLabel.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.horizontalCenter: parent.horizontalCenter
        height: appStyle.icon.smallSize + 2 * appStyle.margin.normal
        spacing: appStyle.spacing.horizontal

        Button {
            id: addButton

            iconSource: 'add.png'
            text: qsTr('Add')

            onClicked: {
                if (!jidInput.acceptableInput) {
                    panel.state = 'badJid';
                    return;
                } else if (!passwordInput.acceptableInput) {
                    panel.state = 'badPassword';
                    return;
                }

                var jid = jidInput.text;
                if (panel.domain != '')
                    jid += '@' + panel.domain;

                // check for duplicate account
                for (var i = 0; i < panel.model.count; i++) {
                    if (Utils.jidToDomain(panel.model.getProperty(i, 'jid')) == Utils.jidToDomain(jid)) {
                        panel.state = 'dupe';
                        return;
                    }
                }
                panel.state = 'testing';
                panel.testJid = jid;
                panel.testPassword = passwordInput.text;
                testClient.connectToServer(panel.testJid + '/AccountCheck', panel.testPassword);
            }
        }

        Button {
            iconSource: 'back.png'
            text: qsTr('Cancel')

            onClicked: {
                panel.state = '';
                jidInput.text = '';
                passwordInput.Text = '';
                passwordInput.closeSoftwareInputPanel();
                testClient.disconnectFromServer();
                panel.close();
            }
        }
    }

    Client {
        id: testClient

        onConnected: {
            if (panel.state == 'testing') {
                panel.state = '';
                passwordInput.closeSoftwareInputPanel();
                panel.accepted(panel.testJid, panel.testPassword);
            }
        }

        onDisconnected: {
            if (panel.state == 'testing') {
                panel.state = 'failed';
            }
        }
    }

    states: [
        State {
            name: 'badJid'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.badJidText }
            StateChangeScript { script: jidInput.forceActiveFocus() }
        },
        State {
            name: 'badPassword'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.badPasswordText }
            StateChangeScript { script: passwordInput.forceActiveFocus() }
        },
        State {
            name: 'dupe'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.dupeText }
        },
        State {
            name: 'failed'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.failedText }
        },
        State {
            name: 'testing'
            PropertyChanges { target: addButton; enabled: false }
            PropertyChanges { target: statusLabel; text: statusLabel.testingText }
            PropertyChanges { target: jidInput; enabled: false }
            PropertyChanges { target: passwordInput; enabled: false }
        }
    ]

    Keys.onReturnPressed: {
        if (addButton.enabled) {
            addButton.clicked();
        }
    }

    Keys.onTabPressed: {
        if (jidInput.activeFocus)
            passwordInput.forceActiveFocus();
        else
            jidInput.forceActiveFocus();
    }
}

