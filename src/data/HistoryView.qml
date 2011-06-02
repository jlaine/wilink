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
    id: block

    property alias model: historyView.model
    signal participantClicked(string participant)

    ListView {
        id: historyView

        property bool scrollBarAtBottom
        property bool bottomChanging: false

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cacheBuffer: 400
        delegate: historyDelegate
        header: Rectangle { height: 2 }
        spacing: 6

        Component {
            id: historyDelegate
            Row {
                property Item textItem: bodyText

                id: item
                spacing: 8
                width: parent.width - 16
                x: 8

                Image {
                    id: avatar
                    asynchronous: true
                    source: model.avatar
                    height: 32
                    width: 32

                    MouseArea {
                        anchors.fill: parent
                        onClicked: block.participantClicked(model.from);
                    }
                }
                Column {
                    width: parent.width - parent.spacing - avatar.width

                    Item {
                        id: header
                        height: 14
                        width: parent.width
                        visible: !model.action

                        Text {
                            id: fromText
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pixelSize: 10
                            text: model.from

                            MouseArea {
                                anchors.fill: parent
                                onClicked: block.participantClicked(model.from);
                            }
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pixelSize: 10
                            text: Qt.formatDateTime(model.date, 'dd MMM hh:mm')
                        }
                    }

                    Rectangle {
                        id: rect
                        height: bodyText.height + 10
                        border.color: model.received ? '#84bde8': '#ababab'
                        border.width: model.action ? 0 : 1
                        color: model.action ? 'transparent' : (model.received ? '#e7f4fe' : '#fafafa')
                        radius: 8
                        width: parent.width

                        Text {
                            id: bodyText
                            anchors.centerIn: parent
                            font.pixelSize: 12
                            width: rect.width - 20
                            text: model.html
                            textFormat: Qt.RichText
                            wrapMode: Text.WordWrap
                            onLinkActivated: Qt.openUrlExternally(link)
                        }
                    }

                    Item {
                        height: 4
                        width: parent.width
                    }
                }
            }
        }

/*
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
*/
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: historyView

    }

    Connections {
        target: historyView
        onHeightChanged: {
            // follow bottom if we were at the bottom
            if (historyView.scrollBarAtBottom) {
                historyView.positionViewAtIndex(historyView.count - 1, ListView.End);
            }
        }
    }

    Connections {
        target: historyView.model
        onBottomAboutToChange: {
            // store whether we are at the bottom
            historyView.bottomChanging = true;
            historyView.scrollBarAtBottom = historyView.atYEnd;
        }
        onBottomChanged: {
            // follow bottom if we were at the bottom
            if (historyView.scrollBarAtBottom) {
                historyView.positionViewAtIndex(historyView.count - 1, ListView.End);
            }
            historyView.bottomChanging = false;
        }
    }

    Connections {
        target: scrollBar
        onPositionChanged: {
            if (!historyView.bottomChanging) {
                // store whether we are at the bottom
                historyView.scrollBarAtBottom = historyView.atYEnd;
            }
        }
    }
}
