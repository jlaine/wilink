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
    id: homePage

    property QtObject panelSwapper

    Image {
        source: 'images/landing_bg_directory.jpg'
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        smooth: true
    }

    VCard {
        id: vcard
    }

    Column {
        id: main

        spacing: 8
        anchors.fill: homePage

        Item {
            id: header

            height: 50
            anchors.left: parent.left
            anchors.right: parent.right

            Rectangle {
                id: headerBackground
                anchors.fill: parent
                color: '#879fbd'
            }

            Text {
                horizontalAlignment: Text.AlignLeft
                text: qsTr('Welcome %1').replace('%1', vcard.name);
                color: 'white'
                font.pixelSize: 20
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: appStyle.margin.large
            }

            Button {
                id: button

                width: 200
                height: 30
                text: "Charger l'annuaire"
                anchors.top: header.top
                anchors.horizontalCenter: header.horizontalCenter
                anchors.margins: 10
                onClicked: {
                    console.log(vcard.name);
                    directoryXml.reload();
                }
            }

            Image {
                source: 'images/logo_header.png'
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: appStyle.margin.large
            }
        }


        Item {
            id: container

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.bottom: footer.top
            anchors.margins: appStyle.margin.large
        }

        Item {
            id: footer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 150

            Rectangle {
                id: footerBackground
                color: 'white'
                anchors.fill: parent
                opacity: 0.5
            }

            ListView {
                id: directory
                anchors.fill: footerBackground
                model: directoryXml
                delegate: directoryDelegate
                orientation: ListView.Horizontal
            }
        }
    }

    DirectoryXmlModel {
        id: directoryXml

        query: '/objects/object'

        XmlRole { name: 'name'; query: 'name/string()' }
        XmlRole { name: 'jabber_id'; query: 'jabber-id/string()' }
        XmlRole { name: 'avatar_url'; query: 'avatar-url/string()' }
        XmlRole { name: 'gender'; query: 'gender/string()' }
        XmlRole { name: 'birthday'; query: 'birthday/string()' }
        XmlRole { name: 'hometown'; query: 'hometown/string()' }
        XmlRole { name: 'first_name'; query: 'first-name/string()' }
        XmlRole { name: 'last_name'; query: 'last-name/string()' }
        XmlRole { name: 'relationship_status'; query: 'relationship-status/string()' }
    }

    Component.onCompleted: {
        directoryXml.url =  appSettings.wifirstBaseUrl + '/wilink/directory';
        directoryXml.username = panelSwapper.parent.webUsername;
        directoryXml.password = panelSwapper.parent.webPassword;
        directoryXml.reload();
    }

    Component {
        id: directoryDelegate

        Item {
            id: rect

            width: 135
            height: 145

            Column {
                id: person
                anchors.horizontalCenter: rect.horizontalCenter
                anchors.verticalCenter: rect.verticalCenter

                Image {
                    id: avatar
                    source: appSettings.wifirstBaseUrl + avatar_url
                    width: 120
                    height: 120
                }

                Text {
                    id: label
                    text: name
                    color: '#c7c7c7'
                    opacity: 0
                }
            }

            MouseArea {
                hoverEnabled: true
                anchors.fill: person
                onClicked: {
                    var jid = jabber_id;
                    var panel = swapper.findPanel('ChatPanel.qml');
                    if (panel) {
                        console.log(vcard.name);
                        if (panel.wifirstRosterReceived) {
                            // Wifirst roster received
                            if (panel.hasSubscribedWithWifirst(jid)) {
                                console.log('Wifirst roster received, opening chat with ' + jid);
                                panel.showConversation(jid);
                            } else {
                                console.log('Wifirst roster received, sending subscribe request to ' + jid);
                                panel.addSubscriptionRequest(jid);
                            }
                        } else {
                            // Wifirst roster NOT received, postponing all actions
                            console.log('Postponing chat opening with ' + jid + ' to when Wifirst roster will have been received');
                            panel.delayedOpening = {action: 'open_conversation', jid: jid};
                        }
                    }
                }
                onEntered: {
                    label.opacity = 1;
                }
                onExited: {
                    label.opacity = 0;
                }
            }
        }
    }
}

