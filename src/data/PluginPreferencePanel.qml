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
        ListElement { installed: false; name: 'Pong'; summary: 'Basic pong game'; description: 'This is a pong game with 4 basic levels. Use left and right arrow keys to move.'; image: 'favorite-active.png'; }
        ListElement { installed: true; name: 'Music'; summary: 'Play sounds and manage albums with wiLink !'; description: 'This plugin add a music manager to wiLink.'; image: 'start.png'; }
        ListElement { installed: false; name: 'Go! Go fish !'; summary: 'Free a lovely fish in wiLink\'s window'; description: 'This application display a pink fish in wiLink\'s window. Yes this is useless, but it\'s really fun !'; image: 'tip.png'; }
        ListElement { installed: false; name: 'H4ck3r'; summary: 'Show a fake hacking console'; description: 'This is a fun fake console, with abstract commands running inside.'; image: ''; }
        ListElement { installed: true; name: 'Rss reader'; summary: 'Follow your friends over rss'; description: 'This plugin add a rss reader to wiLink.'; image: 'peer.png'; }
        ListElement { installed: false; name: 'Storm'; summary: 'Destroy ! Destroy !'; description: 'Display a storm inside wiLink\'s window'; image: 'diagnostics.png'; }
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
            height: 32
            width: parent.width - 1
            opacity: model.installed ? 1 : 0.5

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
                source: model.image !== '' ? model.image : 'plugin.png'
            }

            Column {
                anchors.left: image.right
                anchors.right: checkbox.left
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    elide: Text.ElideRight
                    font.bold: true
                    text: model.name
                    width: parent.width
                }

                Text {
                    elide: Text.ElideRight
                    text: model.summary
                    width: parent.width
                }
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    details.model = model
                    panel.state = 'detailed'
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
        }
    }

    Item {
        id: details

        property QtObject model: ListModel { }

        anchors.fill: parent
        opacity: 0

        Row {
            id: header

            anchors.right: parent.right
            anchors.top: parent.top
            spacing: appStyle.spacing.horizontal

            Button {
                anchors.top: parent.top
                iconSize: appStyle.icon.tinySize
                iconSource: 'back.png'
                text: qsTr('Back');

                onClicked: panel.state = ''
            }

            Button {
                anchors.top: parent.top
                iconSize: appStyle.icon.tinySize
                iconSource: details.model.installed ? 'checkbox-checked.png' : 'checkbox.png'
                text: details.model.installed ? qsTr('Desactivate') : qsTr('Activate')

                onClicked: details.model.installed = !details.model.installed
            }
        }

        Image {
            id: image
            anchors.left: parent.left
            anchors.top: header.bottom
            source: details.model.image !== undefined && details.model.image !== '' ? details.model.image : 'plugin.png'
        }

        Column {
            anchors.top: image.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            Text {
                elide: Text.ElideRight
                font.bold: true
                text: details.model.name !== undefined ? details.model.name : ''
                width: parent.width
            }
            Text {
                text: details.model.description !== undefined ? details.model.description : ''
                width: parent.width
                wrapMode: Text.Wrap
            }
        }
    }

    states: State {
        name: 'detailed'
        PropertyChanges { target: list; opacity: 0 }
        PropertyChanges { target: details; opacity: 1 }
    }
}

