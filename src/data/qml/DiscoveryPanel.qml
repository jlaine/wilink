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

    property string accountJid

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'image://icon/peer'
        title: qsTr('Service discovery')
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'image://icon/back'
                text: qsTr('Go back')
                enabled: crumbBar.model.count > 1

                onClicked: crumbBar.pop()
            }

            ToolButton {
                iconSource: 'image://icon/refresh'
                text: qsTr('Refresh')

                onClicked: {
                    discoView.model.refresh();
                }
            }

            ToolButton {
                iconSource: 'image://icon/close'
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

        onLocationChanged: {
            discoView.model.rootJid = location.jid;
            discoView.model.rootNode = location.node;
        }
        z: 1
    }

    ScrollView {
        id: discoView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: parent.bottom

        model: DiscoveryModel {
            details: true
        }

        delegate: Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: appStyle.icon.normalSize + 2

            Image {
                id: image

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                source: 'image://icon/peer'
            }

            Label {
                anchors.left: image.right
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: '<b>' + model.name + '</b><br/>' + model.jid + (model.node ? ' (' + model.node + ')' : '')
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    crumbBar.push({'name': model.node ? model.node : model.jid, 'jid': model.jid, 'node': model.node});
                }
            }
        }
    }

    onAccountJidChanged: {
        var domain = Utils.jidToDomain(panel.accountJid);
        var client = accountModel.clientForJid(panel.accountJid);
        discoView.model.manager = client.discoveryManager;
        crumbBar.push({'name': domain, 'jid': domain, 'node': ''})
    }
}

