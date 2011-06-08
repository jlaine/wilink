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

    property alias count: view.count
    property alias model: view.model

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#ffffff'
    }

    GridView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cellHeight: 128
        cellWidth: 128

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Highlight {
                id: highlight

                anchors.fill: parent
                state: 'inactive'
            }

            Image {
                id: thumbnail

                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: 80
                height: 80
                smooth: true
                source: model.isDir ? (model.node.length ? 'album-128.png' : 'peer-128.png') : 'file-128.png'
/*
                Rectangle {
                    anchors.top: parent.top
                    anchors.left: parent.right
                    anchors.margins: 4
                    border.color: '#597fbe'
                    border.width: 1
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: '#66597fbe' }
                        GradientStop { position: 0.6; color: '#597fbe' }
                        GradientStop { position: 1.0; color: '#66597fbe' }
                    }
                    height: popularity.paintedHeight + 4
                    width: popularity.paintedWidth + 4
                    radius: 9
                    smooth: true
                    visible: model.popularity != 0

                    Text {
                        id: popularity

                        anchors.centerIn: parent
                        color: '#ffffff'
                        font.pixelSize: 14
                        font.bold: true
                        text: model.popularity > 0 ? model.popularity : ''
                    }
                }
*/
            }

            Text {
                id: text

                anchors.top: thumbnail.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter

                text: model.name
            }


            Text {
                id: sizeText

                anchors.centerIn: thumbnail
                elide: Text.ElideLeft
                font.pixelSize: 12
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

                onEntered: {
                    highlight.state = ''
                    downloadButton.state = ''
                }
                onExited: {
                    highlight.state = 'inactive'
                    downloadButton.state = 'inactive'
                }
            }

            Button {
                id: downloadButton

                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: 4
                height: 20
                iconSource: 'download.png'
                state: 'inactive'
                text: model.isDir ? qsTr('Download folder') : qsTr('Download')
                visible: model.canDownload

                onClicked: {
                    view.model.download(model.index);
                }

                states: State {
                    name: 'inactive'
                    PropertyChanges { target: downloadButton; opacity: 0 }
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

