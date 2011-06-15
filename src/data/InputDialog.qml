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

Dialog {
    id: dialog

    property alias helpText: help.text
    property alias labelText: label.text
    property alias textValue: textEdit.text

    minWidth: 280
    minHeight: (help.opacity == 1) ? 250 : 150
    height: (help.opacity == 1) ? 250 : 150
    width: 280

    Item {
        anchors.fill: contents

        PanelHelp {
            id: help

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            opacity: text.length > 0 ? 1 : 0
        }

        Text {
            id: label

            anchors.top: (help.opacity == 1) ? help.bottom : parent.top
            anchors.topMargin: (help.opacity == 1) ? 8 : 0
            anchors.left:  parent.left
            anchors.right: parent.right
            wrapMode: Text.WordWrap

            onLinkActivated: Qt.openUrlExternally(link)
        }

        Rectangle {
            id: bar

            anchors.top: label.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            border.color: '#c3c3c3'
            border.width: 1
            color: 'white'
            width: 100
            height: textEdit.paintedHeight + 8

            TextEdit {
                id: textEdit

                anchors.fill: parent
                anchors.margins: 4
                focus: true
                smooth: true
                textFormat: TextEdit.PlainText

                Keys.onReturnPressed: {
                    dialog.accepted();
                    return false;
                }
            }
        }
    }
}

