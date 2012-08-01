/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

import QtQuick 1.1
import wiLink 2.4
import 'scripts/utils.js' as Utils

Item {
    id: block

    property alias atYBeginning: historyView.atYBeginning
    property alias atYEnd: historyView.atYEnd
    property alias count: historyView.count
    property alias model: historyView.model
    signal participantClicked(string participant)

    clip: true

    Clipboard {
        id: clipboard
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
            id: highlight

            state: block.state
            width: parent.width - 1
            z: 10

            Button {
                id: copyButton

                anchors.right: cancelButton.left
                anchors.rightMargin: appStyle.spacing.horizontal
                anchors.verticalCenter: parent.verticalCenter
                iconStyle: 'icon-copy'
                text: qsTr('Copy')
                visible: false

                onClicked: {
                    // get selection
                    var selection = [];
                    for (var i = 0; i < historyView.model.count; i++) {
                        if (historyView.model.getProperty(i, 'selected'))
                            selection.push(i);
                    }

                    // copy selection
                    var text = '';
                    if (selection.length == 1) {
                        text = historyView.model.getProperty(selection[0], 'body');
                    } else {
                        for (var i in selection) {
                            var item = historyView.model.get(selection[i]);
                            text += '[ ' + item.from + ' - ' + Utils.formatDateTime(item.date) + ' ]\n';
                            text += item.body;
                            if (i < selection.length - 1)
                                text += '\n\n';
                        }
                    }
                    clipboard.copy(text);

                    // clear selection
                    block.state = '';
                    historyView.selectionStart = -1;
                    historyView.model.select(-1, -1);
                }
            }

            Button {
                id: cancelButton

                anchors.right: parent.right
                anchors.rightMargin: appStyle.spacing.horizontal
                anchors.verticalCenter: parent.verticalCenter
                iconStyle: 'icon-remove'
                text: qsTr('Cancel')
                visible: false

                onClicked: {
                    // cancel selection
                    block.state = '';
                    historyView.selectionStart = -1;
                    historyView.model.select(-1, -1);
                }
            }

            states: [
                State {
                    name: 'selection'
                    PropertyChanges { target: cancelButton; visible: 1 }
                    PropertyChanges { target: copyButton; visible: 1 }
                }
            ]
        }
        highlightMoveDuration: appStyle.highlightMoveDuration

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
                    sourceSize.height: appStyle.icon.normalSize
                    sourceSize.width: appStyle.icon.normalSize
                    height: appStyle.icon.normalSize
                    width: appStyle.icon.normalSize

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

                        Label {
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

                        Label {
                            id: dateText

                            anchors.right: fromText.right
                            color: model.received ? '#2689d6': '#7b7b7b'
                            font.pixelSize: appStyle.font.smallSize
                            // FIXME: this is a rough estimation of required width
                            opacity: fromText.width > 0.7 * (fromText.font.pixelSize * fromText.text.length + dateText.font.pixelSize * dateText.text.length) ? 1 : 0
                            text: Utils.formatDateTime(model.date)
                        }
                    }

                    Item {
                        id: rect
                        height: bodyText.height + 10
                        width: parent.width

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: block.state == 'selection'

                            onDoubleClicked: {
                                // start selection
                                historyView.currentIndex = model.index;
                                historyView.selectionStart = model.index;
                                historyView.model.select(model.index, model.index);
                                block.state = 'selection';
                            }

                            onEntered: {
                                if (block.state == 'selection') {
                                    // update selection
                                    historyView.currentIndex = model.index;
                                    historyView.model.select(historyView.selectionStart, historyView.currentIndex);
                                }
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: parent.height
                            border.color: model.received ? '#BCE8F1': '#E1E1E8'
                            border.width: model.action ? 0 : 1
                            color: model.selected ? '#aa86abd9' : (model.action ? 'transparent' : (model.received ? '#D9EDF7' : '#F7F7F9'))
                            radius: 8

                            Behavior on height {
                                NumberAnimation { duration: appStyle.animation.normalDuration }
                            }
                        }

                        Label {
                            id: bodyText

                            anchors.top: parent.top
                            anchors.topMargin: 5
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            width: rect.width - 20
                            text: model.html
                            textFormat: Qt.RichText
                            wrapMode: Text.Wrap

                            onLinkActivated: Qt.openUrlExternally(link)
                        }
                    }

                    Item {
                        height: 4
                        width: parent.width
                    }
                }

                ListView.onAdd: SequentialAnimation {
                    PropertyAction { target: item; property: "opacity"; value: 0 }
                    NumberAnimation { target: item; property: "opacity"; to: 1; duration: appStyle.animation.normalDuration; easing.type: Easing.InOutQuad }
                }

            }
        }

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
