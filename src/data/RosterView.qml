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

Item {
    id: block
    clip: true

    property string currentJid
    property alias model: view.model
    property alias title: titleText.text

    signal addClicked
    signal itemClicked(variant model)
    signal itemContextMenu(variant model, variant point)

    Rectangle {
        id: background

        width: parent.height
        height: parent.width
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width

        gradient: Gradient {
            GradientStop { id: backgroundStop1; position: 0.0; color: '#e7effd' }
            GradientStop { id: backgroundStop2; position: 1.0; color: '#cbdaf1' }
        }

        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: background.height
        }
    }

    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
//        color: '#b0c4de'
        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#597fbe' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }
        border.color: '#aa567dbc'
        border.width: 1

        width: parent.width
        height: 24
        z: 1

        Text {
            id: titleText

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            color: '#ffffff'
            font.bold: true
        }

        Button {
            anchors.right: parent.right
            anchors.rightMargin:2
            anchors.verticalCenter: parent.verticalCenter
            height: 20
            width: 20
            iconSource: 'add.png'

            onClicked: block.addClicked()
        }
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        focus: true

        delegate: Item {
            id: item

            width: parent.width
            height: 30
            state: currentJid == model.jid ? 'selected' : ''

            Item {
                anchors.fill: parent
                anchors.margins: 2

                Rectangle {
                    id: highlight

                    anchors.fill: parent
                    border.color: '#ffb0c4de'
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { id:stop1; position:0.0; color: '#33b0c4de' }
                        GradientStop { id:stop2; position:0.5; color: '#ffb0c4de' }
                        GradientStop { id:stop3; position:1.0; color: '#33b0c4de' }
                    }
                    opacity: 0
                    radius: 5
                    smooth: true
                }

                Image {
                    id: avatar

                    anchors.left: parent.left
                    anchors.leftMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    width: 22
                    height: 22
                    smooth: true
                    source: model.avatar

                }

                // status gradients
                Gradient { // Offline
                    id: offlineGradient
                    GradientStop { position: 0.0; color: '#dfdfdf' }
                    GradientStop { position: 1.0; color: '#999999' }
                }
                Gradient { // Available
                    id: availableGradient
                    GradientStop { position: 0.0; color: '#64ff64' }
                    GradientStop { position: 1.0; color: '#009600' }
                }
                Gradient { // Away
                    id: awayGradient
                    GradientStop { position: 0.0; color: '#ffc800' }
                    GradientStop { position: 1.0; color: '#d28c00' }
                }
                Gradient { // Busy
                    id: busyGradient
                    GradientStop { position: 0.0; color: '#ff6464' }
                    GradientStop { position: 1.0; color: '#dd0000' }
                }

                Rectangle {
                    id: status
                    anchors.left: avatar.right
                    anchors.leftMargin: 3
                    anchors.verticalCenter: parent.verticalCenter
                    width: 10
                    height: 10
                    border.width: 1
                    border.color: {
                        switch(model.status) {
                            case QXmppPresence.Offline:
                            case QXmppPresence.Invisible:
                                return '#999999';
                            case QXmppPresence.Online:
                            case QXmppPresence.Chat:
                                return '#006400';
                            case QXmppPresence.Away:
                            case QXmppPresence.XA:
                                return '#c86400';
                            case QXmppPresence.DND:
                                return '#640000';
                        }
                    }
                    gradient: {
                        switch(model.status) {
                            case QXmppPresence.Offline:
                            case QXmppPresence.Invisible:
                                return offlineGradient;
                            case QXmppPresence.Online:
                            case QXmppPresence.Chat:
                                return availableGradient;
                            case QXmppPresence.Away:
                            case QXmppPresence.XA:
                                return awayGradient;
                            case QXmppPresence.DND:
                                return busyGradient;
                        }
                    }
                    opacity: (model.status != undefined) ? 1 : 0
                    radius: 20
                    smooth: true
                }

                Text {
                    anchors.left: status.right
                    anchors.right: label.right
                    anchors.leftMargin: 3
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    font.pixelSize: 12
                    text: model.name
                }

                Rectangle {
                    anchors.fill: label
                    anchors.margins: -3
                    border.color: '#597fbe'
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: '#66597fbe' }
                        GradientStop { position: 0.6; color: '#597fbe' }
                        GradientStop { position: 1.0; color: '#66597fbe' }
                    }

                    opacity: model.messages > 0 ? 1 : 0
                    radius: 10
                    smooth: true
                }

                Text {
                    id: label

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 6
                    color: 'white'
                    font.bold: true
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignHCenter
                    opacity: model.messages > 0 ? 1 : 0
                    text: model.messages
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (mouse.button == Qt.LeftButton) {
                            block.itemClicked(model);
                        } else if (mouse.button == Qt.RightButton) {
                            block.itemContextMenu(model, block.mapFromItem(item, mouse.x, mouse.y));
                        }
                    }
                }
            }

            states: State {
                name: 'selected'
                PropertyChanges { target: highlight; opacity: 1.0 }
            }

            transitions: Transition {
                PropertyAnimation { target: highlight; properties: 'opacity'; duration: 300 }
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
