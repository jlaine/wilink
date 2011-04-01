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
        id: historyDelegate
        Item {
            id: item
            height: 100
            width: parent.width

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

                Text {
                    anchors.fill: parent
                    text: body
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
        model: historyModel
        delegate: historyDelegate
    }

    ListView {
        id: historyView
        anchors.fill: parent
        model: visualModel
        delegate: historyDelegate
        focus: true

        Keys.onPressed: {
            if (event.key == Qt.Key_Backspace && event.modifiers == Qt.NoModifier) {
                var oldIndex = visualModel.rootIndex;
                visualModel.rootIndex = visualModel.parentModelIndex();
                currentIndex = historyModel.row(oldIndex, visualModel.rootIndex);
            }
            else if (event.key == Qt.Key_Delete ||
                    (event.key == Qt.Key_Backspace && event.modifiers == Qt.ControlModifier)) {
                if (currentIndex >= 0)
                    historyModel.removeRow(currentIndex, visualModel.rootIndex);
            }
            else if (event.key == Qt.Key_Enter ||
                     event.key == Qt.Key_Return) {
                var row = visualModel.modelIndex(currentIndex);
                if (historyModel.rowCount(row))
                    visualModel.rootIndex = row;
                else
                    historyModel.play(row);
            }
        }
    }
}
