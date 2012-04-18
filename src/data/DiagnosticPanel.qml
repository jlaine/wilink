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

Panel {
    id: panel

    property string accountJid
    property bool busy: Qt.isQtObject(client) && client.diagnosticManager.running
    property QtObject client

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'diagnostics.png'
        title: qsTr('Diagnostics')
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'copy.png'
                enabled: !panel.busy
                text: qsTr('Copy')

                onClicked: appClipboard.copy(diagnostic.text)
            }
            ToolButton {
                iconSource: 'refresh.png'
                enabled: !panel.busy
                text: qsTr('Refresh')

                onClicked: panel.client.diagnosticManager.refresh();
            }
        }
    }

    Flickable {
        id: flickable

        anchors.margins: appStyle.margin.normal
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        contentHeight: diagnostic.height

        Label {
            id: diagnostic

            text: Qt.isQtObject(panel.client) ? panel.client.diagnosticManager.html : ''
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: flickable
    }

    Spinner {
        anchors.centerIn: flickable
        busy: panel.busy
    }

    onAccountJidChanged: {
        panel.client = accountModel.clientForJid(panel.accountJid);
        panel.client.diagnosticManager.refresh()
    }

    Keys.forwardTo: scrollBar
}

