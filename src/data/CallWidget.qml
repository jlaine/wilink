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
    property QtObject audio: null
    property bool videoEnabled: false

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    height: videoEnabled ? 288 : 40

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
            text: qsTr('Connecting..')
        }

        CallVideoItem {
            id: videoMonitor

            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 4

            //border.width: 1
            //border.color: '#2689d6'

            radius: 8
            height: 240
            width: 320

            visible: videoEnabled
        }

        CallVideoItem {
            id: videoOutput

            anchors.top: videoMonitor.bottom
            anchors.left: videoMonitor.right
            anchors.leftMargin: -100
            anchors.topMargin: -80

            //border.width: 1
            //border.color: '#2689d6'

            radius: 8
            height: 120
            width: 160

            visible: videoEnabled
        }

        Button {
            id: hangupButton

            iconSource: 'hangup.png'
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 4
        }

        Button {
            id: cameraButton

            iconSource: 'camera.png'
            anchors.right: hangupButton.left
            anchors.top: parent.top
            anchors.margins: 4
        }

        ProgressBar {
            id: inputVolume

            anchors.top: parent.top
            anchors.right: cameraButton.left
            anchors.margins: 4
            maximumValue: audio ? audio.maximumVolume : 1
            value: audio ? audio.outputVolume : 0
        }

        ProgressBar {
            id: outputVolume

            anchors.top: inputVolume.bottom
            anchors.right: cameraButton.left
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            maximumValue: audio ? audio.maximumVolume : 1
            value: audio ? audio.inputVolume : 0
        }

        Image {
            anchors.top: inputVolume.top
            anchors.right: inputVolume.left
            anchors.rightMargin: 5
            source: 'audio-input.png'
            height: 16
            width: 16
        }

        Image {
            anchors.top: outputVolume.top
            anchors.right: outputVolume.left
            anchors.rightMargin: 5
            source: 'audio-output.png'
            height: 16
            width: 16
        }
    }

    Connections {
        target: hangupButton
        onClicked: {
            call.hangup();
        }
    }

    Connections {
        target: cameraButton
        onClicked: {
            callWidget.videoEnabled = true;
            call.startVideo();
        }
    }

    Connections {
        target: call
        onStateChanged: {
            if (call.state == QXmppCall.ActiveState) {
                status.text = qsTr('Call connected.');
            } else if (call.state == QXmppCall.DisconnectingState) {
                status.text = qsTr('Disconnecting..');
            } else if (call.state == QXmppCall.FinishedState) {
                status.text = qsTr('Call finished.');
            }
        }
    }

}
