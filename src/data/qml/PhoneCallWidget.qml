/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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
import wiLink 2.4
import 'scripts/utils.js' as Utils

Item {
    id: callWidget

    property QtObject call: null
    property string caller
    property int callId: 0

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    clip: true
    height: frame.height
    z: 5

    SoundLoader {
        id: soundLoader

        repeat: true
        source: 'sounds/call-outgoing.ogg'
    }

    PhoneAudioHelper {
        id: audio

        call: callWidget.call
    }

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: '#93b9f2'
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: '#e7effd' }
            GradientStop { position: 1; color: '#cbdaf1' }
        }

        Image {
            id: image

            anchors.left: parent.left
            anchors.leftMargin: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
            source: (call && call.direction == SipCall.IncomingDirection) ? 'image://icon/call-incoming' : 'image://icon/call-outgoing'
            sourceSize.width: appStyle.icon.smallSize
            sourceSize.height: appStyle.icon.smallSize
        }

        Label {
            id: status

            anchors.left: image.right
            anchors.leftMargin: appStyle.spacing.horizontal
            anchors.top: parent.top
            anchors.right: controls.left
            elide: Text.ElideRight
            height: frame.height
            verticalAlignment: Text.AlignVCenter
            text: {
                var status;
                if (!call || call.state == QXmppCall.ConnectingState) {
                    status = qsTr('Connecting..');
                } else if (call.state == QXmppCall.ActiveState) {
                    status = Utils.formatDuration(call.duration);
                } else if (call.state == QXmppCall.DisconnectingState) {
                    status = qsTr('Disconnecting..');
                } else if (call.state == QXmppCall.FinishedState) {
                    status = qsTr('Call finished.');
                }
                return caller + '<br/><small>' + status + '</small>';
            }
        }

        Rectangle {
            id: frame

            anchors.fill: controls
            anchors.margins: -appStyle.margin.normal
            height: appStyle.icon.smallSize + 4 * appStyle.margin.normal
            border.color: background.border.color
            border.width: background.border.width
            gradient: background.gradient
        }

        Row {
            id: controls

            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: appStyle.margin.normal
            height: appStyle.icon.smallSize + 2 * appStyle.margin.normal
            spacing: appStyle.margin.normal

            Column {
                Image {
                    id: inputIcon

                    source: 'image://icon/audio-input'
                    height: appStyle.icon.tinySize
                    width: appStyle.icon.tinySize
                }

                Image {
                    id: outputIcon

                    source: 'image://icon/audio-output'
                    height: appStyle.icon.tinySize
                    width: appStyle.icon.tinySize
                }
            }

            Column {
                ProgressBar {
                    id: inputVolume

                    maximumValue: audio.maximumVolume
                    value: audio.inputVolume
                }

                ProgressBar {
                    id: outputVolume

                    maximumValue: audio.maximumVolume
                    value: audio.outputVolume
                }
            }

            Button {
                id: hangupButton

                iconStyle: 'icon-remove'
                onClicked: call.hangup()
            }
        }
    }

    Connections {
        target: call
        onStateChanged: {
            soundLoader.stop();
            if (call.state == QXmppCall.FinishedState) {
                if (callWidget.callId) {
                    var flags = call.direction;
                    if (call.errorString) {
                        flags += 2;
                        dialogSwapper.showPanel('ErrorNotification.qml', {
                            title: qsTr('Call failed'),
                            text: qsTr('Sorry, but the call could not be completed.') + '\n\n' + call.errorString,
                        });
                    }
                    historyView.model.updateItem(callWidget.callId, {address: call.recipient, duration: call.duration, flags: flags});
                }

                // destroy call
                call.destroyLater();
                callWidget.call = null;

                // make widget go away
                callWidget.opacity = 0;
                callWidget.destroy(1000);
            }
        }
    }

    Connections {
        target: keypad

        onKeyPressed: {
            audio.startTone(key.tone);
        }

        onKeyReleased: {
            audio.stopTone(key.tone);
        }
    }

    onCallChanged: {
        if (!callWidget.call)
            return;

        // play a sound
        if (callWidget.call.direction == QXmppCall.OutgoingDirection &&
            callWidget.call.state == QXmppCall.ConnectingState) {
            soundLoader.start();
        }

        // log call
        historyView.model.addItem({address: call.recipient, duration: 0, flags: call.direction}, function(id) {
            callId = id;
        });
    }

    transitions: Transition {
        PropertyAnimation {
            target: callWidget
            properties: 'opacity'
            duration: appStyle.animation.longDuration
        }
    }
}
