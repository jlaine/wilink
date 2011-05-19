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

    property alias icon: image.source
    signal clicked

    width: 32; height: 32
    border.color: '#2689d6'
    gradient: Gradient {
        GradientStop { id: stop1; position: 0.2; color: '#e7f4fe' }
        GradientStop { id: stop2; position: 0.3; color: '#bfddf4' }
        GradientStop { id: stop3; position: 0.7; color: '#bfddf4' }
        GradientStop { id: stop4; position: 1.0; color: '#88bfe9' }
    }
    radius: 4
    smooth: true

    Image {
        id: image
        width: button.height - 8; height: button.height - 8
        anchors.centerIn: parent
        smooth: true
    }

    MouseArea {
        anchors.fill: parent

        onPressed: {
            button.state = 'pressed'
        }

        onReleased: {
            button.state = ''
            button.clicked()
        }
    }

    states: State {
        name: 'pressed'
        PropertyChanges { target: button; border.color: '#5488bb' }
        PropertyChanges { target: stop1; color: '#88bfe9' }
        PropertyChanges { target: stop4; color: '#e7f4fe' }
    }
}

