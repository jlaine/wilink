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

Panel {
    id: panel

    property QtObject historyModel: PhoneCallsModel {}

    // Builds a full SIP address from a short recipient
    function buildAddress(recipient, sipDomain)
    {
        var address;
        var name;
        var bits = recipient.split('@');
        if (bits.length > 1) {
            name = bits[0];
            address = recipient;
        } else {
            name = recipient;
            address = recipient + "@" + sipDomain;
        }
        return '"' + name + '" <sip:' + address + '>';
    }

    // Extracts the shortest possible recipient from a full SIP address.
    function parseAddress(sipAddress, sipDomain)
    {
        var cap = sipAddress.match(/(.*)<(sip:([^>]+))>(;.+)?/);
        if (!cap)
            return '';

        var recipient = cap[2].substr(4);
        var bits = recipient.split('@');
        if (bits[1] == sipDomain || bits[1].match(/^[0-9]+/))
            return bits[0];
        else
            return recipient;
    }

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        icon: 'phone.png'
        title: '<b>' + qsTr('Phone') + '</b>' + (historyModel.phoneNumber ? qsTr('Your number is %1').replace('%1', historyModel.phoneNumber) : '')

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('You can subscribe to the phone service at the following address:') + ' <a href="' + historyModel.selfcareUrl + '">' + historyModel.selfcareUrl + '</a>';
        visible: !historyModel.enabled
    }

    Item {
        id: numberRow

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: help.bottom
        anchors.margins: 4
        height: 32

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: backButton.left
            anchors.rightMargin: 4
            border.color: '#c3c3c3'
            border.width: 1
            color: '#ffffff'
            height: numberEdit.paintedHeight + 16
            width: 100

            TextEdit {
                id: numberEdit
                focus: true
                x: 8
                y: 8
                smooth: true
                text: ''
                textFormat: TextEdit.PlainText
                width: parent.width - 16

                Keys.onReturnPressed: {
                    if (callButton.enabled)
                        callButton.clicked()
                }
            }

            Text {
                anchors.fill: numberEdit
                color: '#999'
                text: qsTr('Enter the number you want to call')
                visible: numberEdit.text == ''
            }
        }

        Button {
            id: backButton

            anchors.top: parent.top
            anchors.right: callButton.left
            anchors.rightMargin: 4
            icon: 'back.png'

            onClicked: {
                var oldPos = numberEdit.cursorPosition;
                var oldText = numberEdit.text;
                numberEdit.text = oldText.substr(0, oldPos - 1) + oldText.substr(oldPos);
                numberEdit.cursorPosition = oldPos - 1;
            }
        }

        Button {
            id: callButton

            anchors.top: parent.top
            anchors.right: parent.right
            enabled: historyModel.client.state == SipClient.ConnectedState
            icon: 'call.png'
            text: qsTr('Call')
            visible: historyModel.currentCalls == 0

            onClicked: {
                var recipient = numberEdit.text.replace(/\s+/, '');
                if (!recipient.length)
                    return;

                var address = buildAddress(recipient, historyModel.client.domain);
                if (historyModel.call(address))
                    numberEdit.text = '';
            }
        }

        Button {
            id: hangupButton

            anchors.top: parent.top
            anchors.right: parent.right
            icon: 'hangup.png'
            text: qsTr('Hangup')
            visible: historyModel.currentCalls != 0

            onClicked: historyModel.hangup()
        }
    }
    
    Row {
        id: controls

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: numberRow.bottom
        anchors.topMargin: 8
        spacing: 4

        KeyPad {
            id: keypad

            onKeyPressed: {
                if (historyModel.currentCalls)
                    historyModel.startTone(key.tone);
            }

            onKeyReleased: {
                if (historyModel.currentCalls)
                    historyModel.stopTone(key.tone);
                else {
                    var oldPos = numberEdit.cursorPosition;
                    var oldText = numberEdit.text;
                    numberEdit.text = oldText.substr(0, oldPos) + key.name + oldText.substr(oldPos);
                    numberEdit.cursorPosition = oldPos + 1;
                }
            }
        }

        Column {
            spacing: 8

            ProgressBar {
                id: inputVolume

                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.VerticalOrientation
                maximumValue: historyModel.maximumVolume
                value: historyModel.inputVolume
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: 'audio-input.png'
                height: 32
                width: 32
            }
        }

        Column {
            spacing: 8

            ProgressBar {
                id: outputVolume

                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.VerticalOrientation
                maximumValue: historyModel.maximumVolume
                value: historyModel.outputVolume
            }

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: 'audio-output.png'
                height: 32
                width: 32
            }
        }
    }

    PhoneHistoryView {
        id: historyView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: controls.bottom
        anchors.bottom: parent.bottom
        anchors.margins: 10
        clip: true
        model: historyModel

        onAddressClicked: {
            numberEdit.text = parseAddress(address, historyModel.client.domain);
        }
    }

    Connections {
        target: historyModel.client

        onCallReceived: {
            var contactName = call.recipient;

            var box = window.messageBox();
            box.icon = QMessageBox.Question;
            box.standardButton = QMessageBox.Yes | QMessageBox.No;
            box.text = qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', contactName);
            box.windowTitle = qsTr('Call from %1').replace('%1', contactName);
            box.exec();
        }
    }

    Connections {
        target: historyModel

        onError: {
            var box = window.messageBox();
            box.icon = QMessageBox.Warning;
            box.text = qsTr('Sorry, but the call could not be completed.') + '\n\n' + error;
            box.windowTitle = qsTr("Call failed");
            box.show();
        }
    }
}
