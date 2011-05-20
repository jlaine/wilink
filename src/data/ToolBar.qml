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
    id: toolbar

    property alias listView: view
    property alias actions: view.model

    signal itemClicked(int index)

    color: 'transparent'
    height: 42

    ListView {
        id: view

        anchors.fill: parent
        anchors.topMargin: 1
        anchors.leftMargin: 1
        clip: true
        interactive: false
        model: toolbar.model
        orientation: ListView.Horizontal
        spacing: 0

        delegate: ToolButton {
            id: listViewItem

            enabled: model.enabled != false
            icon: model.icon
            text: model.text
            visible: model.visible != false
        }
    }
}
