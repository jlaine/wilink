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

Rectangle {
    id: button

    property bool enabled: true
    property alias icon: image.source
    property alias text: label.text
    signal clicked

    height: 32
    width: label.width + image.width + 2 * image.anchors.margins
    border.color: '#84bde8'
    gradient: Gradient {
        GradientStop { id: stop1; position: 0.0; color: '#ffffff' }
        GradientStop { id: stop2; position: 1.0; color: '#beceeb' }
    }
    radius: 4
    smooth: true

    Rectangle {
        id: overlay
        anchors.fill: button
        color: '#99ffffff'
        opacity: !button.enabled
        radius: 4
        smooth: true
        z: 10
    }

    Image {
        id: image
        height: button.height - 8
        width: button.height - 8
        anchors.left: parent.left
        anchors.margins: 4
        anchors.verticalCenter: parent.verticalCenter
        smooth: true
    }

    Text {
        id: label
        anchors.left: image.right
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: Math.round(button.height / 2)
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            if (button.enabled) {
                button.clicked();
            }
        }
        onPressed: {
            if (button.enabled) {
                button.state = 'pressed';
            }
        }
        onReleased: {
            if (button.enabled) {
                button.state = '';
            }
        }
    }

    states: State {
        name: 'pressed'
        PropertyChanges { target: stop1; color: '#aacbd9f0' }
        PropertyChanges { target: stop2; color: '#aaffffff' }
    }
}

