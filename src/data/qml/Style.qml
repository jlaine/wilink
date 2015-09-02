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

Item {
    property alias animation: animationItem
    property alias font: fontItem
    property int highlightMoveDuration: 500
    property alias icon: iconItem
    property bool isMobile: false
    property alias margin: marginItem
    property int sidebarWidth: isMobile ? 400 : 200
    property alias spacing: spacingItem

    opacity: 0

    Item {
        id: animationItem

        property int normalDuration: 200
        property int longDuration: 500
    }

    Item {
        id: fontItem

        property int largeSize: isMobile ? 40 : 20
        property int normalSize: isMobile ? 26 : 13
        property int smallSize: isMobile ? 24 : 12
        property int tinySize: isMobile ? 22 : 11
    }

    Item {
        id: iconItem

        property int tinySize: isMobile ? 32 : 16
        property int smallSize: isMobile ? 48 : 24
        property int normalSize: isMobile ? 64 : 32
        property int largeSize: isMobile ? 128 : 64

        property alias fontFamily: iconFont.name
    }

    Item {
        id: marginItem

        property int small: isMobile ? 4 : 2
        property int normal: isMobile ? 8 : 4
        property int large: isMobile ? 16 : 8
    }

    Item {
        id: spacingItem

        property int horizontal: isMobile ? 12 : 6
        property int vertical: isMobile ? 8 : 4
    }

    FontLoader {
        id: iconFont
        source: "fonts/fontawesome-webfont.ttf"
    }
}

