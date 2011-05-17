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

    Column {
        id: widgetBar
        objectName: 'widgetBar'

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1
    }

    Item {
        anchors.top: widgetBar.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
    
        HistoryView {
            id: historyView

            property bool scrollBarAtBottom

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: historyScrollBar.left
            model: historyModel

            Connections {
                target: historyView.model
                onBottomAboutToChange: {
                    // test if scrollbar should follow bottom change
                    if (0 == Math.round((1 - historyScrollBar.position - historyScrollBar.pageSize) * historyScrollBar.flickableItem.contentHeight)) {
                        historyView.scrollBarAtBottom = true
                    } else {
                        historyView.scrollBarAtBottom = false
                    }
                }
                onBottomChanged: {
                    if( historyView.scrollBarAtBottom ) {
                        historyView.positionViewAtIndex(historyView.count - 1, ListView.End)
                    }
                }
            }
        }

        ScrollBar {
            id: historyScrollBar

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: Qt.isQtObject(participantModel) ? participantView.left : parent.right
            flickableItem: historyView
        }

        ParticipantView {
            id: participantView

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: participantScrollBar.left
            model: participantModel
            visible: Qt.isQtObject(participantModel)
        }

        ScrollBar {
            id: participantScrollBar

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            flickableItem: participantView
            visible: participantView.visibleArea.heightRatio < 1
        }
    }

    Rectangle {
        id: footer

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        color: '#dfdfdf'
        height: chatInput.height + 8

        ChatEdit {
            id: chatInput

            model: participantModel
            x: 4
            y: 4
            width: parent.width - 8
        }
    }

    Connections {
        target: chatInput

        onChatStateChanged: {
            if (conversationHasState)
                conversation.state = chatInput.chatState;
        }

        onReturnPressed: {
            if (Qt.isQtObject(conversation)) {
                var text = chatInput.text;
                if (conversation.sendMessage(text))
                    chatInput.text = '';
            }
        }
    }

    Connections {
        target: historyView
        onParticipantClicked: chatInput.talkAt(participant)
    }

    Connections {
        target: participantView
        onParticipantClicked: chatInput.talkAt(participant)
    }
}
