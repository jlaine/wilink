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
import QXmpp 0.4
import wiLink 1.2

Panel {
    id: panel

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'options.png'
        title: qsTr('Debugging console')
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                iconSource: 'start.png'
                text: qsTr('Start')
                visible: !logModel.enabled

                onClicked: logModel.enabled = true
            }

            ToolButton {
                iconSource: 'stop.png'
                text: qsTr('Stop')
                visible: logModel.enabled

                onClicked: logModel.enabled = false
            }

            ToolButton {
                iconSource: 'refresh.png'
                text: qsTr('Clear')

                onClicked: logModel.clear()
            }

            ToolButton {
                iconSource: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }
    }

    ListView {
        id: view

        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: header.bottom
        anchors.bottom: parent.bottom

        model: LogModel {
            id: logModel

            logger: window.client.logger
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

            Text {
                id: date

                anchors.left: parent.left
                anchors.top: parent.top
                text: '<b>' + Qt.formatDateTime(model.date, 'hh:mm:ss') + '</b>'
                width: 70
            }

            Text {
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


    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }

}

