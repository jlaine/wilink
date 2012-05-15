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
            iconSource: 'image://icon/add'
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
        property int accountStep: 0

        function doLogin() {
            accountIndex += 1;
            accountStep = 0;
            var account = accountModel.get(accountIndex);
            if (!account)
                return;

            console.log("Examining account: " + account.realm);
            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                loginView.url = 'https://www.wifirst.net/';
            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                loginView.url = 'https://accounts.google.com/ServiceLogin?service=mail';
            } else {
                doLogin();
            }
        }

        onLoadFinished: {
            var account = accountModel.get(accountIndex);
            if (!account)
                return;

            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                var js = "var xhr = new XMLHttpRequest();";
                js += "xhr.open('POST', 'https://www.wifirst.net/sessions', false);";
                js += "xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');";
                js += "xhr.send('login=" + account.username + "&password=" + account.password + "');";
                js += "xhr.status";

                var status = loginView.evaluateJavaScript(js);
                if (status == "200") {
                    console.log("Logged into wifirst account: " + account.username);
                    tabSwapper.addPanel('WebTab.qml', {'url': 'https://www.wifirst.net/'}, true)
                } else {
                    console.log("Could not log into wifirst account: " + status);
                }
                doLogin();
            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                if (accountStep) {
                    console.log("Logged into google account: " + account.username);
                    tabSwapper.addPanel('WebTab.qml', {'url': 'https://mail.google.com/mail/'}, true)
                    doLogin();
                } else {
                    accountStep += 1;
                    var js = "var f = document.getElementById('gaia_loginform');\n";
                    js += "f.Email.value = '" + account.username + "';\n";
                    js += "f.Passwd.value = '" + account.password + "';\n";
                    js += "f.submit();";
                    loginView.evaluateJavaScript(js);
                }
            }
        }
    }

    Component.onCompleted: loginView.doLogin()

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
