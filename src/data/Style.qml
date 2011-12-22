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

Item {
    property alias animation: animationItem
    property alias font: fontItem
    property int highlightMoveDuration: 500
    property alias icon: iconItem
    property bool isMobile: application.isMobile
    property alias margin: marginItem
    property alias spacing: spacingItem

    opacity: 0

    Item {
        id: animationItem

        property int normalDuration: 200
        property int longDuration: 500
    }

    Item {
        id: fontItem

        property int largeSize: Math.ceil(normalSize * 1.5)
        property int normalSize: isMobile ? 22 : textItem.font.pixelSize
        property int smallSize: Math.ceil(normalSize * 0.9)
    }

    Item {
        id: iconItem

        property int tinySize: isMobile ? 32 : 16
        property int smallSize: isMobile ? 48 : 24
        property int normalSize: isMobile ? 64 : 32
    }

    Item {
        id: marginItem

        property int normal: isMobile ? 8 : 4
        property int large: isMobile ? 16 : 8
    }

    Item {
        id: spacingItem

        property int horizontal: 6
        property int vertical: 4
    }

    Text {
        id: textItem
    }
}

