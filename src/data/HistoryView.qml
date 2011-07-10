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
        property int selectionStart: -1

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cacheBuffer: 400
        delegate: historyDelegate
        header: Rectangle { height: 2 }
        spacing: 6

        highlight: Item {
            z: 10

            Button {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconSource: block.state == 'selection' ? 'copy.png' : 'resize.png'
                iconSize: appStyle.icon.tinySize
                text: block.state == 'selection' ? qsTr('Copy selection') : qsTr('Select')

                onClicked: {
                    if (state == 'selection') {
                        // copy selection
                        var text = '';
                        for (var i = 0; i <= listHelper.count; i++) {
                            var item = listHelper.get(i);
                            if (item.selected) {
                                text += '>> ' + item.from + ':\n' + item.body + '\n';
                            }
                        }
                        copyHelper.text = text;
                        copyHelper.selectAll();
                        copyHelper.copy();

                        // clear selection
                        block.state = '';
                        historyView.selectionStart = -1;
                        historyView.model.select(-1, -1);
                    } else {
                        // start selection
                        block.state = 'selection';
                        historyView.selectionStart = historyView.currentIndex;
                        historyView.model.select(historyView.selectionStart, historyView.selectionStart);
                    }
                }
            }
        }

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
                        color: model.selected ? '#aa86abd9' : (model.action ? 'transparent' : (model.received ? '#e7f4fe' : '#fafafa'))
                        radius: 8
                        width: parent.width

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                var rectCoords = mapToItem(historyView.contentItem, rect.x, rect.y);
                                historyView.currentIndex = historyView.indexAt(rectCoords.x, rectCoords.y);

                                // set selection
                                if (block.state == 'selection') {
                                    historyView.model.select(historyView.selectionStart, historyView.currentIndex);
                                }
                            }
                            onExited: {
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

                        Image {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            source: model.selected ? 'checkbox-checked.png' : ''
                        }
                    }

                    Item {
                        height: 4
                        width: parent.width
                    }
                }
            }
        }

    }

    MouseArea {
        id: cancelSelection

        anchors.fill: historyView
        enabled: block.state == 'selection'
        onClicked: {
            // cancel selection
            block.state = '';
            historyView.selectionStart = -1;
            historyView.model.select(-1, -1);
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
}
