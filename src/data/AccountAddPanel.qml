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
import QXmpp 0.4
import wiLink 1.2

Item {
    id: panel

    property bool acceptableInput: (jidInput.acceptableInput && passwordInput.acceptableInput)
    property variant model

    signal accepted
    signal rejected

    PanelHelp {
        id: help

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('Enter the address and password for the account you want to add.')
    }

    InputBar {
        id: jidInput

        anchors.top: help.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        hintText: qsTr('Address')
        validator: RegExpValidator {
            regExp: /^[^@/]+@[^@/]+$/
        }
    }

    InputBar {
        id: passwordInput

        anchors.top: jidInput.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        echoMode: TextInput.Password
        hintText: qsTr('Password')
        validator: RegExpValidator {
            regExp: /.+/
        }
    }

    Text {
        id: statusLabel

        property string failedText: qsTr('Could not connect, please check your username and password.')
        property string testingText: qsTr('Checking your username and password..')

        anchors.top: passwordInput.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        wrapMode: Text.WordWrap
    }

    Row {
        anchors.top: statusLabel.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.horizontalCenter: parent.horizontalCenter
        height: 32
        spacing: appStyle.spacing.horizontal

        Button {
            iconSource: 'back.png'
            text: qsTr('Go back')

            onClicked: panel.rejected()
        }

        Button {
            id: addButton

            iconSource: 'add.png'
            enabled: panel.acceptableInput && panel.state != 'testing'
            text: qsTr('Add')

            onClicked: {
                panel.state = 'testing';
                testClient.connectToServer(jidInput.text + '/AccountCheck', passwordInput.text);
            }
        }
    }

    Client {
        id: testClient

        onConnected: {
            panel.state = '';
            panel.accepted();
        }

        onDisconnected: {
            if (panel.state == 'testing') {
                panel.state = 'failed';
            }
        }
    }

    states: [
        State {
            name: 'failed'
            PropertyChanges { target: statusLabel; color: 'red'; text: statusLabel.failedText }
        },
        State {
            name: 'testing'
            PropertyChanges { target: statusLabel; text: statusLabel.testingText }
            PropertyChanges { target: jidInput; enabled: false }
            PropertyChanges { target: passwordInput; enabled: false }
        }
    ]
}

