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

import wiLink 2.4
import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Window 2.2

ApplicationWindow {
    id: window

    signal showAbout
    signal showHelp
    signal showPreferences

    minimumWidth: 360
    minimumHeight: 360
    menuBar: MenuBar {
        Menu {
            title: qsTr('&File')

            MenuItem {
                text: qsTr('&Preferences')
                onTriggered: window.showPreferences()
            }

            MenuItem {
                text: qsTr('&Quit')
                shortcut: 'Ctrl+Q'
                onTriggered: Qt.quit()
            }
        }
        Menu {
            title: qsTr('&Help')

            MenuItem {
                text: qsTr('%1 FAQ').replace('%1', Qt.application.name)
                shortcut: 'F1' // FIXME: for mac CTRL+?
                onTriggered: window.showHelp()
            }

            MenuItem {
                text: qsTr('About %1').replace('%1', Qt.application.name)
                onTriggered: window.showAbout()
            }
        }
    }

    function reload() {
        uiLoader.source = '';
        uiLoader.source = 'Main.qml';
    }

    function showAndRaise() {
        show();
        raise();
    }

    TranslationLoader {
        Component.onCompleted: {
            if (localeName == 'fr')
                source = 'i18n/' + localeName + '.qm';
            else
                uiLoader.source = 'Main.qml';
        }

        onStatusChanged: {
            if (status == TranslationLoader.Ready || status == TranslationLoader.Error) {
                uiLoader.source = 'Main.qml';
            }
        }
    }

    Loader {
        id: uiLoader

        anchors.fill: parent
        focus: true
    }

    /** Once the QML file finishes loading, show the application window.
     */
    Component.onCompleted: {
        window.width = Math.min(1280, Screen.desktopAvailableWidth - 100);
        window.height = Math.min(960, Screen.desktopAvailableHeight - 100);
        window.x = (Screen.desktopAvailableWidth - window.width) / 2;
        window.y = (Screen.desktopAvailableHeight - window.height) / 2;
        window.show();
    }
}
