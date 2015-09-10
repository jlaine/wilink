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
import QtQuick.Window 2.2
import QtMultimedia 5.4
import wiLink 2.4
import 'scripts/utils.js' as Utils

Item {
    id: callWidget

    property QtObject call: null
    property string caller

    property QtObject audio: CallAudioHelper {
        call: callWidget.call
    }
    property QtObject video: CallVideoHelper {
        call: callWidget.call
        monitor: videoMonitor
        output: videoOutput
    }
    property bool videoEnabled: true

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    clip: true
    height: video.openMode != CallVideoHelper.NotOpen ? (240 + 2 * appStyle.margin.normal) : frame.height
    z: 5

    SoundEffect {
        id: soundLoader

        loops: SoundEffect.Infinite
        source: 'sounds/call-outgoing.wav'
    }

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: '#93b9f2'
        border.width: 1
        gradient: Gradient {
            id: backgroundGradient
            GradientStop { position: 0; color: '#e7effd' }
            GradientStop { position: 1; color: '#cbdaf1' }
        }
        smooth: true

        Image {
            id: image

            anchors.left: parent.left
            anchors.leftMargin: appStyle.margin.normal
            anchors.verticalCenter: parent.verticalCenter
            source: (call && call.direction == QXmppCall.IncomingDirection) ? 'images/call-incoming.png' : 'images/call-outgoing.png'
            width: appStyle.icon.smallSize
            height: appStyle.icon.smallSize
            visible: !videoEnabled
        }

        Label {
            id: status

            anchors.left: image.right
            anchors.leftMargin: appStyle.margin.normal
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
                    if (call.duration)
                        status = Utils.formatDuration(call.duration);
                    else
                        status = qsTr('Call connected.');
                } else if (call.state == QXmppCall.DisconnectingState) {
                    status = qsTr('Disconnecting..');
                } else if (call.state == QXmppCall.FinishedState) {
                    status = qsTr('Call finished.');
                }
                if (caller)
                    return caller + '<br/><small>' + status + '</small>';
                else
                    return status;
            }
        }

        CallVideoItem {
            id: videoOutput

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: appStyle.margin.normal
            border.width: 1
            border.color: '#2689d6'
            radius: 8
            height: parent.height - (2 * appStyle.margin.normal)
            width: height * 4 / 3
            visible: video.openMode != CallVideoHelper.NotOpen
        }

        CallVideoItem {
            id: videoMonitor

            anchors.bottom: videoOutput.bottom
            anchors.left: videoOutput.right
            anchors.leftMargin: appStyle.spacing.horizontal
            border.width: 1
            border.color: '#ff0000'
            radius: 8
            height: 120
            width: 160
            visible: (video.openMode & CallVideoHelper.WriteOnly) != 0
        }

        Rectangle {
            id: frame

            anchors.fill: controls
            anchors.margins: -appStyle.margin.normal
            height: appStyle.icon.smallSize + 4 * appStyle.margin.normal
            border.color: background.border.color
            border.width: background.border.width
            gradient: backgroundGradient
            smooth: true
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

                    source: 'images/audio-input.png'
                    height: appStyle.icon.tinySize
                    width: appStyle.icon.tinySize
                }

                Image {
                    id: outputIcon

                    source: 'images/audio-output.png'
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
                id: cameraButton

                enabled: Qt.isQtObject(call) && call.state == QXmppCall.ActiveState
                iconStyle: 'icon-facetime-video'
                visible: videoEnabled

                onClicked: {
                    if (video.openMode & CallVideoHelper.WriteOnly)
                        call.stopVideo();
                    else
                        call.startVideo();
                }

                states: State {
                    name: 'active'
                    when: (video.openMode & CallVideoHelper.WriteOnly) != 0
                    PropertyChanges { target: cameraButton; iconColor: 'red' }
                }
            }

            Button {
                id: fullScreenButton

                enabled: Qt.isQtObject(call) && call.state == QXmppCall.ActiveState
                iconStyle: 'icon-fullscreen'
                visible: videoEnabled

                onClicked: {
                    if (callWidget.state == '')
                        callWidget.state = 'fullscreen';
                    else if (callWidget.state == 'fullscreen')
                        callWidget.state = '';
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
        onStateChanged: soundLoader.stop()
    }

    onCallChanged: {
        if (!callWidget.call)
            return;

        // play a sound
        if (callWidget.call.direction == QXmppCall.OutgoingDirection &&
            callWidget.call.state == QXmppCall.ConnectingState) {
            soundLoader.play();
        }
    }

    states: [
        State {
            name: 'inactive'
            when: Qt.isQtObject(call) && call.state == QXmppCall.FinishedState
            PropertyChanges { target: callWidget; opacity: 0 }
        },
        State {
            name: 'fullscreen'

            ParentChange {
                target: callWidget
                parent: root
            }

            PropertyChanges {
                target: callWidget
                x: 0
                y: 0
                height: root.height
            }

            PropertyChanges {
                target: window
                visibility: Window.FullScreen
            }
        }
    ]

    transitions: Transition {
        PropertyAnimation {
            target: callWidget
            properties: 'opacity'
            duration: appStyle.animation.longDuration
        }
        PropertyAnimation {
            target: callWidget
            properties: 'x,y,height,width'
            duration: appStyle.animation.normalDuration
        }
        PropertyAnimation {
            target: videoOutput
            properties: 'x,y,height,width'
            duration: appStyle.animation.normalDuration
        }
        PropertyAnimation {
            target: videoMonitor
            properties: 'x,y,height,width'
            duration: appStyle.animation.normalDuration
        }
    }
}
