/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import wiLink 2.5
import 'scripts/sip.js' as Sip

Panel {
    id: panel

    property bool phoneEnabled: false
    property string phoneNumber
    property string selfcareUrl
    property string voicemailNumber
    property string webUsername
    property string webPassword

    // Retrieves phone settings.
    function fetchSettings()
    {
        console.log("Phone fetching settings");

        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == 4) {
                if (xhr.status == 200) {
                    var doc = xhr.responseXML.documentElement;
                    for (var i = 0; i < doc.childNodes.length; ++i) {
                        var node = doc.childNodes[i];

                        // skip empty elements
                        if (!node.firstChild)
                            continue;

                        if (node.nodeName == 'calls-url') {
                            historyView.model.url = node.firstChild.nodeValue;
                            historyView.model.username = panel.webUsername;
                            historyView.model.password = panel.webPassword;
                            historyView.model.reload();
                        } else if (node.nodeName == 'contacts-url') {
                            contacts.model.url = node.firstChild.nodeValue;
                            contacts.model.username = panel.webUsername;
                            contacts.model.password = panel.webPassword;
                            contacts.model.reload();
                        } else if (node.nodeName == 'domain') {
                            sipClient.domain = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'enabled') {
                            panel.phoneEnabled = (node.firstChild.nodeValue == 'true');
                        } else if (node.nodeName == 'number') {
                            sipClient.displayName = node.firstChild.nodeValue;
                            panel.phoneNumber = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'password') {
                            sipClient.password = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'selfcare-url') {
                            panel.selfcareUrl = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'username') {
                            sipClient.username = node.firstChild.nodeValue;
                        } else if (node.nodeName == 'voicemail-number') {
                            panel.voicemailNumber = node.firstChild.nodeValue;
                        }
                    }
                    if (panel.phoneEnabled) {
                        sipClient.connectToServer();
                    } else {
                        sipClient.disconnectFromServer();
                    }
                } else {
                    console.log("Phone failed to retrieve settings");
                    if (xhr.status != 401) {
                        settingsTimer.start();
                    }
                }
            }
        }
        xhr.open('GET', 'https://www.wifirst.net/wilink/voip', true, panel.webUsername, panel.webPassword);
        xhr.setRequestHeader('Accept', 'application/xml');
        xhr.send();
    }

    Timer {
        id: settingsTimer

        interval: 60000
        repeat: false
        onTriggered: fetchSettings()
    }

    SipClient {
        id: sipClient

        logger: appLogger

        onCallReceived: {
            if (sipClient.activeCalls > 1) {
                console.log("Too many active calls, refusing call from " + call.recipient);

                // if already busy, refuse call
                call.hangup();
                call.destroyLater();
                return;
            }

            dialogSwapper.showPanel('PhoneNotification.qml', {
                call: call,
                caller: historyView.contactName(call.recipient),
                swapper: swapper,
            });
        }

        onCallStarted: {
            // NOTE: for some reason, if we don't store the call in a local
            // variable, finishCreation seems to receive an incomplete call
            // object.
            var currentCall = call;

            // log call
            var callId = 0;
            historyView.model.addItem({address: currentCall.recipient, duration: 0, flags: currentCall.direction}, function(id) {
                callId = id;
            });

            call.finished.connect(function() {
                if (callId) {
                    var flags = currentCall.direction;
                    if (currentCall.errorString)
                        flags += 2;
                    historyView.model.updateItem(callId, {address: currentCall.recipient, duration: currentCall.duration, flags: flags});
                }
            });

            // create widget
            var component = Qt.createComponent('PhoneCallWidget.qml');
            function finishCreation() {
                if (component.status != Component.Ready)
                    return;

                var widget = component.createObject(widgetBar, {
                    call: currentCall,
                    caller: historyView.contactName(currentCall.recipient),
                });
            }

            if (component.status == Component.Loading)
                component.statusChanged.connect(finishCreation);
            else
                finishCreation();
        }
    }

    Rectangle {
        id: sidebar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: '#474747'
        visible: width > 0
        width: panel.singlePanel ? parent.width : appStyle.sidebarWidth
        z: 1

        PhoneContactView {
            id: contacts

            anchors.fill: parent

            onItemClicked: {
                if (callButton.enabled) {
                    var address = Sip.buildAddress(model.phone, sipClient.domain);
                    sipClient.call(address);
                    if (panel.singlePanel)
                        panel.state = 'no-sidebar';
                }
            }
        }
    }

    Item {
        id: rightColumn

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: panel.singlePanel ? parent.left : sidebar.right
        anchors.right: parent.right
        visible: width > 0

        PanelHeader {
            id: header

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            iconStyle: 'icon-phone'
            title: qsTr('Phone')
            subTitle: panel.phoneNumber ? qsTr('Your number is %1').replace('%1', panel.phoneNumber) : ''
            toolBar: ToolBar {
                ToolButton {
                    enabled: callButton.enabled
                    iconStyle: 'icon-phone'
                    text: qsTr('Voicemail')
                    visible: panel.voicemailNumber != ''

                    onClicked: {
                        var address = Sip.buildAddress(panel.voicemailNumber, sipClient.domain);
                        sipClient.call(address);
                        if (panel.singlePanel)
                            panel.state = 'no-sidebar';
                    }
                }
            }
        }

        PanelHelp {
            id: help

            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            text: (panel.phoneEnabled ? qsTr('You can view your phone bills and your call history at the following address:') : qsTr('You can subscribe to the phone service at the following address:')) + ' <a href="' + panel.selfcareUrl + '">' + panel.selfcareUrl + '</a>';
            visible: panel.selfcareUrl != ''
        }

        Item {
            id: numberRow

            anchors.left: parent.left
            anchors.leftMargin: appStyle.margin.normal
            anchors.right: parent.right
            anchors.rightMargin: appStyle.margin.normal
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
                focus: true
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
                    iconStyle: 'icon-caret-left'

                    onClicked: numberEdit.backspacePressed()
                }

                Button {
                    id: callButton

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    enabled: sipClient.state == SipClient.ConnectedState && !sipClient.activeCalls
                    style: 'primary'
                    text: qsTr('Call')

                    onClicked: {
                        var recipient = numberEdit.text.replace(/\s+/, '');
                        if (!recipient.length)
                            return;

                        var address = Sip.buildAddress(recipient, sipClient.domain);
                        if (sipClient.call(address))
                            numberEdit.text = '';
                        if (panel.singlePanel)
                            panel.state = 'no-sidebar';
                    }
                }
            }
        }

        Column {
            id: widgetBar
            objectName: 'widgetBar'

            anchors.top: numberRow.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            z: 1
        }

        KeyPad {
            id: keypad

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: widgetBar.bottom
            anchors.topMargin: appStyle.spacing.vertical

            onKeyReleased: {
                if (!sipClient.activeCalls) {
                    var oldPos = numberEdit.cursorPosition;
                    var oldText = numberEdit.text;
                    numberEdit.text = oldText.substr(0, oldPos) + key.name + oldText.substr(oldPos);
                    numberEdit.cursorPosition = oldPos + 1;
                }
            }
        }

        PhoneHistoryView {
            id: historyView

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: keypad.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.bottom: parent.bottom
            contactModel: contacts.model

            onAddressClicked: {
                numberEdit.text = Sip.parseAddress(address, sipClient.domain);
            }

            onAddressDoubleClicked: {
                if (callButton.enabled) {
                    sipClient.call(address);
                }
            }
        }

        states: State {
            name: 'inCall'
            when: sipClient.activeCalls

            PropertyChanges {
                target: numberRow
                height: 0
                visible: false
            }
        }
    }

    Component.onCompleted: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                panel.webUsername = account.username;
                panel.webPassword = account.password;
                fetchSettings();
                break;
            }
        }
    }

    onDockClicked: {
        panel.state = (panel.state == 'no-sidebar') ? '' : 'no-sidebar';
    }

    states: State {
        name: 'no-sidebar'

        PropertyChanges { target: sidebar; width: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: sidebar; properties: 'width'; duration: appStyle.animation.normalDuration }
    }
}
