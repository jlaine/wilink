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
import 'utils.js' as Utils

Rectangle {
    id: dock
    width: appStyle.icon.normalSize + 12

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
        spacing: appStyle.isMobile ? 10 : 5

        Repeater {
            id: repeater

            model: ListModel {
                id: listModel

                function add(properties) {
                    function priority(props) {
                        if (props.priority == undefined)
                            return 0;
                        else
                            return props.priority;
                    }

                    var row = count;
                    for (var i = 0; i < count; i++) {
                        if (priority(get(i)) < priority(properties)) {
                            row = i;
                            break;
                        }
                    }
                    insert(row, properties);
                }

                function removePanel(source) {
                    for (var i = repeater.model.count - 1; i >= 0; i--) {
                        if (repeater.model.get(i).panelSource == source) {
                            repeater.model.remove(i);
                            var panel = swapper.findPanel(source);
                            if (panel) panel.close();
                        }
                    }
                }
            }

            delegate: DockButton {
                id: button

                iconSource: model.iconSource
                iconPress: model.iconPress
                notified: model.notified == true
                panelProperties: model.panelProperties
                panelSource: model.panelSource
                shortcut: model.shortcut ? model.shortcut : 0
                text: model.text
                visible: model.visible

                onClicked: {
                    visible = true;
                    var model = listModel.get(index);
                    var panel = swapper.findPanel(model.panelSource, model.panelProperties);
                    if (panel && panel == swapper.currentItem) {
                        panel.dockClicked();
                    } else {
                        swapper.showPanel(model.panelSource, model.panelProperties);
                    }
                }
            }
        }

        DockButton {
            id: preferenceButton

            iconSource: 'image://icon/dock-options'
            iconPress: 'image://icon/options'
            panelSource: 'PreferenceDialog.qml'
            text: qsTr('Preferences')
            visible: true
            onClicked: {
                dialogSwapper.showPanel(panelSource);
            }
        }

        DockButton {
            id: quitButton
            iconSource: 'image://icon/dock-close'
            iconPress: 'image://icon/close'
            text: qsTr('Quit')
            visible: appStyle.isMobile
            onClicked: {
                application.quit();
            }
        }
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

