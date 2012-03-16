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
import wiLink 2.0

Panel {
    id: panel

    property variant url

    TabView {
        id: tabView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: appStyle.icon.smallSize
        panelSwapper: tabSwapper

        footer: ToolButton {
            id: addButton

            anchors.verticalCenter: parent.verticalCenter
            iconSize: appStyle.icon.tinySize
            iconSource: 'add.png'
            width: iconSize

            onClicked: tabSwapper.addPanel('WebTab.qml', {}, true);
        }
    }

    PanelSwapper {
        id: tabSwapper

        anchors.top: tabView.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        focus: true
    }

    Component.onCompleted: {
        tabSwapper.addPanel('WebTab.qml', {'url': 'https://www.wifirst.net/'}, true)
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_T) {
            tabSwapper.addPanel('WebTab.qml', {}, true);
        }
    }
}
