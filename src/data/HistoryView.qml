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
import wiLink 1.2

Item {
    id: block

    property alias model: historyView.model
    signal participantClicked(string participant)

    ListHelper {
        id: listHelper
        model: historyView.model
    }

    ListView {
        id: historyView

        property bool scrollBarAtBottom
        property bool bottomChanging: false
        property int messageSelected
        property int selectionStart
        property int selectionEnd

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
                id: item

                property alias textItem: bodyText

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
                        height: appStyle.font.smallSize + 4
                        width: parent.width
                        visible: !model.action

                        Text {
                            id: fromText

                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            color: model.received ? '#2689d6': '#7b7b7b'
                            elide: Text.ElideRight
                            font.pixelSize: appStyle.font.smallSize
                            text: model.from

                            MouseArea {
                                anchors.fill: parent
                                onClicked: block.participantClicked(model.from);
                            }
                        }

                        Text {
                            id: dateText

                            anchors.right: fromText.right
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pixelSize: appStyle.font.smallSize
                            // FIXME: this is a rough estimation of required width
                            opacity: fromText.width > 0.7 * (fromText.font.pixelSize * fromText.text.length + dateText.font.pixelSize * dateText.text.length) ? 1 : 0
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

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                copyButtonLoader.sourceComponent = copyButtonComponent;
                                var rectCoords = mapToItem(historyView.contentItem, rect.x, rect.y);
                                historyView.messageSelected = historyView.indexAt(rectCoords.x, rectCoords.y);

                                // set selection
                                if (block.state == 'selection') {
                                    selection.endPos = rect.mapToItem(historyView.contentItem, 0, rect.y + bodyText.height).y;
                                } else {
                                    selection.startPos = fromText.mapToItem(historyView.contentItem, 0, fromText.y).y;
                                    selection.endPos = rect.mapToItem(historyView.contentItem, 0, rect.y + bodyText.height).y;
                                }
                            }
                            onExited: {
                                copyButtonLoader.sourceComponent = undefined;
                            }
                        }

                        Text {
                            id: bodyText

                            anchors.centerIn: parent
                            width: rect.width - 20
                            text: model.html
                            textFormat: Qt.RichText
                            wrapMode: Text.Wrap

                            onLinkActivated: Qt.openUrlExternally(link)
                        }

                        Loader {
                            id: copyButtonLoader
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            visible: block.state != 'selection'
                        }

                        states: State {
                            name: "no-copyButton"
                            when: rect.width < 100
                            PropertyChanges { target: copyButtonLoader; visible: false }
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

            property variant press
            property int selectionStart: 0
            property int selectionEnd: -1

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton

            onPressed: {
                historyView.interactive = false;
                press = {
                    'x': mouse.x,
                    'y': mouse.y,
                    'index': historyView.indexAt(mouse.x + historyView.contentX, mouse.y + historyView.contentY),
                };

                // clear old selection
                for (var i = selectionStart; i <= selectionEnd; i++) {
                    historyView.currentIndex = i;
                    if (!historyView.currentItem)
                        continue;
                    var item = historyView.currentItem.textItem;
                    item.select(0, 0);
                }
                selectionStart = 0;
                selectionEnd = -1;
            }

            onReleased: {
                historyView.interactive = true;
            }

            onPositionChanged: {
                var start, end;
                var current = {
                    'x': mouse.x,
                    'y': mouse.y,
                    'index': historyView.indexAt(mouse.x + historyView.contentX, mouse.y + historyView.contentY),
                };

                // determine start / end
                if (current.index > press.index || (current.index == press.index && current.x > press.x)) {
                    start = press;
                    end = current;
                } else {
                    start = current;
                    end = press;
                }

                // check bounds
                if (start.index < 0 || end.index < 0)
                    return;

                function setSelection(item) {
                    if (!item)
                        return 0;
                    var startCoord = mapToItem(item, start.x, start.y);
                    var startPos = item.positionAt(startCoord.x, startCoord.y);
                    var endCoord = mapToItem(item, end.x, end.y);
                    var endPos = item.positionAt(endCoord.x, endCoord.y);
                    item.select(startPos, endPos);
                    return startPos >= 0 && endPos >= 0 && endPos != startPos;
                }

                // set new selection
                for (var i = start.index; i <= end.index; i++) {
                    historyView.currentIndex = i;
                    if (!historyView.currentItem)
                        continue;
                    var item = historyView.currentItem.textItem;
                    setSelection(item);
                }

                // clear old selection
                for (var i = selectionStart; i <= selectionEnd; i++) {
                    historyView.currentIndex = i;
                    if (!historyView.currentItem)
                        continue;
                    if (i < start.index || i > end.index) {
                        var item = historyView.currentItem.textItem;
                        item.select(0, 0);
                    }
                }

                // store selection
                selectionStart = start.index;
                selectionEnd = end.index;
            }
        }
*/
    }

    MouseArea {
        id: cancelSelection

        anchors.fill: historyView
        enabled: block.state == 'selection'
        onClicked: {
            // cancel selection
            block.state = '';
            selection.startPos = 0;
            selection.endPos = 0;
        }
    }

    Rectangle {
        id: selection

        property int startPos: 0
        property int endPos: 0

        anchors.left: parent.left
        anchors.right: scrollBar.left
        color: '#aa86abd9'
        visible: false

        y: startPos - historyView.contentY
        height: endPos - startPos

        Button {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            iconSource: 'copy.png'
            iconSize: appStyle.icon.tinySize
            text: qsTr('Copy selection')
            visible: parent.height > 0

            onClicked: {
                cancelSelection.clicked(null)
                historyView.selectionEnd = Math.min(listHelper.count-1, historyView.messageSelected);

                // copy elements
                copyHelper.text = '';
                for( var i = historyView.selectionStart; i <= historyView.selectionEnd; i++) {
                    var item = listHelper.get(i);
                    copyHelper.text += '>> ' + item.from + ':\n' + item.body + '\n';
                    copyHelper.selectAll();
                    copyHelper.copy();
                }
            }
        }
    }

    Component {
        id: copyButtonComponent

        Button {
            iconSource: 'resize.png'
            iconSize: appStyle.icon.tinySize
            text: qsTr('Select')

            onClicked: {
                // start selection
                block.state = 'selection';
                historyView.selectionStart = Math.max(0, historyView.messageSelected);
            }
        }
    }

    TextEdit {
        id: copyHelper
        visible: false
    }

    Rectangle {
        id: border

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: '#597fbe'
        width: 1
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: historyView

        onPositionChanged: {
            if (!historyView.bottomChanging) {
                // store whether we are at the bottom
                historyView.scrollBarAtBottom = historyView.atYEnd;
            }
        }
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

    Keys.forwardTo: scrollBar

    states: State {
        name: 'selection'
        PropertyChanges { target: selection; visible: true }
    }
}
