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
    width: 44

    Rectangle {
        id: dockBackground

        height: parent.width
        width: parent.height
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width
        gradient: Gradient {
            GradientStop {position: 0.1; color: '#597fbe' }
            GradientStop {position: 0.11; color: '#7495ca'}
            GradientStop {position: 1.0; color: '#9fb7dd'}
        }

        transform: Rotation {
            angle: 90
            origin.x: 0
            origin.y: dockBackground.height
        }
    }

    Column {
        id: control
        anchors.leftMargin: 4
        anchors.topMargin: 4
        anchors.top: parent.top
        anchors.left: parent.left
        spacing: 5
/*
        DockButton {
            iconSource: 'wiLink-white.png'
            iconPress: 'wiLink.png'
            text: qsTr('My account')
            onClicked: {
                Qt.openUrlExternally('https://www.wifirst.net/w/profile')
            }
        }
*/
        DockButton {
            iconSource: 'dock-chat.png'
            iconPress: 'chat.png'
            text: qsTr('Chat')
            onClicked: {
                var panel = swapper.findPanel('ChatPanel.qml');
                if (panel == swapper.currentItem) {
                    if (panel.state == 'noroster')
                        panel.state = '';
                    else
                        panel.state = 'noroster';
                } else {
                    swapper.setCurrentItem(panel);
                }
            }
        }

        DockButton {
            iconSource: 'start.png'
            shortcut: Qt.ControlModifier + Qt.Key_M
            text: qsTr('Media')
            visible: false

            onClicked: {
                visible = true;
                swapper.showPanel('PlayerPanel.qml');
            }
        }

        DockButton {
            id: phoneButton

            iconSource: 'dock-phone.png'
            iconPress: 'phone.png'
            shortcut: Qt.ControlModifier + Qt.Key_T
            text: qsTr('Phone')
            visible: Utils.jidToDomain(window.client.jid) == 'wifirst.net'

            onClicked: {
                if (visible) {
                    swapper.showPanel('PhonePanel.qml');
                }
            }

            Component.onCompleted: {
                if (visible) {
                    swapper.addPanel('PhonePanel.qml');
                }
            }
        }

        DockButton {
            id: shareButton

            iconSource: 'dock-share.png'
            iconPress: 'share.png'
            shortcut: Qt.ControlModifier + Qt.Key_S
            text: qsTr('Shares')
            visible: window.client.shareServer != ''

            onClicked: {
                if (visible) {
                    swapper.showPanel('SharePanel.qml');
                }
            }

            onVisibleChanged: {
                if (visible) {
                    swapper.addPanel('SharePanel.qml');
                }
            }
        }

        DockButton {
            property string domain: Utils.jidToDomain(window.client.jid)

            iconSource: 'dock-photo.png'
            shortcut: Qt.ControlModifier + Qt.Key_P
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
            id: preferenceButton

            iconSource: 'dock-options.png'
            text: qsTr('Preferences')
            visible: true
            onClicked: {
                dialogLoader.source = 'PreferenceDialog.qml';
                dialogLoader.show();
            }

        }

        DockButton {
            iconSource: 'dock-diagnostics.png'
            shortcut: Qt.ControlModifier + Qt.Key_I
            text: qsTr('Diagnostics')
            onClicked: swapper.showPanel('DiagnosticPanel.qml')
        }

        DockButton {
            iconSource: 'options.png'
            shortcut: Qt.ControlModifier + Qt.Key_L
            text: qsTr('Debugging')
            visible: false

            onClicked: {
                visible = true;
                swapper.showPanel('LogPanel.qml');
            }
        }

        DockButton {
            iconSource: 'peer.png'
            shortcut: Qt.ControlModifier + Qt.Key_B
            text: qsTr('Discovery')
            visible: false

            onClicked: {
                visible = true;
                swapper.showPanel('DiscoveryPanel.qml');
            }
        }
    }

    Connections {
        target: window

        onShowAbout: {
            dialogLoader.source = 'AboutDialog.qml';
            dialogLoader.show();
        }

        onShowPreferences: preferenceButton.clicked()
    }

    Keys.onPressed: {
        var val = event.modifiers + event.key;
        for (var i = 0; i < control.children.length; i++) {
            var button = control.children[i];
            if (val == button.shortcut) {
                button.clicked();
                break;
            }
        }
    }
}

