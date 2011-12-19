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
import wiLink 2.0

FocusScope {
    id: display

    property alias currentIndex: displayView.currentIndex
    property alias model: displayView.model
    signal currentIndexChanged

    function positionViewAtIndex(index, pos) {
        displayView.positionViewAtIndex(index, pos);
    }

    Rectangle {
        id: background

        anchors.fill: parent
    }

    ListView {
        id: displayView

        anchors.fill: parent
        focus: true
        highlightMoveDuration: appStyle.highlightMoveDuration
        highlightRangeMode: ListView.StrictlyEnforceRange
        orientation: Qt.Horizontal
        snapMode: ListView.SnapToItem

        delegate: Item {
            width: displayView.width
            height: displayView.height

            Image {
                id: preview

                width: displayView.width
                height: displayView.height
                source: model.avatar
                fillMode: Image.PreserveAspectFit
            }

            Image {
                id: image

                asynchronous: true
                width: displayView.width
                height: displayView.height
                source: model.image
                fillMode: Image.PreserveAspectFit
                opacity: 0
            }

            states: State {
                name: 'ready'
                when: model.imageReady
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

        onCurrentIndexChanged: display.currentIndexChanged()

        Keys.onPressed: {
            if (event.key == Qt.Key_Back ||
                event.key == Qt.Key_Backspace) {
                if (crumbBar.model.count > 1) {
                    crumbBar.pop();
                }
            }
            else if (event.key == Qt.Key_Enter ||
                     event.key == Qt.Key_Return ||
                     event.key == Qt.Key_Space) {
                displayView.incrementCurrentIndex();
            }
        }
    }
}
