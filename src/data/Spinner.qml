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

Image {
    id: spinner

    property bool busy: false

    opacity: spinner.busy ? 1.0 : 0.0
    source: 'image://icon/128x128/spinner'

    Behavior on opacity {
        NumberAnimation {
            duration: 100
        }
    }

    RotationAnimation on rotation {
        from: 0
        to: 360
        duration: 1000
        loops: Animation.Infinite
        running: spinner.busy
    }
}
