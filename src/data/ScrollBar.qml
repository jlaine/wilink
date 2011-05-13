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
    id: scrollBar

    property ListView flickableItem
    property bool autoMove: false
    property bool dragToBottomEnabled
    property int minHeight: 20
    property string moveAction: ""

    width: 16

    ScrollBarView {
        id: scrollBarView

        anchors.fill: parent
        position: flickableItem.visibleArea.yPosition
        pageSize: flickableItem.visibleArea.heightRatio

        function moveDown() {
            var targetIndex = flickableItem.currentIndex + 5
            if (targetIndex < flickableItem.count) {
                flickableItem.currentIndex = targetIndex
                flickableItem.positionViewAtIndex(targetIndex, ListView.Visible);
            } else {
                autoMove = false
                dragToBottomEnabled = true
                flickableItem.positionViewAtIndex(flickableItem.count - 1, ListView.End);
            }
        }

        function moveUp() {
            var targetIndex = flickableItem.currentIndex - 5
            if (targetIndex > 0) {
                flickableItem.currentIndex = targetIndex
                flickableItem.positionViewAtIndex(targetIndex, ListView.Visible);
            } else {
                autoMove = false
                flickableItem.positionViewAtIndex(0, ListView.Start);
            }
        }

        function dragHandle(origin, target)
        {
            var density = flickableItem.count / scrollBar.height
            var targetIndex = flickableItem.currentIndex + density * (target - origin)

            if (targetIndex < 0) {
                flickableItem.positionViewAtIndex(0, ListView.Start);
            } else if (targetIndex > flickableItem.count - 1 ) {
                flickableItem.positionViewAtIndex(flickableItem.count - 1, ListView.End);
            } else {
                flickableItem.positionViewAtIndex(targetIndex, ListView.Visible);
            }
        }

        function dropHandle(origin, target)
        {
            var density = flickableItem.count / scrollBar.height
            var targetIndex = flickableItem.currentIndex + density * (target - origin)

            if (targetIndex < 0) {
                flickableItem.currentIndex = 0
            } else if (targetIndex > flickableItem.count - 1 ) {
                flickableItem.currentIndex = flickableItem.count - 1
            } else {
                flickableItem.currentIndex = targetIndex
            }
        }
    }
}
