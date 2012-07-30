/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

import QtQuick 1.1
import wiLink 2.4

GroupBox {
    id: block

    property alias count: view.count
    property alias currentIndex: view.currentIndex
    property alias delegate: view.delegate
    property alias model: view.model
    property alias moving: view.moving

    signal addClicked

    Button {
        parent: block
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 4
        iconSize: appStyle.icon.tinySize
        iconStyle: 'icon-plus'

        onClicked: block.addClicked()
    }

    ScrollView {
        id: view

        anchors.fill: parent
        clip: true
        focus: true
    }
}
