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
import wiLink 2.0
import 'utils.js' as Utils

Panel {
    id: panel

    property alias iconSource: vcard.avatar
    property alias jid: conversation.jid
    property alias title: vcard.name

    Conversation {
        id: conversation

        onJidChanged: {
            conversation.client = accountModel.clientForJid(jid);
        }
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
                iconSource: 'image://icon/call'
                text: qsTr('Call')
                visible: vcard.features & VCard.VoiceFeature

                onClicked: {
                    var fullJid = vcard.jidForFeature(VCard.VoiceFeature);
                    conversation.client.callManager.call(fullJid);
                }

                Connections {
                    ignoreUnknownSignals: true
                    target: Qt.isQtObject(conversation.client) ? conversation.client.callManager : null

                    onCallStarted: {
                        if (Utils.jidToBareJid(call.jid) == conversation.jid) {
                            var component = Qt.createComponent('CallWidget.qml');

                            function finishCreation() {
                                if (component.status != Component.Ready)
                                    return;

                                var widget = component.createObject(widgetBar);
                                widget.call = call;
                            }

                            if (component.status == Component.Loading)
                                component.statusChanged.connect(finishCreation);
                            else
                                finishCreation();
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'image://icon/upload'
                text: qsTr('Send')
                visible: vcard.features & VCard.FileTransferFeature

                onClicked: {
                    var dialog = appNotifier.fileDialog();
                    dialog.windowTitle = qsTr('Send a file');
                    dialog.fileMode = QFileDialog.ExistingFile;
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            var filePath = dialog.selectedFiles[i];
                            var fullJid = vcard.jidForFeature(VCard.FileTransferFeature);
                            conversation.client.transferManager.sendFile(fullJid, filePath);
                        }
                    }
                }

                Connections {
                    ignoreUnknownSignals: true
                    target: Qt.isQtObject(conversation.client) ? conversation.client.transferManager : null

                    onJobStarted: {
                        if (Utils.jidToBareJid(job.jid) == conversation.jid) {
                            var component = Qt.createComponent('TransferWidget.qml');

                            function finishCreation() {
                                if (component.status != Component.Ready)
                                    return;

                                var widget = component.createObject(widgetBar);
                                widget.job = job;
                            }

                            if (component.status == Component.Loading)
                                component.statusChanged.connect(finishCreation);
                            else
                                finishCreation();
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'image://icon/clear'
                text: qsTr('Clear')
                visible: true

                onClicked: {
                    conversation.historyModel.clear();
                }
            }
        }

        StatusPill {
            property int iconOffset: header.iconMargin + header.iconSize - width * 0.75

            anchors.left: parent.left
            anchors.leftMargin: iconOffset
            anchors.top: parent.top
            anchors.topMargin: iconOffset
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
            opacity: header.width < 400 ? 0.0 : 1.0
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
                    if (appSettings.incomingMessageNotification) {
                        var handle = appNotifier.showMessage(vcard.name, text, qsTranslate('ConversationPanel', 'Show this conversation'));
                        if (handle) {
                            handle.clicked.connect(function() {
                                window.showAndRaise();
                                showConversation(jid);
                            });
                        }
                    }

                    // notify alert
                    appNotifier.alert();

                    // play a sound
                    appSoundPlayer.play(appSettings.incomingMessageSound);

                    // add pending message
                    rosterModel.addPendingMessage(jid);
                }
            }
        }

        DropArea {
            anchors.fill: parent
            enabled: vcard.features & VCard.FileTransferFeature
            visible: enabled

            onFilesDropped: {
                for (var i in files) {
                    var fullJid = vcard.jidForFeature(VCard.FileTransferFeature);
                    conversation.client.transferManager.sendFile(fullJid, files[i]);
                }
            }
        }

        // Button to fetch older messages.
        Rectangle {
            id: fetchPrevious

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: appStyle.margin.normal
            height: 32
            width: 200
            color: 'pink'
            opacity: 0

            Label {
                anchors.centerIn: parent
                text: qsTr('Fetch older messages')
            }

            MouseArea {
                id: fetchArea

                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    if (fetchPrevious.state == 'active') {
                        conversation.historyModel.fetchPreviousPage();
                    }
                }
            }

            state: (historyView.count && historyView.atYBeginning) ? 'active' : ''
            states: State {
                name: 'active'
                PropertyChanges { target: fetchPrevious; opacity: 1 }
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
                    appSoundPlayer.play(appSettings.outgoingMessageSound);
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
