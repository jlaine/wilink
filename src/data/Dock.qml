/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

import QtQuick 1.0
import 'utils.js' as Utils

Rectangle {
    id: dock
    width: 32

    Rectangle {
        id: dockBackground

        height: 32
        width: parent.height
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -32
        border.color: '#597fbe'
        border.width: 1

        gradient: Gradient {
            GradientStop { position:0.0; color: '#9fb7dd' }
            GradientStop { position:0.5; color: '#7495ca' }
            GradientStop { position:1.0; color: '#9fb7dd' }
        }

        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: 32
        }
    }

    Column {
        id: control
        anchors.margins:  2
        anchors.top: parent.top
        anchors.left: parent.left
        spacing: 5

        DockButton {
            iconSource: 'chat.png'
            text: qsTr('Chat')
            onClicked: swapper.showPanel('ChatPanel.qml')
        }

        DockButton {
            iconSource: 'start.png'
            shortcut: Qt.ControlModifier + Qt.Key_M
            text: qsTr('Media')
            visible: false
            onClicked: swapper.showPanel('PlayerPanel.qml')
        }

        DockButton {
            id: phoneButton

            iconSource: 'phone.png'
            text: qsTr('Phone')
            visible: Utils.jidToDomain(window.client.jid) == 'wifirst.net'
            onClicked: swapper.showPanel('PhonePanel.qml')
            Component.onCompleted: {
                if (visible) {
                    swapper.addPanel('PhonePanel.qml');
                }
            }
        }

        DockButton {
            id: shareButton

            iconSource: 'share.png'
            text: qsTr('Shares')
            visible: window.client.shareServer != ''
            onClicked: swapper.showPanel('SharePanel.qml')
            onVisibleChanged: {
                if (shareButton.visible) {
                    swapper.addPanel('SharePanel.qml');
                }
            }
        }

        DockButton {
            property string domain: Utils.jidToDomain(window.client.jid)

            iconSource: 'photos.png'
            text: qsTr('Photos')
            visible: domain == 'wifirst.net' || domain == 'gmail.com'
            onClicked: {
                if (domain == 'wifirst.net')
                    swapper.showPanel('PhotoPanel.qml', {'url': 'wifirst://www.wifirst.net/w'});
                else if (domain == 'gmail.com')
                    swapper.showPanel('PhotoPanel.qml', {'url': 'picasa://default'});
            }
        }

        DockButton {
            iconSource: 'options.png'
            text: qsTr('Preferences')
            onClicked: {
                dialog.source = 'PreferenceDialog.qml';
                dialog.item.show();
            }
        }

        DockButton {
            iconSource: 'diagnostics.png'
            shortcut: Qt.ControlModifier + Qt.Key_I
            text: qsTr('Diagnostics')
            onClicked: swapper.showPanel('DiagnosticPanel.qml')
        }

        DockButton {
            iconSource: 'options.png'
            shortcut: Qt.ControlModifier + Qt.Key_L
            text: qsTr('Debugging')
            visible: false
            onClicked: swapper.showPanel('LogPanel.qml')
        }

        DockButton {
            iconSource: 'peer.png'
            shortcut: Qt.ControlModifier + Qt.Key_B
            text: qsTr('Discovery')
            visible: false
            onClicked: swapper.showPanel('DiscoveryPanel.qml')
        }
    }

    Keys.onPressed: {
        var val = event.modifiers + event.key;
        for (var i = 0; i < control.children.length; i++) {
            var button = control.children[i];
            if (val == button.shortcut) {
                button.visible = true;
                button.clicked();
                break;
            }
        }
    }
}

