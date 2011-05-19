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
    anchors.fill: parent
    width: 320
    height: 240

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

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
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
                textFormat: TextEdit.PlainText
                width: parent.width - 16
            }
        }

        Button {
            id: backButton

            anchors.top: parent.top
            anchors.right: callButton.left
            anchors.rightMargin: 4
            enabled: sipClient.state == SipClient.ConnectedState
            icon: 'back.png'
        }

        Button {
            id: callButton

            anchors.top: parent.top
            anchors.right: parent.right
            icon: 'call.png'
            text: qsTr('Call')
            visible: historyModel.currentCalls == 0

            onClicked: {
                var recipient = numberEdit.text.replace(/\s+/, '');
                if (!recipient.length)
                    return;

                var address = buildAddress(recipient, sipClient.domain);
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
        anchors.top: header.bottom
        anchors.topMargin: 8
        spacing: 4

        KeyPad {
            id: keypad

            onKeyPressed: {
                console.log("key pressed " + key.name + ": " + key.tone);
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
        anchors.margins: 4
        clip: true
        model: historyModel

        onAddressClicked: {
            numberEdit.text = parseAddress(address, sipClient.domain);
        }
    }
}
