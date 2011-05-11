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
    property QtObject job: null

    width: 320
    height: 40

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

        Image {
            id: icon

            width: 32; height: 32
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            source: 'upload.png'
        }

        Text {
            id: text

            anchors.left: icon.right
            anchors.verticalCenter: parent.verticalCenter
            text: job ? job.fileName : ''
        }

        ProgressBar {
            id: progressBar

            anchors.left: text.right
            anchors.leftMargin: 4
            anchors.right: button.left
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter

            Connections {
                target: job
                onProgress: {
                    progressBar.maximumValue = total;
                    progressBar.value = done;
                }
            }
        }

        Button {
            id: button

            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            iconSource: 'close.png'
        }
    }

}
