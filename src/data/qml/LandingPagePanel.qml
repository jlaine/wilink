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

Panel {
    id: panel

    property string webUsername
    property string webPassword
    property string cookie

    PanelSwapper {
        id: landingSwapper

        anchors.fill: parent
        focus: true
        visible: width > 0
    }

    Component.onCompleted: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                panel.webUsername = account.username;
                panel.webPassword = account.password;
                break;
             }
        }
        var js = "var f = document.getElementsByTagName('form')[0];\n";
        js += "f.login.value = '" + panel.webUsername + "';\n";
        js += "f.password.value = '" + panel.webPassword + "';\n";
        js += "f.submit();";

        /**
         * TODO
         *
         * At startup, HomePage should be displayed
         *
         * onAuthenticationFailed with Facebook, a web tab with https://apps.wifirst.net/wilink/embedded?invalid_facebook_permission=true
         * should be loaded
         */

        // TODO: find a way to load this in background so that authentication can be performed and cookie obtained
        // landingSwapper.addPanel('WebTab.qml', {url: 'https://selfcare.wifirst.net', urlBar: false, loadScript: js}, true);

        // TODO
        // load the following at startup
        landingSwapper.showPanel('HomePage.qml', {panelSwapper: landingSwapper});

        // TODO
        // hook the following with the onAuthenticationFailed for the facebook client
        // landingSwapper.showPanel('WebTab.qml', {url: appSettings.wifirstBaseUrl + '/wilink/embedded/?invalid_facebook_permission=true', urlBar: false, loadScript: js}, true)
    }
}
