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

Panel {
    id: panel

    function save() {
        if (openAtLogin.enabled)
            application.openAtLogin = openAtLogin.checked;
        application.showOfflineContacts = showOfflineContacts.checked;
        application.sortContactsByStatus = sortContactsByStatus.checked;
    }

    color: 'transparent'

    GroupBox {
        id: general

        anchors.top: parent.top
        anchors.bottom: about.top
        anchors.bottomMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height / 2
        title: qsTr('General options')

        Column {
            anchors.fill: general.contents
            spacing: appStyle.spacing.vertical

            CheckBox {
                id: showOfflineContacts

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.showOfflineContacts
                text: qsTr('Show offline contacts')
                onClicked: checked = !checked
            }

            CheckBox {
                id: sortContactsByStatus

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.sortContactsByStatus
                text: qsTr('Sort contacts by status')
                onClicked: checked = !checked
            }
        }
    }

    GroupBox {
        id: about

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 160
        title: qsTr('About %1').replace('%1', application.applicationName)

        Column {
            anchors.fill: about.contents
            spacing: appStyle.spacing.vertical

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: 64

                Image {
                    id: appIcon

                    anchors.top: parent.top
                    anchors.left: parent.left
                    source: 'wiLink-64.png'
                }

                Text {
                    id: appName

                    anchors.left: appIcon.right
                    anchors.right: parent.right
                    anchors.top: appIcon.top
                    anchors.margins: 6
                    font.bold: true
                    font.pixelSize: appStyle.font.largeSize
                    text: application.applicationName
                }

                Text {
                    id: appVersion

                    anchors.left: appIcon.right
                    anchors.right: parent.right
                    anchors.top: appName.bottom
                    anchors.margins: 6
                    text: qsTr('version %1').replace('%1', application.applicationVersion)
                }
            }

            CheckBox {
                id: openAtLogin

                anchors.left: parent.left
                anchors.right:  parent.right
                checked: application.openAtLogin
                enabled: application.isInstalled
                text: qsTr('Open at login')
                onClicked: checked = !checked
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 6
                iconSource: 'refresh.png'
                text: qsTr('Check for updates')
                onClicked: application.updatesDialog.check()
            }
        }
    }
}

