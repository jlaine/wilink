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
            property Item textItem: bodyText

            id: item
            height: wrapper.height + 10
            width: item.ListView.view.width - 16
            x: 8

            Row {
                id: wrapper
                width: parent.width

                Image {
                    id: avatar
                    source: model.avatar
                    height: 32
                    width: 32
                }
                Item {
                    id: spacer
                    height: 32
                    width: 8
                }
                Column {
                    width: parent.width - avatar.width - spacer.width

                    Item {
                        id: header
                        height: fromText.height
                        width: parent.width
                        visible: !model.action

                        Text {
                            id: fromText
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pointSize: 7
                            text: model.from
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pointSize: 7
                            text: Qt.formatDateTime(model.date, 'dd MMM hh:mm')
                        }
                    }

                    Rectangle {
                        id: rect
                        height: bodyText.height + 10
                        border.color: model.received ? '#2689d6': '#7b7b7b'
                        border.width: model.action ? 0 : 1
                        color: model.action ? 'transparent' : (model.received ? '#e7f4fe' : '#fafafa')
                        radius: 8
                        width: parent.width

                        TextEdit {
                            id: bodyText
                            anchors.centerIn: parent
                            font.pointSize: 10
                            readOnly: true
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

/*
    ListModel {
        id: historyModel

        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Hello</p><p>How are you?</p>"
            received: true
        }
        ListElement {
            avatar: "home.png"
            date: ""
            from: "Me"
            html: "<p>Just fine, how about yourself?</p><p>How are you?</p>"
            received: false
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Where did you spend your holidays?</p>"
            received: true
        }
    }
*/

    ListView {
        id: historyView

        anchors.left: parent.left
        anchors.top: parent.top
        height: parent.height
        width: parent.width - scrollBar.width
        model: historyModel
        delegate: historyDelegate

        MouseArea {
            id: selector

            property real pressX
            property real pressY
            property list<Item> selection

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton

            onPressed: {
                historyView.interactive = false;
                pressX = mouse.x;
                pressY = mouse.y;

                for (var i = 0; i < selection.length; i++) {
                    var obj = selection[i];
                    if (obj)
                        obj.select(0, 0);
                }
                selection = []
            }

            onReleased: {
                historyView.interactive = true;
            }

            onPositionChanged: {
                function setSelection(item) {
                    if (!item)
                        return 0;
                    var start = mapToItem(item, pressX, pressY);
                    var startPos = item.positionAt(start.x, start.y);
                    var end = mapToItem(item, mouse.x, mouse.y);
                    var endPos = item.positionAt(end.x, end.y);
                    item.select(startPos, endPos);
                    return startPos >= 0 && endPos >= 0 && endPos != startPos;
                }

                // get current item
                historyView.currentIndex = historyView.indexAt(mouse.x, mouse.y);
                var textItem = historyView.currentItem ? historyView.currentItem.textItem : null;

                // update existing selections
                var newSelection = new Array();
                for (var i = 0; i < selection.length; i++) {
                    var obj = selection[i];
                    if (obj == textItem)
                        continue;
                    else if (setSelection(obj))
                        newSelection[newSelection.length] = obj;
                }
                if (setSelection(textItem))
                    newSelection[newSelection.length] = textItem;
                selection = newSelection;
            }
        }
    }

    Item {
        id: scrollBar

        property Flickable flickableItem: historyView

        anchors.right: parent.right
        height: parent.height
        width: 16

        Rectangle {
            id: scrollTrack
            anchors.fill: parent
            color: 'red'
        }

        Rectangle {
            id: handle

            property Flickable flickableItem: historyView

            color: 'blue'
            x: 0
            y: flickableItem.visibleArea.yPosition * flickableItem.height
            height: flickableItem.visibleArea.heightRatio * flickableItem.height
            width: parent.width
            radius: 5

            MouseArea {
                property Flickable flickableItem: historyView
                property int handleY: flickableItem ? Math.floor(handle.y / flickableItem.height * flickableItem.contentHeight) : NaN
                property real maxDragY: flickableItem ? flickableItem.height - handle.height : NaN

                anchors.fill: parent

                drag {
                    axis: Drag.YAxis
                    target: handle
                    minimumY: 0
                    maximumY: maxDragY
                }

                onPositionChanged: {
                    flickableItem.contentY = handleY
                }
            }
        }
    }
}
