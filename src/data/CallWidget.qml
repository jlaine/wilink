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

Item {
    height: 300
    width: 550

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
    }

    Rectangle {
        id: videoMonitor

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 10

        border.width: 1
        border.color: '#2689d6'
        color: '#000000'

        radius: 8
        height: 240
        width: 320
    }

    Rectangle {
        id: videoOutput

        anchors.top: videoMonitor.bottom
        anchors.left: videoMonitor.right
        anchors.leftMargin: -100
        anchors.topMargin: -80

        border.width: 1
        border.color: '#2689d6'
        color: '#000000'

        radius: 8
        height: 120
        width: 160
    }

    Button {
        id: closeButton
        iconSource: 'close.png'
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
    }

    Button {
        id: cameraButton
        iconSource: 'camera.png'
        anchors.right: closeButton.left
        anchors.top: parent.top
        anchors.margins: 10
    }

    ProgressBar {
        id: inputVolume
        anchors.top: parent.top
        anchors.right: cameraButton.left
        anchors.bottomMargin: 5
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 10
    }

    ProgressBar {
        id: outputVolume
        anchors.top: inputVolume.bottom
        anchors.right: cameraButton.left
        anchors.bottomMargin: 10
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.topMargin: 5
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
