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

    signal returnPressed

    border.color: '#c3c3c3'
    border.width: 1
    color: '#ffffff'
    height: input.paintedHeight + 16

    TextEdit {
        id: input

        x: 8
        y: 8
        smooth: true
        width: parent.width - 16

        Keys.onReturnPressed: {
            if (event.modifiers == Qt.NoModifier) {
                chatEdit.returnPressed();
            } else {
                event.accepted = false;
            }
        }
    }
}

