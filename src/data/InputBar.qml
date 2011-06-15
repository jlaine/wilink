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
    id: bar

    property alias text: edit.text
    signal returnPressed

    function backspacePressed() {
        var oldPos = edit.cursorPosition;
        var oldText = edit.text;
        edit.text = oldText.substr(0, oldPos - 1) + oldText.substr(oldPos);
        edit.cursorPosition = oldPos - 1;
    }

    border.color: '#c3c3c3'
    border.width: 1
    color: 'white'
    width: 100
    height: edit.paintedHeight + 8

    TextEdit {
        id: edit

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 4
        focus: true
        smooth: true
        selectByMouse: true
        textFormat: TextEdit.PlainText

        Keys.onReturnPressed: {
            bar.returnPressed();
            return false;
        }
    }

    MouseArea {
        anchors.fill: edit
        acceptedButtons: Qt.RightButton

        onPressed: {
            var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
            menuLoader.source = 'InputMenu.qml';
            menuLoader.item.target = edit;
            menuLoader.item.show(pos.x, pos.y - menuLoader.item.height);
        }
    }
}

