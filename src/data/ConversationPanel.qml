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
import 'utils.js' as Utils

Panel {
    id: panel

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        icon: vcard.avatar
        title: '<b>' + vcard.name + '</b> ' + stateText() + '<br/>' + extraText()
        z: 1

        function extraText() {
            var domain = Utils.jidToDomain(vcard.jid);
            if (domain == 'wifirst.net') {
                // for wifirst accounts, return the nickname if it is
                // different from the display name
                if (vcard.name != vcard.nickName)
                    return nickName;
                else
                    return '';
            } else {
                return vcard.jid;
            }
        }

        function stateText() {
            if (conversation.remoteState == QXmppMessage.Composing)
                return qsTr('is composing a message');
            else if (conversation.remoteState == QXmppMessage.Gone)
                return qsTr('has closed the conversation');
            else
                return '';
        }

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'call.png'
                text: qsTr('Call')

                onClicked: client.callManager.call(vcard.jid)

                Connections {
                    target: client.callManager
                    onCallStarted: {
                        var component = Qt.createComponent('CallWidget.qml');
                        var widget = component.createObject(widgetBar);
                        widget.call = call;
                    }
                }
            }

            ToolButton {
                icon: 'upload.png'
                text: qsTr('Send a file')

                onClicked: {
                    var dialog = window.fileDialog();
                    dialog.windowTitle = qsTr('Send a file');
                    dialog.fileMode = QFileDialog.ExistingFile;
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            var filePath = dialog.selectedFiles[i];
                            client.transferManager.sendFile(vcard.jid, filePath);
                        }
                    }
                }

                Connections {
                    target: client.transferManager
                    onJobStarted: {
                        var component = Qt.createComponent('TransferWidget.qml');
                        var widget = component.createObject(widgetBar);
                        widget.job = job;
                    }
                }
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }

        VCard {
            id: vcard
            jid: conversation.jid
        }
    }

    Column {
        id: widgetBar
        objectName: 'widgetBar'

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1
    }

    HistoryView {
        id: historyView

        anchors.top: widgetBar.bottom
        anchors.bottom: chatInput.top
        anchors.left: parent.left
        anchors.right: parent.right
        model: historyModel

        onParticipantClicked: chatInput.talkAt(participant)
    }

    ChatEdit {
        id: chatInput

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        onChatStateChanged: {
            conversation.localState = chatInput.chatState;
        }

        onReturnPressed: {
            if (Qt.isQtObject(conversation)) {
                var text = chatInput.text;
                if (conversation.sendMessage(text))
                    chatInput.text = '';
            }
        }
    }
}
