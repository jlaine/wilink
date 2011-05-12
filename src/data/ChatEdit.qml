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
    id: chatEdit

    property alias text: input.text
    signal returnPressed
    signal tabPressed

    border.color: '#c3c3c3'
    border.width: 1
    color: '#ffffff'
    height: input.paintedHeight + 16

    TextEdit {
        id: input

        x: 8
        y: 8
        smooth: true
        textFormat: TextEdit.PlainText
        width: parent.width - 16

        Keys.onReturnPressed: {
            if (event.modifiers == Qt.NoModifier) {
                chatEdit.returnPressed();
            } else {
                event.accepted = false;
            }
        }
        Keys.onTabPressed: {
            // select word, including 'at' sign
            var text = input.text;
            var end = input.cursorPosition;
            var start = end;
            while (start >= 0 && text.charAt(start) != '@') {
                start -= 1;
            }
            if (start < 0)
                return;
            start += 1;

            var needle = input.text.slice(start, end).toLowerCase();
            // FIXME: get real members
            var members = ['Foo', 'Bar'];
            for (var i in members) {
                if (members[i].slice(0, needle.length).toLowerCase() == needle) {
                    var replacement = members[i] + ': ';
                    input.text = text.slice(0, start) + replacement + text.slice(end);
                    input.cursorPosition = start + replacement.length;
                }
            }
        }
    }
}

