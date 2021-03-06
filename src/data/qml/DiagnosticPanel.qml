/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import QtQuick.Controls 1.0
import QtWebKit 3.0
import wiLink 2.5
import '.' as Custom

Panel {
    id: panel

    property bool busy: Qt.isQtObject(client) && client.diagnosticManager.running
    property QtObject client

    Clipboard {
        id: clipboard
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconStyle: 'icon-info-sign'
        title: qsTr('Diagnostics')
        toolBar: Custom.ToolBar {
            Custom.ToolButton {
                iconStyle: 'icon-copy'
                enabled: !panel.busy
                text: qsTr('Copy')

                onClicked: clipboard.copy(panel.client.diagnosticManager.html)
            }
            Custom.ToolButton {
                iconStyle: 'icon-refresh'
                enabled: !panel.busy
                text: qsTr('Refresh')

                onClicked: panel.client.diagnosticManager.refresh();
            }
        }
    }

    ScrollView {
        id: flickable

        anchors.margins: appStyle.margin.normal
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: parent.bottom

        WebView {
            id: diagnostic

            anchors.fill: parent

            Connections {
                ignoreUnknownSignals: true
                target: panel.client.diagnosticManager

                onHtmlChanged: {
                    diagnostic.loadHtml(panel.client.diagnosticManager.html);
                }

                onJsonChanged: {
                    for (var i = 0; i < accountModel.count; ++i) {
                        var account = accountModel.get(i);
                        if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                            var xhr = new XMLHttpRequest();
                            xhr.open('POST', 'https://support.wifirst.net/diagnostics', true, account.username, account.password);
                            xhr.setRequestHeader("Content-Type", "application/json");
                            xhr.send();
                            break;
                        }
                    }
                }
            }
        }
    }

    Spinner {
        anchors.centerIn: flickable
        busy: panel.busy
    }

    Component.onCompleted: {
        panel.client = accountModel.clientForJid('wifirst.net');
        panel.client.diagnosticManager.refresh()
    }
}

