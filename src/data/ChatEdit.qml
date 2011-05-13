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
import wiLink 1.2

Rectangle {
    id: chatEdit

    property int chatState: qXmppMessage.none
    property alias text: input.text
    property QtObject model
    signal returnPressed
    signal tabPressed

    border.color: '#c3c3c3'
    border.width: 1
    color: '#ffffff'
    height: input.paintedHeight + 16

    function talkAt(participant) {
        var text = input.text;
        var newText = '';

        var pattern = /@([^,:]+[,:] )+/;
        var start = text.search(pattern);
        if (start >= 0) {
            var m = text.match(pattern)[0];
            var bits = m.split(/[,:] /);
            if (start > 0)
                newText += text.slice(0, start);
            for (var i in bits) {
                if (bits[i] == '@' + participant)
                    return;
                if (bits[i].length)
                    newText += bits[i] + ', ';
            }
        } else {
            newText = text;
        }
        newText += '@' + participant + ': ';
        input.text = newText;
    }

    ListHelper {
        id: listHelper
        model: chatEdit.model
    }

    // FIXME: this is a hack to expose QXmpp constant
    QtObject {
        id: qXmppMessage

        property int none: 0
        property int active: 1
        property int inactive: 2
        property int gone: 3
        property int composing: 4
        property int paused: 5
    }

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

            // search matching participants
            var needle = input.text.slice(start, end).toLowerCase();
            var matches = [];
            for (var i = 0; i < listHelper.count; i += 1) {
                var name = listHelper.get(i).name;
                if (name.slice(0, needle.length).toLowerCase() == needle) {
                    matches[matches.length] = name;
                }
            }
            if (matches.length == 1) {
                var replacement = matches[0] + ': ';
                input.text = text.slice(0, start) + replacement + text.slice(end);
                input.cursorPosition = start + replacement.length;
            }
        }
    }

}

