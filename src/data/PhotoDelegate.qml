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

import QtQuick 1.1

Item {
    id: delegate

    property bool imageNeeded: width > 128

    Image {
        id: preview

        anchors.fill: parent
        source: model.avatar
        fillMode: Image.PreserveAspectFit
    }

    Image {
        id: image

        anchors.fill: parent
        asynchronous: true
        source: imageNeeded ? model.image : ''
        fillMode: Image.PreserveAspectFit
        opacity: 0
    }

    states: State {
        name: 'ready'
        when: imageNeeded && model.imageReady
        PropertyChanges { target: image; opacity: 1 }
    }

    transitions: Transition {
        NumberAnimation {
            target: image
            properties: 'opacity'
            duration: appStyle.animation.normalDuration
        }
    }
}

