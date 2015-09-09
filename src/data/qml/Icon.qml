/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import 'scripts/utils.js' as Utils

Item {
    id: icon

    property alias color: label.color
    property alias font: label.font
    property string style

    width: label.width
    height: Math.round(label.height * 0.75)

    Label {
        id: label

        font.family: appStyle.icon.fontFamily
        font.pixelSize: appStyle.isMobile ? 28 : 14
        text: Utils.iconText(icon.style)
    }
}
