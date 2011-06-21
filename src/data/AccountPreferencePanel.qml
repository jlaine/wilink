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
import 'utils.js' as Utils

Panel {
    id: panel

    function save() {
    }

    color: 'transparent'

    GroupBox {
        id: accounts

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Chat accounts')

        Item {
            anchors.fill: accounts.contents

            PanelHelp {
                id: help

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr('In addition to your %1 account, %2 can connect to additional chat accounts such as Google Talk and Facebook.').replace('%1', application.organizationName).replace('%2', application.applicationName)
            }

            ListView {
                id: view

                anchors.top: help.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: scrollBar.left
                model: application.chatAccounts
                delegate: Item {
                    height: appStyle.icon.smallSize
                    width: view.width - 1

                    Image {
                        id: image

                        anchors.top: parent.top
                        anchors.left: parent.left
                        smooth: true
                        source: Utils.jidToDomain(modelData) == 'wifirst.net' ? 'wiLink.png' : 'peer.png'
                        height: appStyle.icon.smallSize
                        width: appStyle.icon.smallSize
                    }

                    Text {
                        anchors.left: image.right
                        anchors.leftMargin: appStyle.spacing.horizontal
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: modelData
                    }
                }
            }

            ScrollBar {
                id: scrollBar

                anchors.top: help.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                flickableItem: view
            }
        }
    }
}

