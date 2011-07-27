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
import wiLink 2.0
import 'utils.js' as Utils

Rectangle {
    id: dock
    width: 44

    property alias model: repeater.model

    Rectangle {
        id: dockBackground

        height: parent.width
        width: parent.height
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: -parent.width
        gradient: Gradient {
            GradientStop {position: 0.05; color: '#597fbe' }
            GradientStop {position: 0.06; color: '#7495ca'}
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

        DockButton {
            iconSource: 'dock-chat.png'
            iconPress: 'chat.png'
            notified: root.pendingMessages != 0
            panelSource: 'ChatPanel.qml'
            text: qsTr('Chat')
            onClicked: {
                var panel = swapper.findPanel(panelSource);
                if (panel == swapper.currentItem) {
                    if (panel.state == 'no-sidebar')
                        panel.state = '';
                    else
                        panel.state = 'no-sidebar';
                } else {
                    swapper.setCurrentItem(panel);
                }
            }
        }

        Repeater {
            id: repeater

            model: ListModel {
                id: listModel

                function add(properties) {
                    append(properties);
                }

                function removePanel(source) {
                    for (var i = 0; i < repeater.model.count; i++) {
                        if (repeater.model.get(i).panelSource == source) {
                            swapper.removePanel(source);
                            repeater.model.remove(i);
                            break;
                        }
                    }
                }
            }

            delegate: DockButton {
                id: button

                iconSource: model.iconSource
                iconPress: model.iconPress
                panelSource: model.panelSource
                shortcut: model.shortcut
                text: model.text
                visible: model.visible

                onClicked: {
                    visible = true;
                    var model = listModel.get(index);
                    var panel = swapper.findPanel(model.panelSource, model.panelProperties);
                    if (panel == swapper.currentItem) {
                        if (panel.state == 'no-sidebar')
                            panel.state = '';
                        else
                            panel.state = 'no-sidebar';
                    } else {
                        swapper.showPanel(model.panelSource, model.panelProperties);
                    }
                }
            }
        }

        DockButton {
            id: preferenceButton

            iconSource: 'dock-options.png'
            iconPress: 'options.png'
            panelSource: 'PreferenceDialog.qml'
            text: qsTr('Preferences')
            visible: true
            onClicked: {
                dialogSwapper.showPanel(panelSource);
            }
        }
    }

    Connections {
        target: application.updater

        onStateChanged: {
            // when an update is ready to install, prompt user
            if (application.updater.state == Updater.PromptState) {
                dialogSwapper.showPanel('AboutDialog.qml');
                window.alert();
            }
        }
    }

    Connections {
        target: window

        onShowAbout: {
            dialogSwapper.showPanel('AboutDialog.qml');
        }

        onShowHelp: Qt.openUrlExternally('https://www.wifirst.net/wilink/faq')

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

