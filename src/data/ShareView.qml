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

Item {
    id: block

    property alias model: view.model

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#ffffff'
    }

    ListView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left

        delegate: Item {
            id: item
            width: view.width - 1
            height: 24

            Item {
                id: main

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.right: downloadButton.left
                anchors.rightMargin: 4

                Highlight {
                    id: highlight

                    anchors.fill: parent
                    state: 'inactive'
                }

                Image {
                    id: thumbnail

                    anchors.left: parent.left
                    anchors.margins: 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    smooth: true
                    source: model.isDir ? (model.node.length ? 'album.png' : 'peer.png') : 'file.png'

                    Text {
                        anchors.centerIn: parent
                        color: '#444444'
                        font.pixelSize: 10
                        text: model.popularity > 0 ? model.popularity : ''
                    }
                }

                Text {
                    id: text

                    anchors.left: thumbnail.right
                    anchors.right: sizeText.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: 4
                    text: model.name
                }

                Text {
                    id: sizeText

                    anchors.right: parent.right
                    anchors.margins: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: (model.size > 0 || !model.isDir) ? Utils.formatSize(model.size) : ''
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true

                    onClicked: {
                        if (model.isDir) {
                            crumbBar.push(model);
                        }
                    }

                    onEntered: highlight.state = ''
                    onExited: highlight.state = 'inactive'
                }
            }

            Button {
                id: downloadButton
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                iconSource: 'download.png'
                visible: model.canDownload

                onClicked: {
                    view.model.download(model.index);
                }
            }

        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}

