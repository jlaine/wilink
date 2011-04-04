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
    anchors.topMargin: 10

    Component {
        id: historyDelegate
        Item {
            id: item
            height: column.height + 10
            width: item.ListView.view.width - 10
            x: 5

            Row {
                width: parent.width

                Image {
                    source: model.avatar
                    width: 32
                }
                Column {
                    id: column
                    width: parent.width - 32

                    Row {
                        x: 10

                        Text {
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pointSize: 7
                            text: model.from
                        }

                        Text {
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pointSize: 7
                            text: Qt.formatDateTime(model.date, 'dd MMM hh:mm')
                        }
                    }

                    Rectangle {
                        id: rect
                        height: bodyText.height + 10
                        border.color: model.received ? '#2689d6': '#7b7b7b'
                        border.width: 1
                        color: model.received ? '#e7f4fe' : '#fafafa'
                        radius: 8
                        width: parent.width

                        Text {
                            id: bodyText
                            anchors.centerIn: parent
                            font.pointSize: 10
                            width: rect.width - 20
                            text: model.html
                            textFormat: Qt.RichText
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }

    ListView {
        id: historyView
        anchors.fill: parent
        model: historyModel
        delegate: historyDelegate
    }
}
