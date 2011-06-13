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
    id: block

    property alias delegate: view.delegate
    property alias model: view.model
    property alias currentIndex: view.currentIndex

    border.width: 1
    border.color: '#ffffff'
    color: '#567dbc'
    height: 25
    radius: 3
    smooth: true

    Text {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 5
        color: 'white'
        opacity: block.state == 'expanded' ? 0 : 1
        text: '<html>&#9167;</html>'
        z: 1
    }

    ListView {
        id: view

        anchors.fill: parent
        interactive: false
        clip: true
    }

    states: State {
        name: 'expanded'
        PropertyChanges {
            target: block
            height: 25 * view.count
        }
    }
}
