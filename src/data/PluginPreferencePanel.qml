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

Panel {
    id: panel

    function save() {
        for (var i = 0; i < view.model.count; i++) {
            var plugin = view.model.get(i);
            if (plugin.selected)
                appPlugins.loadPlugin(plugin.source);
            else
                appPlugins.unloadPlugin(plugin.source);
        }
        appPlugins.storePreferences();
    }

    color: 'transparent'

    GroupBox {
        id: list

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        title: qsTr('Plugins')

        Item {
            anchors.fill: parent.contents

            ListView {
                id: view

                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: scrollBar.left
                clip: true
                delegate: pluginDelegate
                highlight: Highlight {
                    width: view.width
                }
                highlightMoveDuration: appStyle.highlightMoveDuration
                model: ListModel {}
                spacing: appStyle.spacing.vertical

                Component.onCompleted: {
                    for (var i = 0; i < appPlugins.model.count; i++) {
                        var plugin = appPlugins.model.get(i);
                        view.model.append({'source': plugin.source,
                            'selected': plugin.loaded != undefined});
                    }
                }
            }

            ScrollBar {
                id: scrollBar

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                clip: true
                flickableItem: view
            }
        }
    }

    Component {
        id: pluginDelegate

        Item {
            id: item

            property alias plugin: loader.item

            height: appStyle.icon.normalSize
            width: parent.width - 1

            Loader {
                id: loader
                source: model.source
            }

            Image {
                id: image
                anchors.top: parent.top
                anchors.left: parent.left
                source: Qt.isQtObject(plugin) ? plugin.imageSource : 'plugin.png'
            }

            Text {
                id: nameLabel

                anchors.verticalCenter: image.verticalCenter
                anchors.left: image.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: checkbox.left
                anchors.rightMargin: appStyle.spacing.horizontal
                elide: Text.ElideRight
                font.bold: true
                text: Qt.isQtObject(plugin) ? plugin.name : ''
            }

            Text {
                id: descriptionLabel

                anchors.top: image.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.left: parent.left
                anchors.right: parent.right
                visible: false
                wrapMode: Text.WordWrap
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    view.currentIndex = model.index;
                 }
            }

            CheckBox {
                id: checkbox

                anchors.right: parent.right
                anchors.verticalCenter: image.verticalCenter
                checked: model.selected
                width: 16
                height: 16

                onClicked: {
                    view.model.setProperty(model.index, 'selected', !model.selected);
                }
            }

            states: State {
                name: 'details'
                when: view.currentIndex == model.index

                PropertyChanges {
                    target: descriptionLabel
                    text: Qt.isQtObject(plugin) ? plugin.description : ''
                    visible: true
                }

                PropertyChanges {
                    target: item
                    height: appStyle.icon.normalSize + 2*appStyle.spacing.vertical + descriptionLabel.height
                }
            }
        }
    }
}

