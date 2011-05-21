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

Panel {
    id: panel

    Component {
        id: playerDelegate
        Item {
            id: item
            property bool isSelected: playerView.currentItem == item
            height: 42
            width: parent.width

            function formatDuration(ms) {
                var secs = ms / 1000;
                var hours = Number(secs / 3600).toFixed();
                var minutes = Number(secs / 60).toFixed() % 60;
                var seconds = Number(secs).toFixed() % 60;

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
                onClicked: {
                    playerView.currentIndex = index;
                }
                onDoubleClicked: {
                    var row = visualModel.modelIndex(index);
                    if (playerModel.rowCount(row))
                        visualModel.rootIndex = row;
                    else
                        playerModel.play(row);
                }
            }

            Rectangle {
                id: rect
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.bottomMargin: 2
                anchors.leftMargin: 3
                anchors.rightMargin: 3
                border.color: 'lightgray'
                border.width: 1
                radius: 5
                state: item.isSelected ? 'selected' : ''

                Image {
                    id: imageColumn
                    anchors.left: parent.left
                    anchors.leftMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    width: 32
                    height: 32
                    source: model.playing ? "start.png" : (model.downloading ? "download.png" : model.imageUrl)
                }

                Column {
                    id: textColumn
                    anchors.left: imageColumn.right
                    anchors.verticalCenter: parent.verticalCenter
                    /*width: item.width - imageColumn.width - 60 */
                    Text { text: '<b>' + title + '</b>' }
                    Text { text: (item.isSelected && album) ? artist + ' - ' + album : artist }
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    text: formatDuration(duration)
                }

                states: State {
                    name: "selected"
                    PropertyChanges { target: rect; color: 'lightsteelblue'; border.color: 'darkgray' }
                }

                transitions: Transition {
                    PropertyAnimation { target: rect; properties: 'color,border.color'; duration: 300 }
                }
            }
        }
    }

    VisualDataModel {
        id: visualModel
        model: playerModel
        delegate: playerDelegate
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        icon: 'start.png'
        title: qsTr('Media player')
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'stop.png'
                text: qsTr('Stop')
                visible: playerModel.playing
                onClicked: playerModel.stop()
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
            }
        }
    }

    ListView {
        id: playerView

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        model: visualModel
        delegate: playerDelegate
        focus: true

        Keys.onPressed: {
            if (event.key == Qt.Key_Backspace && event.modifiers == Qt.NoModifier) {
                var oldIndex = visualModel.rootIndex;
                visualModel.rootIndex = visualModel.parentModelIndex();
                currentIndex = playerModel.row(oldIndex, visualModel.rootIndex);
            }
            else if (event.key == Qt.Key_Delete ||
                    (event.key == Qt.Key_Backspace && event.modifiers == Qt.ControlModifier)) {
                if (currentIndex >= 0)
                    playerModel.removeRow(currentIndex, visualModel.rootIndex);
            }
            else if (event.key == Qt.Key_Enter ||
                     event.key == Qt.Key_Return) {
                var row = visualModel.modelIndex(currentIndex);
                if (playerModel.rowCount(row))
                    visualModel.rootIndex = row;
                else
                    playerModel.play(row);
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: playerView
    }
}
