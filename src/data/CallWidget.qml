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
    id: callWidget

    property QtObject call: null
    property int soundId: 0

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    clip: true
    height: video.openMode != CallVideoHelper.NotOpen ? 248 : 40

    CallAudioHelper {
        id: audio

        call: callWidget.call
    }

    CallVideoHelper {
        id: video

        call: callWidget.call
        monitor: videoMonitor
        output: videoOutput
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

        Text {
            id: status

            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.right: inputIcon.left
            elide: Text.ElideRight
            text: {
                if (!call || call.state == QXmppCall.ConnectingState) {
                    return qsTr('Connecting..');
                } else if (call.state == QXmppCall.ActiveState) {
                    return qsTr('Call connected.');
                } else if (call.state == QXmppCall.DisconnectingState) {
                    return qsTr('Disconnecting..');
                } else if (call.state == QXmppCall.FinishedState) {
                    return qsTr('Call finished.');
                }
            }
        }

        CallVideoItem {
            id: videoOutput

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 4
            border.width: 1
            border.color: '#2689d6'
            radius: 8
            height: 240
            width: 320
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
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: inputIcon.left
            height: 40
            border.color: background.border.color
            border.width: background.border.width
            gradient: background.gradient
        }

        Button {
            id: hangupButton

            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4
            iconSource: 'hangup.png'
        }

        Button {
            id: cameraButton

            anchors.right: hangupButton.left
            anchors.top: parent.top
            anchors.margins: 4
            enabled: Qt.isQtObject(call) && call.state == QXmppCall.ActiveState
            iconSource: 'camera.png'
        }

        ProgressBar {
            id: inputVolume

            anchors.top: parent.top
            anchors.right: cameraButton.left
            anchors.margins: 4
            maximumValue: audio.maximumVolume
            value: audio.inputVolume
        }

        ProgressBar {
            id: outputVolume

            anchors.top: inputVolume.bottom
            anchors.right: cameraButton.left
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            maximumValue: audio.maximumVolume
            value: audio.outputVolume
        }

        Image {
            id: inputIcon

            anchors.top: inputVolume.top
            anchors.right: inputVolume.left
            anchors.rightMargin: 5
            source: 'audio-input.png'
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
        }

        Image {
            id: outputIcon

            anchors.top: outputVolume.top
            anchors.right: outputVolume.left
            anchors.rightMargin: 5
            source: 'audio-output.png'
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
        }
    }

    Connections {
        target: hangupButton
        onClicked: call.hangup()
    }

    Connections {
        target: cameraButton
        onClicked: {
            if (video.openMode & CallVideoHelper.WriteOnly)
                call.stopVideo();
            else
                call.startVideo();
        }
    }

    Connections {
        target: call
        onStateChanged: {
            if (callWidget.soundId > 0) {
                application.soundPlayer.stop(callWidget.soundId);
                callWidget.soundId = 0;
            }
        }
    }

    onCallChanged: {
        // play a sound
        if (callWidget.call.direction == QXmppCall.OutgoingDirection &&
            callWidget.call.state == QXmppCall.ConnectingState) {
            callWidget.soundId = application.soundPlayer.play(":/call-outgoing.ogg", true);
        }
    }

    states: State {
          name: 'inactive'
          when: Qt.isQtObject(call) && call.state == QXmppCall.FinishedState
          PropertyChanges { target: callWidget; opacity: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: callWidget; properties: 'opacity'; duration: 500 }
    }
}
