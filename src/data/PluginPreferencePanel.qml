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
        id: installedModel
        ListElement { installed: true; name: 'Music'; summary: 'Play sounds and manage albums with wiLink !'; description: 'This plugin add a music manager to wiLink.'; image: 'start.png'; }
        ListElement { installed: true; name: 'Rss reader'; summary: 'Follow your friends over rss'; description: 'This plugin add a rss reader to wiLink.'; image: 'peer.png'; }
    }
    ListModel {
        id: availableModel
        ListElement { installed: false; name: 'Pong'; summary: 'Basic pong game'; description: 'This is a pong game with 4 basic levels. Use left and right arrow keys to move.'; image: 'favorite-active.png'; }
        ListElement { installed: false; name: 'Go! Go fish !'; summary: 'Free a lovely fish in wiLink\'s window'; description: 'This application display a pink fish in wiLink\'s window. Yes this is useless, but it\'s really fun !'; image: 'tip.png'; }
        ListElement { installed: false; name: 'H4ck3r'; summary: 'Show a fake hacking console'; description: 'This is a fun fake console, with abstract commands running inside.'; image: ''; }
        ListElement { installed: false; name: 'Storm'; summary: 'Destroy ! Destroy !'; description: 'Display a storm inside wiLink\'s window'; image: 'diagnostics.png'; }
    }
    /* end: hard-coded models */

    GroupBox {
        id: available

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.floor(parent.height * 0.6)
        title: qsTr('Available plugins')

        ListView {
            id: availableView

            anchors.left: parent.contents.left
            anchors.top: parent.contents.top
            anchors.bottom: parent.contents.bottom
            anchors.right: availableScrollBar.left
            clip: true
            delegate: pluginDelegate
            model: availableModel
            spacing: appStyle.spacing.vertical
        }

        ScrollBar {
            id: availableScrollBar

            anchors.top: parent.contents.top
            anchors.bottom: parent.contents.bottom
            anchors.right: parent.contents.right
            flickableItem: availableView
        }
    }

    GroupBox {
        id: installed

        anchors.top: available.bottom
        anchors.topMargin: appStyle.spacing.vertical
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Installed plugins')

        ListView {
            id: installedView

            anchors.left: parent.contents.left
            anchors.top: parent.contents.top
            anchors.bottom: parent.contents.bottom
            anchors.right: installedScrollBar.left
            clip: true
            delegate: pluginDelegate
            model: installedModel
            spacing: appStyle.spacing.vertical
        }

        ScrollBar {
            id: installedScrollBar

            anchors.top: parent.contents.top
            anchors.bottom: parent.contents.bottom
            anchors.right: parent.contents.right
            flickableItem: installedView
        }
    }

    Component {
        id: pluginDelegate

        Item {
            height: 32
            width: parent.width - 1

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
                anchors.right: button.left
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

            Button {
                id: button

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                iconSource: model.installed ? 'close.png' : 'add.png'
                iconSize: appStyle.icon.tinySize
            }
        }
    }

    Item {
        id: details

        property QtObject model: ListModel { }

        anchors.fill: parent
        opacity: 0

        Image {
            id: image
            anchors.left: parent.left
            anchors.top: parent.top
            source: details.model.image !== undefined && details.model.image !== '' ? details.model.image : 'plugin.png'
        }

        Button {
            anchors.top: parent.top
            anchors.right: actionButton.left
            anchors.rightMargin: appStyle.spacing.horizontal
            iconSize: appStyle.icon.tinySize
            iconSource: 'back.png'
            text: qsTr('Back');

            onClicked: panel.state = ''
        }

        Button {
            id: actionButton

            anchors.top: parent.top
            anchors.right: parent.right
            iconSize: appStyle.icon.tinySize
            iconSource: details.model.installed ? 'remove.png' : 'add.png'
            text: details.model.installed ? qsTr('Uninstall') : qsTr('Install')
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
        PropertyChanges { target: available; opacity: 0 }
        PropertyChanges { target: installed; opacity: 0 }
        PropertyChanges { target: details; opacity: 1 }
    }
}

