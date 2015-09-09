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
import QtWebKit.experimental 1.0
import wiLink 2.4
import '.' as Custom
import 'scripts/utils.js' as Utils

Panel {
    id: panel

    property string iconSource: 'images/web.png'
    property string loadScript: ''
    property alias title: webView.title
    property alias url: webView.url
    property bool urlBar: true
    clip: true

    Item {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: urlInput.height + 2 * appStyle.margin.normal + 1
        z: 2

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: -1
            anchors.leftMargin: -1
            border.color: '#acb4c4'
            color: '#dfdfdf'
        }

        Custom.ToolBar {
            id: toolBar

            anchors.margins: appStyle.margin.normal
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left

            ToolButton {
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.bottom: parent.bottom
                text: Utils.iconText('icon-home')

                onClicked: {
                    webView.url = appSettings.wifirstBaseUrl;
                }
            }

            ToolButton {
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.bottom: parent.bottom
                enabled: webView.canGoBack
                text: Utils.iconText('icon-chevron-left')

                onClicked: webView.goBack()
            }

            ToolButton {
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.bottom: parent.bottom
                enabled: webView.canGoForward
                text: Utils.iconText('icon-chevron-right')

                onClicked: webView.goForward()
            }

            ToolButton {
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.bottom: parent.bottom
                enabled: webView.url != ''
                text: Utils.iconText('icon-refresh')
                visible: !webView.stop.enabled

                onClicked: webView.reload()
            }

            ToolButton {
                anchors.top: parent.top
                anchors.topMargin: 2
                anchors.bottom: parent.bottom
                text: Utils.iconText('icon-stop')
                visible: webView.loading

                onClicked: webView.stop()
            }
        }

        Custom.InputBar {
            id: urlInput

            anchors.margins: appStyle.margin.normal
            anchors.top: parent.top
            anchors.left: toolBar.right
            anchors.right: parent.right
            focus: webView.url == ''
            text: webView.url
            visible: urlBar

            onAccepted: {
                var url = urlInput.text.trim();
                if (url.match(/^(file|ftp|http|https):\/\//)) {
                    webView.url = urlInput.text;
                } else if ((url.search(/\s/) == -1) && url.match(/\.[a-z\.\/]{2,}$/)) {
                    webView.url = 'http://' + url;
                } else {
                    webView.url = 'http://www.google.com/search?q=' + encodeURIComponent(url);
                }
                webView.forceActiveFocus()
            }
        }
    }

    FocusScope {
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        focus: webView.url != ''

        ScrollView {
            anchors.fill: parent

            WebView {
                id: webView

                anchors.fill: parent
                experimental.preferences.navigatorQtObjectEnabled: true

                onLoadingChanged: {
                    if (loadRequest.status == WebView.LoadFailedStatus) {
                        webView.loadHtml('<html><head><title>Error loading page</title></head><body>There was an error loading the page.<br/>' + url + '</body></html>', '', url);
                    } else if (loadRequest.status == WebView.LoadSucceededStatus) {
                        if (panel.loadScript) {
                            // automatic login
                            var js = panel.loadScript;
                            panel.loadScript = '';
                            webView.experimental.evaluateJavaScript(js);
                        } else {
                            // catch links to wilink:// urls
                            webView.experimental.evaluateJavaScript('$("a[href^=\\"wilink:\\"]").live("click", function() { navigator.qt.postMessage(this.href); return false; })');
                        }
                    }
                }

                experimental.onMessageReceived: {
                    Qt.openUrlExternally(message.data);
                }
            }
        }

        Keys.onPressed: {
            if (((event.modifiers & Qt.ControlModifier) && event.key == Qt.Key_R)
             || ((event.modifiers == Qt.NoModifier) && event.key == Qt.Key_F5)) {
                webView.reload()
            }
        }
    }
}
