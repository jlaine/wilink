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
    width: 320
    height: 400

    Component {
        id: playerDelegate
        Item {
            id: item
            height: 40
            width: parent.width

            function formatDuration(ms) {
                var secs = ms / 1000;
                var hours = Number(secs / 3600).toFixed();
                var minutes = Number(secs / 60).toFixed() % 3600;
                var seconds = Number(secs).toFixed() % 60

                function padNumber(n) {
                    var s = n.toString();
                    if (s.length == 1)
                        return '0' + s;
                    return s;
                }

                if (hours > 0)
                    return hours.toString() + ':' + padNumber(minutes) + ':' + padNumber(seconds);
                else
                    return minutes.toString() + ':' + padNumber(seconds);
            }

            MouseArea {
                anchors.fill: parent
                onClicked: playerView.currentIndex = index;
                onDoubleClicked: {
                    var row = playerView.model.row(index);
                    playerView.model.play(row);
                }
            }

            Rectangle {
                Image {
                    id: imageColumn
                    x: 10
                    width: 32
                    height: 32
                    source: model.playing ? "start.png" : (model.downloading ? "download.png" : model.imageUrl);
                }
                Column {
                    id: textColumn
                    anchors.left: imageColumn.right;
                    width: item.width - imageColumn.width - 60
                    Text { text: '<b>' + artist + '</b>' }
                    Text { text: title }
                }
                Text {
                    anchors.left: textColumn.right
                    text: formatDuration(duration)
                }
            }
        }
    }

    ListView {
        id: playerView
        anchors.fill: parent
        model: playerModel
        delegate: playerDelegate
        highlight: Rectangle { color: "lightsteelblue"; radius: 5; width: playerView.width }
        focus: true
    }
}
