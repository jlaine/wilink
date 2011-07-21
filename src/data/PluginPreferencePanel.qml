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

    function save() { }

    color: 'transparent'

    /* hard-coded models */
    ListModel {
        id: pluginModel
        ListElement { source: 'PlayerPlugin.qml'; installed: true; }
        ListElement { source: 'RssPlugin.qml'; installed: false; }
    }
    /* end: hard-coded models */

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
                model: pluginModel
                spacing: appStyle.spacing.vertical
            }

            ScrollBar {
                id: scrollBar

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                flickableItem: view
            }
        }
    }

    Component {
        id: pluginDelegate

        Item {
            id: item

            property alias plugin: loader.item

            height: 32
            width: parent.width - 1
            opacity: model.installed ? 1 : 0.5

            Loader {
                id: loader
                source: model.source
            }

            Rectangle {
                anchors.fill: parent
                color: '#999fb7dd'
                border.color: '#9fb7dd'
                border.width: 1
                radius: 5
            }

            Image {
                id: image
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                source: plugin.imageSource !== '' ? plugin.imageSource : 'plugin.png'
            }

            Column {
                anchors.left: image.right
                anchors.right: checkbox.left
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    elide: Text.ElideRight
                    font.bold: true
                    text: plugin.name
                    width: parent.width
                }

                Text {
                    id: textLabel

                    elide: Text.ElideRight
                    text: view.currentIndex == model.index ? plugin.description : plugin.summary
                    width: parent.width
                }
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
                anchors.verticalCenter: parent.verticalCenter
                checked: model.installed
                width: 16
                height: 16

                onClicked: model.installed = !model.installed
            }

            states: State {
                name: 'details'
                PropertyChanges {
                    target: textLabel
                    text: plugin.description
                }
            }
        }
    }
}

