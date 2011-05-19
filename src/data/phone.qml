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

Item {
    anchors.fill: parent
    width: 320
    height: 240

    Item {
        id: header

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        height: 32

        TextEdit {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: backButton.left
            width: 100
        }

        Button {
            id: backButton

            anchors.top: parent.top
            anchors.right: callButton.left
            icon: 'back.png'
        }

        Button {
            id: callButton

            anchors.top: parent.top
            anchors.right: parent.right
            icon: 'call.png'
            text: qsTr('Call')
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
        }

        Column {
            spacing: 8

            ProgressBar {
                id: inputVolume

                anchors.horizontalCenter: parent.horizontalCenter
                orientation: Qt.VerticalOrientation
                maximumValue: 100
                value: 50
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
                maximumValue: Qt.isQtObject(audio) ? audio.maximumVolume : 1
                value: Qt.isQtObject(audio) ? audio.outputVolume : 0
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
    }

    Connections {
        target: historyView
        onAddressClicked: {
            console.log("clicked " + address);
        }
    }
}
