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
import wiLink 2.4
import 'scripts/utils.js' as Utils

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
        
        /*
         * Sparrow 
         */
//        gradient: Gradient {
//            GradientStop {position: 0.05; color: '#262b30' }
//            GradientStop {position: 0.06; color: '#262b30'}
//            GradientStop {position: 1.0; color: '#262b30'}
//        }
        
        /*
         * Webapps
         */
        gradient: Gradient {
            GradientStop {position: 0.05; color: '#566c8a' }
            GradientStop {position: 0.06; color: '#566c8a'}
            GradientStop {position: 1.0; color: '#677e9f'}
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
                iconStyle: model.iconStyle
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
            iconStyle: 'icon-cogs'
            panelSource: 'PreferenceDialog.qml'
            text: qsTr('Preferences')
            visible: true
            onClicked: {
                dialogSwapper.showPanel(panelSource);
            }
        }

        DockButton {
            id: quitButton
            iconStyle: 'icon-remove'
            text: qsTr('Quit')
            visible: appStyle.isMobile
            onClicked: {
                Qt.quit();
            }
        }
    }

    Rectangle {
        id: rightBorder

        anchors.left: parent.right
        width: 1
        height: parent.height
        color: 'white'
        opacity: 0.0
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_K) {
            window.reload();
            return;
        }

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

