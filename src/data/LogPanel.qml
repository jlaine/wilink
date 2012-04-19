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
import QXmpp 0.4
import wiLink 2.0

Panel {
    id: panel

    Clipboard {
        id: clipboard
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'image://icon/debug'
        title: qsTr('Debugging console')
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'image://icon/start'
                text: qsTr('Start')
                visible: !logModel.enabled

                onClicked: logModel.enabled = true
            }

            ToolButton {
                iconSource: 'image://icon/stop'
                text: qsTr('Stop')
                visible: logModel.enabled

                onClicked: logModel.enabled = false
            }

            ToolButton {
                iconSource: 'image://icon/copy'
                text: qsTr('Copy')
                onClicked: {
                    // copy the 20 last messages, to avoid freeze with important log
                    var text = '';
                    var count = logView.model.count;
                    for (var i = Math.max(0, count - 20); i < count; i++) {
                        var item = logView.model.get(i);
                        text += Qt.formatDateTime(item.date, 'hh:mm:ss') + '\n';
                        text += item.content + '\n';
                    }
                    clipboard.copy(text);
                }
            }

            ToolButton {
                iconSource: 'image://icon/clear'
                text: qsTr('Clear')

                onClicked: logModel.clear()
            }
        }
    }

    ScrollView {
        id: logView

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: LogModel {
            id: logModel
            logger: appLogger
        }

        delegate: Rectangle {
            color: colorForType(model.type)
            width: view.width - 1
            height: content.paintedHeight + 2

            function colorForType(type) {
                if (type == QXmppLogger.ReceivedMessage)
                    return '#ccffcc';
                else if (type == QXmppLogger.SentMessage)
                    return '#ccccff';
                else if (type == QXmppLogger.WarningMessage)
                    return '#ff9595';
                return 'white';
            }

            Label {
                id: date

                anchors.left: parent.left
                anchors.top: parent.top
                text: '<b>' + Qt.formatDateTime(model.date, 'hh:mm:ss') + '</b>'
                width: appStyle.font.normalSize * 6
            }

            Label {
                id: content

                anchors.top: parent.top
                anchors.left: date.right
                anchors.right: parent.right
                text: model.content
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }
        }
    }
}

