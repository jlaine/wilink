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

    PhoneCallsModel {
        id: historyModel

        onCurrentCallsChanged: {
            if (historyModel.currentCalls) {
                callButton.visible = false;
                hangupButton.visible = true;
            } else {
                hangupButton.visible = false;
                callButton.visible = true;
            }
        }

        onError: {
            dialogSwapper.showPanel('ErrorNotification.qml', {
                'iconSource': 'phone.png',
                'title': qsTr('Call failed'),
                'text': qsTr('Sorry, but the call could not be completed.') + '\n\n' + error,
            });
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 1.0; color: '#e7effd' }
            GradientStop { position: 0.0; color: '#cbdaf1' }
        }
    }

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'phone.png'
        title: qsTr('Phone')
        subTitle: historyModel.phoneNumber ? qsTr('Your number is %1').replace('%1', historyModel.phoneNumber) : ''
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
        anchors.leftMargin: appStyle.spacing.horizontal
        anchors.right: parent.right
        anchors.rightMargin: appStyle.spacing.horizontal
        anchors.top: help.bottom
        anchors.topMargin: appStyle.spacing.vertical
        height: 32

        InputBar {
            id: numberEdit

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: buttonBox.left
            anchors.rightMargin: appStyle.spacing.horizontal
            hintText: qsTr('Enter the number you want to call')

            Keys.onReturnPressed: {
                if (callButton.enabled) {
                    callButton.clicked();
                }
            }
        }

        Row {
            id: buttonBox

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            spacing: appStyle.spacing.horizontal

            Button {
                id: backButton

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                iconSource: 'back.png'

                onClicked: numberEdit.backspacePressed()
            }

            Button {
                id: callButton

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                enabled: historyModel.client.state == SipClient.ConnectedState
                iconSource: 'call.png'
                text: qsTr('Call')

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
                anchors.bottom: parent.bottom
                iconSource: 'hangup.png'
                text: qsTr('Hangup')
                visible: false

                onClicked: historyModel.hangup()
            }
        }
    }
    
    Row {
        id: controls

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: numberRow.bottom
        anchors.topMargin: appStyle.spacing.vertical
        spacing: appStyle.spacing.horizontal

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

        Item {
            anchors.top: keypad.top
            anchors.bottom: keypad.bottom
            width: 32

            ProgressBar {
                id: inputVolume

                anchors.top: parent.top
                anchors.bottom: inputIcon.top
                anchors.bottomMargin: appStyle.spacing.vertical
                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.Vertical
                maximumValue: Qt.isQtObject(historyModel) ? historyModel.maximumVolume : 100
                value: Qt.isQtObject(historyModel) ? historyModel.inputVolume : 0
            }

            Image {
                id: inputIcon

                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                source: 'audio-input.png'
                height: 32
                width: 32
            }
        }

        Item {
            anchors.top: keypad.top
            anchors.bottom: keypad.bottom
            width: 32

            ProgressBar {
                id: outputVolume

                anchors.top: parent.top
                anchors.bottom: outputIcon.top
                anchors.bottomMargin: appStyle.spacing.vertical
                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.Vertical
                maximumValue: Qt.isQtObject(historyModel) ? historyModel.maximumVolume : 100
                value: Qt.isQtObject(historyModel) ? historyModel.outputVolume : 0
            }

            Image {
                id: outputIcon

                anchors.bottom: parent.bottom
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
        anchors.topMargin: appStyle.spacing.vertical
        anchors.bottom: parent.bottom
        model: historyModel

        onAddressClicked: {
            numberEdit.text = parseAddress(address, historyModel.client.domain);
        }
    }

    Connections {
        target: historyModel.client

        onCallReceived: {
            if (historyModel.currentCalls) {
                // if already busy, refuse call
                call.hangup();
                return;
            }

            historyModel.addCall(call);
            dialogSwapper.showPanel('PhoneNotification.qml', {
                'call': call,
                'caller': parseAddress(call.recipient),
                'swapper': swapper,
            });
        }
    }
}
