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

Panel {
    id: panel

    function save() {
        // devices
        if (input.currentIndex >= 0) {
            var device = input.model.get(input.currentIndex).text;
            appSettings.audioInputDeviceName = device;
        }
        if (output.currentIndex >= 0) {
            var device = output.model.get(output.currentIndex).text;
            appSettings.audioOutputDeviceName = device;
        }

        // notifications
        appSettings.incomingMessageSound = incomingMessageSound.checked;
        appSettings.outgoingMessageSound = outgoingMessageSound.checked;
    }

    SoundTester {
        id: tester

        onStateChanged: {
            if (tester.state === SoundTester.RecordingState) {
                devices.state = 'recording';
            } else if (tester.state === SoundTester.PlayingState) {
                devices.state = 'playing';
            } else {
                devices.state = '';
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
            anchors.fill: parent
            spacing: appStyle.spacing.vertical

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                elide: Text.ElideRight
                text: qsTr('Audio playback device')
            }

            ComboBox {
                id: output

                anchors.left: parent.left
                anchors.right: parent.right
                model: ListModel {}

                Component.onCompleted: {
                    var names = tester.outputDeviceNames;
                    for (var i in names) {
                        var device = names[i];
                        model.append({text: device});
                        if (device === appSettings.audioOutputDeviceName) {
                            output.currentIndex = i;
                        }
                    }
                    if (output.currentIndex < 0 && names.length > 0)
                        output.currentIndex = 0;
                }
            }

            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                elide: Text.ElideRight
                text: qsTr('Audio capture device')
            }

            ComboBox {
                id: input

                anchors.left: parent.left
                anchors.right: parent.right
                model: ListModel {}

                Component.onCompleted: {
                    var names = tester.inputDeviceNames;
                    for (var i in names) {
                        var device = names[i];
                        model.append({text: device});
                        if (device === appSettings.audioInputDeviceName) {
                            input.currentIndex = i;
                        }
                    }
                    if (input.currentIndex < 0 && names.length > 0)
                        input.currentIndex = 0;
                }
            }

            ProgressBar {
                id: progressBar

                anchors.left: parent.left
                anchors.right: parent.right
                opacity: 0
                maximumValue: Qt.isQtObject(tester) ? tester.maximumVolume : 100
                value: Qt.isQtObject(tester) ? tester.volume : 0
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 24

                Label {
                    id: label

                    property string playingText: qsTr('You should now hear the sound you recorded.')
                    property string recordingText: qsTr('Speak into the microphone for %1 seconds and check the sound level.').replace('%1', tester.duration)

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

                    onClicked: {
                        var inputDevice = input.model.get(input.currentIndex).text;
                        var outputDevice = output.model.get(output.currentIndex).text;
                        tester.start(inputDevice, outputDevice);
                    }
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
            PropertyAnimation { target: progressBar; properties: 'height'; duration: appStyle.animation.normalDuration }
        }
    }

    GroupBox {
        id: notifications

        anchors.top: devices.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Sound notifications')

        Column {
            anchors.fill: parent
            spacing: appStyle.spacing.vertical

            CheckBox {
                id: incomingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: appSettings.incomingMessageSound
                text: qsTr('Incoming message')
                onClicked: checked = !checked
            }

            CheckBox {
                id: outgoingMessageSound

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: appSettings.outgoingMessageSound
                text: qsTr('Outgoing message')
                onClicked: checked = !checked
            }
        }
    }
}

