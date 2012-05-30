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
import QtWebKit 1.0
import wiLink 2.0

Panel {
    id: panel

    property string iconSource: 'image://icon/web'
    property string loadScript: ''
    property alias title: webView.title
    property alias url: webView.url
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

        ToolBar {
            id: toolBar

            anchors.margins: appStyle.margin.normal
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left

            ToolButton {
                enabled: webView.back.enabled
                iconSource: 'image://icon/back'
                width: iconSize

                onClicked: webView.back.triggered()
            }

            ToolButton {
                enabled: webView.forward.enabled
                iconSource: 'image://icon/forward'
                width: iconSize

                onClicked: webView.forward.triggered()
            }

            ToolButton {
                enabled: webView.url != '' && webView.reload.enabled
                iconSource: 'image://icon/refresh'
                visible: !webView.stop.enabled
                width: iconSize

                onClicked: webView.reload.triggered()
            }

            ToolButton {
                iconSource: 'image://icon/stop'
                visible: webView.stop.enabled
                width: iconSize

                onClicked: webView.stop.triggered()
            }
        }

        InputBar {
            id: urlInput

            anchors.margins: appStyle.margin.normal
            anchors.top: parent.top
            anchors.left: toolBar.right
            anchors.right: parent.right
            focus: true
            text: webView.url

            onAccepted: {
                var url = urlInput.text.trim();
                if (url.match(/^(ftp|http|https):\/\//)) {
                    webView.url = urlInput.text;
                } else if ((url.search(/\s/) == -1) && url.match(/\.[a-z]{2,}$/)) {
                    webView.url = 'http://' + url;
                } else {
                    webView.url = 'http://www.google.com/search?q=' + encodeURIComponent(url);
                }
                webView.forceActiveFocus()
            }
        }
    }

    Item {
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Flickable {
            id: webFlickable

            anchors.top: parent.top
            anchors.bottom: horizontalScrollBar.top
            anchors.left: parent.left
            anchors.right: verticalScrollBar.left
            contentWidth: webView.width
            contentHeight: webView.height

            WebView {
                id: webView

                preferredHeight: webFlickable.height
                preferredWidth: webFlickable.width

                onAlert: {
                    console.log("Alert: " + message);
                }

                onLoadFailed: {
                    webView.html = '<html><head><title>Error loading page</title></head><body>There was an error loading the page.</body></html>';
                }

                onLoadFinished: {
                    if (panel.loadScript) {
                        var js = panel.loadScript;
                        panel.loadScript = '';
                        webView.evaluateJavaScript(js);
                    }
                }
            }
        }

        WheelArea {
            anchors.fill: webFlickable

            onHorizontalWheel: horizontalScrollBar.moveBy(-delta)
            onVerticalWheel: verticalScrollBar.moveBy(-delta)
        }

        ScrollBar {
            id: verticalScrollBar

            anchors.top: parent.top
            anchors.bottom: horizontalScrollBar.top
            anchors.right: parent.right
            flickableItem: webFlickable
        }

        ScrollBar {
            id: horizontalScrollBar

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: verticalScrollBar.left
            flickableItem: webFlickable
            orientation: Qt.Horizontal
        }

        Rectangle {
            color: '#dfdfdf'
            anchors.left: horizontalScrollBar.right
            anchors.right: parent.right
            anchors.top: verticalScrollBar.bottom
            anchors.bottom: parent.bottom
        }
    }
}
