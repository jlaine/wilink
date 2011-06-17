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
import wiLink 1.2

Panel {
    id: panel

    property bool recording: false
    property int testSeconds: 5

    function save() {
        application.incomingMessageSound = incomingMessageSound.checked ? ':/message-incoming.ogg' : '';
        application.outgoingMessageSound = outgoingMessageSound.checked ? ':/message-outgoing.ogg' : '';
    }

    color: 'transparent'

    SoundTester {
        id: tester

        onStateChanged: {
            if (tester.state == SoundTester.RecordingState) {
                devices.state = 'recording';
            } else if (tester.state == SoundTester.PlayingState) {
                devices.state = 'playing';
            } else {
                devices.state = '';
            }
        }
    }

    ListModel {
        id: inputDevices

        Component.onCompleted: {
            var names = tester.inputDeviceNames;
            for (var i in names) {
                append({'text': names[i]});
            }
        }
    }

    ListModel {
        id: outputDevices

        Component.onCompleted: {
            var names = tester.outputDeviceNames;
            for (var i in names) {
                append({'text': names[i]});
            }
        }
    }

    GroupBox {
        id: devices

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.floor(parent.height * 0.6)
        title: qsTr('Sound devices')

        Column {
            anchors.fill: devices.contents
            spacing: appStyle.spacing.vertical

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: appStyle.spacing.horizontal

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: 'audio-output.png'
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr('Audio playback device')
                }
            }

            ComboBox {
                id: output

                anchors.left: parent.left
                anchors.right: parent.right
                model: outputDevices
                delegate: Text {
                    width: devices.width - 1
                    height: 24
                    elide: Text.ElideRight
                    text: model.text
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: appStyle.spacing.horizontal

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: 'audio-input.png'
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr('Audio capture device')
                }
            }

            ComboBox {
                id: input

                anchors.left: parent.left
                anchors.right: parent.right
                model: inputDevices
                delegate: Text {
                    width: devices.width - 1
                    height: 24
                    elide: Text.ElideRight
                    text: model.text
                    verticalAlignment: Text.AlignVCenter
                }
            }

            ProgressBar {
                id: progressBar

                anchors.left: parent.left
                anchors.right: parent.right
                opacity: 0
                height: 0
                maximumValue: tester.maximumVolume
                value: tester.volume
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 24

                Text {
                    id: label

                    property string playingText: qsTr('You should now hear the sound you recorded.')
                    property string recordingText: qsTr('Speak into the microphone for %1 seconds and check the sound level.').replace('%1', testSeconds)

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: button.left
                    wrapMode: Text.WordWrap
                }

                Button {
                    id: button

                    anchors.top: parent.top
                    anchors.right: parent.right
                    text: qsTr('Test')

                    onClicked: tester.start(input.text, output.text);
                }
            }
        }

        states: [
            State {
                name: 'playing'
                PropertyChanges { target: progressBar; height: 16; opacity: 1 }
                PropertyChanges { target: button; enabled: false }
                PropertyChanges { target: label; text: label.playingText }
            },
            State {
                name: 'recording'
                PropertyChanges { target: progressBar; height: 16; opacity: 1 }
                PropertyChanges { target: button; enabled: false }
                PropertyChanges { target: label; text: label.recordingText }
            }
        ]

        transitions: Transition {
            PropertyAnimation { target: progressBar; properties: 'height'; duration: 150 }
        }
    }

    GroupBox {
        id: notifications

        anchors.top: devices.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Sound notifications')

        Column {
            anchors.fill: notifications.contents
            spacing: appStyle.spacing.vertical

            CheckBox {
                id: incomingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.incomingMessageSound.length > 0
                text: qsTr('Incoming message')
            }

            CheckBox {
                id: outgoingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.outgoingMessageSound.length > 0
                text: qsTr('Outgoing message')
            }
        }
    }
}

