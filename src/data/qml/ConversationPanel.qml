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

Panel {
    id: panel

    property alias iconSource: vcard.avatar
    property alias jid: conversation.jid
    property alias title: vcard.name
    property alias presenceStatus: vcard.status

    SoundLoader {
        id: incomingMessageSound
        source: appSettings.incomingMessageSound ? 'sounds/message-incoming.ogg' : ''
    }

    SoundLoader {
        id: outgoingMessageSound
        source: appSettings.outgoingMessageSound ? 'sounds/message-outgoing.ogg' : ''
    }

    Conversation {
        id: conversation

        onJidChanged: {
            conversation.client = accountModel.clientForJid(jid);
        }
    }

    Column {
        id: widgetBar
        objectName: 'widgetBar'

        anchors.top: parent.top
        anchors.topMargin: appStyle.margin.large
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 2
        z: 1

        /** Call widget.
         */
        Connections {
            ignoreUnknownSignals: true
            target: Qt.isQtObject(conversation.client) ? conversation.client.callManager : null

            onCallStarted: {
                if (Utils.jidToBareJid(call.jid) == conversation.jid) {
                    // NOTE: for some reason, if we don't store the call in a local
                    // variable, finishCreation seems to receive an incomplete call
                    // object.
                    var currentCall = call;
                    var component = Qt.createComponent('CallWidget.qml');

                    function finishCreation() {
                        if (component.status != Component.Ready)
                            return;

                        component.createObject(widgetBar, {call: currentCall});
                    }

                    if (component.status == Component.Loading)
                        component.statusChanged.connect(finishCreation);
                    else
                        finishCreation();
                }
            }
        }

        /** File transfer widget.
         */
        Connections {
            ignoreUnknownSignals: true
            target: Qt.isQtObject(conversation.client) ? conversation.client.transferManager : null

            onJobStarted: {
                if (Utils.jidToBareJid(job.jid) == conversation.jid) {
                    // NOTE: for some reason, if we don't store the job in a local
                    // variable, finishCreation seems to receive an incomplete job
                    // object.
                    var currentJob = job;
                    var component = Qt.createComponent('TransferWidget.qml');

                    function finishCreation() {
                        if (component.status != Component.Ready)
                            return;

                        component.createObject(widgetBar, {job: currentJob});
                    }

                    if (component.status == Component.Loading)
                        component.statusChanged.connect(finishCreation);
                    else
                        finishCreation();
                }
            }
        }
    }

    HistoryView {
        id: historyView

        anchors.top: widgetBar.bottom
        anchors.bottom: chatInput.top
        anchors.left: parent.left
        anchors.right: parent.right
        model: conversation.historyModel

        onParticipantClicked: {
            if (mouse.button == Qt.LeftButton) {
                chatInput.talkAt(participant.name);
            }
        }

        historyHeader: Item {
            id: fetchPrevious

            anchors.horizontalCenter: parent.horizontalCenter
            height: 0
            width: appStyle.icon.smallSize + 2*appStyle.margin.normal + appStyle.spacing.horizontal + fetchLabel.width
            opacity: 0

            Rectangle {
                anchors.fill: parent
                opacity: 0.8
                radius: appStyle.margin.large
                smooth: true
                gradient: Gradient {
                    GradientStop { position: 0; color: '#9bbdf4' }
                    GradientStop { position: 1; color: '#90acd8' }
                }
            }

            Icon {
                id: fetchIcon

                anchors.left: parent.left
                anchors.leftMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                style: 'icon-info-sign'
            }

            Label {
                id: fetchLabel

                anchors.left: fetchIcon.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.verticalCenter: parent.verticalCenter
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

            states: State {
                name: 'active'
                when: conversation.historyModel.hasPreviousPage
                PropertyChanges { target: fetchPrevious; opacity: 1; height: appStyle.icon.smallSize + 2*appStyle.margin.normal }
            }
        }

        historyFooter: Rectangle {
            id: footer

            property alias iconStyle: icon.style

            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0
            height: 0
            width: parent.width - 16

            Image {
                id: avatar

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 6
                asynchronous: true
                source: vcard.avatar
                sourceSize.height: appStyle.icon.normalSize
                sourceSize.width: appStyle.icon.normalSize
                height: appStyle.icon.normalSize
                width: appStyle.icon.normalSize

                Icon {
                    id: icon

                    color: '#333'
                    font.pixelSize: 20
                    opacity: 0.5
                    anchors.top: parent.top
                    anchors.topMargin: -5
                    anchors.left: parent.right
                    anchors.leftMargin: 10
                }
            }

            states: [
                State {
                    name: 'composing'
                    when: conversation.remoteState == QXmppMessage.Composing
                    PropertyChanges { target: footer; opacity: 1; height: 48; iconStyle: 'icon-comment-alt' }
                    StateChangeScript { script: view.positionViewAtEnd() }
                },
                State {
                    name: 'paused'
                    when: conversation.remoteState == QXmppMessage.Paused
                    PropertyChanges { target: footer; opacity: 0.7; height: 48; iconStyle: 'icon-cloud' }
                }
            ]
        }

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
                    incomingMessageSound.start();

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

    }

    ChatEdit {
        id: chatInput

        callButton.visible: vcard.features & VCard.VoiceFeature

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        menuComponent: Menu {
            id: menu

            onItemClicked: {
                var item = menu.model.get(index);
                if (item.action == 'call') {
                    var fullJid = vcard.jidForFeature(VCard.VoiceFeature);
                    conversation.client.callManager.call(fullJid);
                } else if (item.action == 'send') {
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
                } else if (item.action == 'clear') {
                    conversation.historyModel.clear();
                }
            }

            Component.onCompleted: {
                if (vcard.features & VCard.FileTransferFeature) {
                    menu.model.append({
                        action: 'send',
                        iconStyle: 'icon-upload',
                        text: qsTr('Send a file')});
                }
                menu.model.append({
                    action: 'clear',
                    iconStyle: 'icon-remove',
                    text: qsTr('Clear history')});
            }
        }

        onCallTriggered: {
            var fullJid = vcard.jidForFeature(VCard.VoiceFeature);
            conversation.client.callManager.call(fullJid);
        }

        onChatStateChanged: {
            conversation.localState = chatInput.chatState;
        }

        onReturnPressed: {
            if (Qt.isQtObject(conversation)) {
                var text = chatInput.text;
                if (conversation.sendMessage(text)) {
                    chatInput.text = '';
                    outgoingMessageSound.start();
                }
            }
        }
    }

    resources: [
        VCard {
            id: vcard
            jid: conversation.jid
        }
    ]

    Keys.forwardTo: historyView
}
