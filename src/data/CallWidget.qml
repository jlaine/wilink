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

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    height: video.enabled ? 288 : 40

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
        anchors.fill: parent
        border.color: '#2689d6'
        gradient: Gradient {
            GradientStop { position: 0.0; color: '#e7f4fe' }
            GradientStop { position: 0.2; color: '#bfddf4' }
            GradientStop { position: 0.8; color: '#bfddf4' }
            GradientStop { position: 1.0; color: '#e7f4fe' }
        }
        radius: 8
        smooth: true

        Text {
            id: status

            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.top: parent.top
            anchors.topMargin: 12
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
            visible: video.enabled
        }

        CallVideoItem {
            id: videoMonitor

            anchors.top: videoOutput.bottom
            anchors.left: videoOutput.right
            anchors.leftMargin: -100
            anchors.topMargin: -80
            border.width: 1
            border.color: '#ff0000'
            radius: 8
            height: 120
            width: 160
            visible: video.enabled
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
            anchors.top: inputVolume.top
            anchors.right: inputVolume.left
            anchors.rightMargin: 5
            source: 'audio-input.png'
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
        }

        Image {
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
        onClicked: call.startVideo()
    }
}
