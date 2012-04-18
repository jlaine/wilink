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

FocusScope {
    id: root

    AccountModel {
        id: accountModel

        onModelReset: {
            loader.source = '';
            if (accountModel.count)
                loader.source = 'MainWindow.qml';
        }
    }

    Style {
        id: appStyle
        isMobile: application.isMobile
    }

    Rectangle {
        anchors.fill: parent
        color: 'pink'
    }

    Loader {
        id: loader

        anchors.fill: parent
    }

    SetupDialog {
        anchors.centerIn: parent
        opacity: accountModel.count ? 0.0 : 1.0;
    }

    Component.onCompleted: {
        window.minimumWidth = 360;
        window.minimumHeight = 360;
        window.fullScreen = application.isMobile && !application.isAndroid;
        window.showAndRaise();

        application.showWindows.connect(function() {
            window.showAndRaise();
        });

        loader.source = accountModel.count ? 'MainWindow.qml' : '';
    }
}
