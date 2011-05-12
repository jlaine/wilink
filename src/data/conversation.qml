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
    width: 320
    height: 400

    Column {
        id: widgetBar
        objectName: 'widgetBar'

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1
    }

    Row {
        anchors.top: widgetBar.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
    
        HistoryView {
            id: historyView
            objectName: 'historyView'

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: parent.width - scrollBar.width
            model: historyModel

            function onBottomChanged() {
                if(scrollBar.dragToBottomEnabled)
                    currentIndex = count - 1
                    positionViewAtIndex(count - 1, ListView.End);
            }
        }

        ScrollBar {
            id: scrollBar

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            flickableItem: historyView
            dragToBottomEnabled: true
        }
    }

    Rectangle {
        id: footer

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#dfdfdf'
        height: chatInput.height + 8

        ChatEdit {
            id: chatInput

            x: 4
            y: 4
            width: parent.width - 8
        }
    }
}
