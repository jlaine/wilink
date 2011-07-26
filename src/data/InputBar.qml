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

FocusScope {
    id: bar

    property alias acceptableInput: edit.acceptableInput
    property alias cursorPosition: edit.cursorPosition
    property alias echoMode: edit.echoMode
    property alias hintText: label.text
    property alias readOnly: edit.readOnly
    property alias text: edit.text
    property alias validator: edit.validator

    focus: true

    function backspacePressed() {
        var oldPos = edit.cursorPosition;
        var oldText = edit.text;
        edit.text = oldText.substr(0, oldPos - 1) + oldText.substr(oldPos);
        edit.cursorPosition = oldPos - 1;
    }

    width: 100
    height: edit.height + 8

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: '#c3c3c3'
        border.width: 1
        color: readOnly ? '#dfdfdf' : 'white'
    }

    TextInput {
        id: edit

        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: (parent.height - height) / 2
        focus: !readOnly
        smooth: true
        selectByMouse: !readOnly
    }

    Text {
        id: label

        anchors.fill: edit
        elide: Text.ElideRight
        color: '#999'
        opacity: edit.text == '' ? 1 : 0
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton

        onPressed: {
            var pos = mapToItem(menuLoader.parent, mouse.x, mouse.y);
            var component = Qt.createComponent('InputMenu.qml');

            function finishCreation() {
                if (component.status != Component.Ready)
                    return;

                menuLoader.sourceComponent = component;
                menuLoader.item.target = edit;
                menuLoader.show(pos.x, pos.y);
            }

            if (component.status == Component.Ready)
                finishCreation();
            else
                component.statusChanged.connect(finishCreation);
        }
    }

    states: State {
        name: 'error'
        PropertyChanges { target: background; color: '#ffaaaa' }
    }
}

