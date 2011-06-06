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

Panel {
    id: panel

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'diagnostics.png'
        title: qsTr('Service discovery')
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                iconSource: 'back.png'
                text: qsTr('Go back')
                enabled: crumbBar.model.count > 0

                onClicked: crumbBar.goBack()
            }

            ToolButton {
                iconSource: 'refresh.png'
                text: qsTr('Refresh')

                onClicked: {
                    view.model.refresh();
                }
            }

            ToolButton {
                iconSource: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }
    }

    CrumbBar {
        id: crumbBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        view: view
    }

    ListView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: parent.bottom

        model: DiscoveryModel {
            property string rootName: 'Wifirst'
            manager: window.client.discoveryManager
            rootJid: 'wifirst.net'
        }

        delegate: Rectangle {
            width: view.width - 1
            height: 34

            Image {
                id: image

                anchors.top: parent.top
                anchors.left: parent.left
                source: 'peer.png'
            }

            Text {
                anchors.left: image.right
                anchors.top: parent.top
                text: '<b>' + model.name + '</b><br/>' + model.jid + (model.node ? ' (' + model.node + ')' : '')
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    crumbBar.model.append({'name': view.model.rootJid, 'jid': view.model.rootJid, 'node': view.model.rootNode});
                    view.model.rootName = model.jid;
                    view.model.rootJid = model.jid;
                    view.model.rootNode = model.node;
                }
            }
        }
    }
}

