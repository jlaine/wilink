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

Dialog {
    id: dialog
    title: qsTr("Preferences")

    onAccepted: {
        generalOptions.save();
        parent.hide();
    }

    Item {
        anchors.fill: contents
        anchors.margins: 6

        ListView {
            id: tabList

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 100

            model: ListModel {
                ListElement {
                    avatar: 'options.png'
                    name: 'General'
                }
                ListElement {
                    avatar: 'audio-output.png'
                    name: 'Sound'
                }
                ListElement {
                    avatar: 'share.png'
                    name: 'Shares'
                }
            }
            delegate: Row {
                spacing: appStyle.verticalSpacing

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: model.avatar
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.name
                }
            }
        }

        Item {
            id: generalOptions

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: tabList.right
            anchors.right: parent.right

            function save() {
                if (openAtLogin.enabled)
                    application.openAtLogin = openAtLogin.checked;
                application.showOfflineContacts = showOfflineContacts.checked;
                application.sortContactsByStatus = sortContactsByStatus.checked;
            }

            Column {
                id: prefs

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: appStyle.horizontalSpacing

                CheckBox {
                    id: openAtLogin

                    anchors.left: parent.left
                    anchors.right:  parent.right
                    checked: application.openAtLogin
                    enabled: application.isInstalled
                    text: qsTr('Open at login')
                }

                CheckBox {
                    id: showOfflineContacts

                    anchors.left: parent.left
                    anchors.right:  parent.right
                    checked: application.showOfflineContacts
                    text: qsTr('Show offline contacts')
                }

                CheckBox {
                    id: sortContactsByStatus

                    anchors.left: parent.left
                    anchors.right:  parent.right
                    checked: application.sortContactsByStatus
                    text: qsTr('Sort contacts by status')
                }
            }

            Item {
                anchors.top: prefs.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right

                Image {
                    id: appIcon
                    anchors.left: parent.left
                    source: 'wiLink-64.png'
                }

                Text {
                    id: appName

                    anchors.left: appIcon.right
                    anchors.top: appIcon.top
                    anchors.margins: 6
                    font.bold: true
                    font.pixelSize: 20
                    text: application.applicationName
                }

                Text {
                    anchors.left: appIcon.right
                    anchors.top: appName.bottom
                    anchors.margins: 6
                    font.pixelSize: 16
                    text: qsTr('version %1').replace('%1', application.applicationVersion)
                }
            }
        }
        
    }
}

