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

    property variant url

    TabView {
        id: tabView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: appStyle.icon.smallSize
        panelSwapper: tabSwapper

        footer: ToolButton {
            id: addButton

            anchors.verticalCenter: parent.verticalCenter
            iconSize: appStyle.icon.tinySize
            iconSource: 'add.png'
            width: iconSize

            onClicked: tabSwapper.addPanel('WebTab.qml', {}, true);
        }
    }

    PanelSwapper {
        id: tabSwapper

        anchors.top: tabView.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        focus: true
    }

    WebView {
        id: loginView
        opacity: 0

        property int accountIndex: -1

        function doLogin() {
            accountIndex += 1;
            var account = accountHelper.get(accountIndex);
            if (!account)
                return;

            console.log("Exmining account: " + account.type);
            if (account.type == 'wifirst') {
                loginView.url = 'https://www.wifirst.net/';
            } else {
                doLogin();
            }
        }

        function doXhr(method, url, data) {
            if (data)
                data = "'" + data + "'";
            else
                data = 'null';

            console.log('Request ' + method + ' ' + url);
            var js = "var xhr = new XMLHttpRequest();";
            js += "xhr.open('" + method + "', '" + url + "', false);";
            if (method == 'POST')
                js += "xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
            js += "xhr.send(" + data + ");";
            js += "xhr.status";
            return loginView.evaluateJavaScript(js);
        }

        ListHelper {
            id: accountHelper
            model: AccountModel {}

            Component.onCompleted: loginView.doLogin()
        }

        onLoadFinished: {
            var account = accountHelper.get(accountIndex);
            if (account.type == 'wifirst') {
                var data = "login=" + account.username + "&password=" + account.password;
                var status = doXhr('POST', 'https://www.wifirst.net/sessions', data);
                if (status == "200") {
                    console.log("Logged into wifirst account: " + account.username);
                    tabSwapper.addPanel('WebTab.qml', {'url': 'https://www.wifirst.net/'}, true)
                } else {
                    console.log("Could not log into wifirst account: " + status);
                }
            }
            doLogin();
        }
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_T) {
            tabSwapper.addPanel('WebTab.qml', {}, true);
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Left) {
            tabSwapper.decrementCurrentIndex();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Right) {
            tabSwapper.incrementCurrentIndex();
        }
    }
}
