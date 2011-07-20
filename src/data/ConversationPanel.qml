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
import wiLink 2.0
import 'utils.js' as Utils

Panel {
    id: panel

    property alias jid: conversation.jid

    Conversation {
        id: conversation

        client: appClient
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: vcard.avatar
        title: {
            var text = vcard.name;
            if (conversation.remoteState == QXmppMessage.Composing)
                text += ' ' + qsTr('is composing a message');
            else if (conversation.remoteState == QXmppMessage.Gone)
                text += ' ' + qsTr('has closed the conversation');
            return text;
        }
        subTitle: {
            var domain = Utils.jidToDomain(vcard.jid);
            if (domain == 'wifirst.net') {
                // for wifirst accounts, return the nickname if it is
                // different from the display name
                if (vcard.name != vcard.nickName)
                    return vcard.nickName;
                else
                    return '';
            } else {
                return vcard.jid;
            }
        }
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'call.png'
                text: qsTr('Call')
                visible: vcard.features & VCard.VoiceFeature

                onClicked: {
                    var fullJid = vcard.jidForFeature(VCard.AudioFeature);
                    appClient.callManager.call(fullJid);
                }

                Connections {
                    target: appClient.callManager
                    onCallStarted: {
                        if (Utils.jidToBareJid(call.jid) == conversation.jid) {
                            var component = Qt.createComponent('CallWidget.qml');
                            var widget = component.createObject(widgetBar);
                            widget.call = call;
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'upload.png'
                text: qsTr('Send')
                visible: vcard.features & VCard.FileTransferFeature

                onClicked: {
                    var dialog = window.fileDialog();
                    dialog.windowTitle = qsTr('Send a file');
                    dialog.fileMode = QFileDialog.ExistingFile;
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            var filePath = dialog.selectedFiles[i];
                            var fullJid = vcard.jidForFeature(VCard.FileTransferFeature);
                            appClient.transferManager.sendFile(fullJid, filePath);
                        }
                    }
                }

                Connections {
                    target: appClient.transferManager
                    onJobStarted: {
                        if (Utils.jidToBareJid(job.jid) == conversation.jid) {
                            var component = Qt.createComponent('TransferWidget.qml');
                            var widget = component.createObject(widgetBar);
                            widget.job = job;
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'clear.png'
                text: qsTr('Clear');
                visible: true

                onClicked: {
                    conversation.historyModel.clear();
                }
            }
        }

        StatusPill {
            anchors.left: parent.left
            anchors.leftMargin: header.iconSize - 3
            anchors.top: parent.top
            anchors.topMargin: header.iconSize - 3
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
            opacity: header.iconSize > 0 ? 1: 0
            presenceStatus: vcard.status
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
        model: conversation.historyModel

        onParticipantClicked: chatInput.talkAt(participant)

        Connections {
            target: historyView.model
            onMessageReceived: {
                if (contacts.currentJid != jid) {
                    // show notification
                    var handle = application.showMessage(vcard.name, text, qsTranslate('ConversationPanel', 'Show this conversation'));
                    if (handle) {
                        handle.clicked.connect(function() { showConversation(jid); });
                    }

                    // alert window
                    window.alert();

                    // play a sound
                    application.soundPlayer.play(application.settings.incomingMessageSound);

                    // add pending message
                    rosterModel.addPendingMessage(jid);
                }
            }
        }
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
                if (conversation.sendMessage(text)) {
                    chatInput.text = '';
                    application.soundPlayer.play(application.settings.outgoingMessageSound);
                }
            }
        }
    }

    VCard {
        id: vcard
        jid: conversation.jid
    }

    Keys.forwardTo: historyView
}
