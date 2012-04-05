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
import 'utils.js' as Utils

Item {
    id: block

    property alias count: view.count
    property alias model: view.model
    property bool itemHovered: false

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#ffffff'
    }

    ScrollView {
        id: view

        anchors.fill: parent
        highlight: Item {}

        delegate: Item {
            id: item

            anchors.left: parent.left
            anchors.right: parent.right
            height: thumbnail.height + appStyle.margin.normal

            Highlight {
                id: highlight

                anchors.fill: parent
                state: 'inactive'
            }

            Image {
                id: thumbnail

                anchors.left: parent.left
                anchors.leftMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                width: appStyle.icon.normalSize
                height: appStyle.icon.normalSize
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: model.avatar
            }

            Image {
                id: fire

                anchors.fill: thumbnail
                source: 'fire.png'
                smooth: true
                visible: model.popularity > 10
            }

            Item {
                anchors.left: thumbnail.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: downloadButton.left
                anchors.rightMargin: appStyle.spacing.horizontal
                anchors.verticalCenter: parent.verticalCenter
                height: model.size > 0 ? text.height + sizeText.height : text.height

                Label {
                    id: text

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    elide: Text.ElideRight
                    text: model.name
                }

                Label {
                    id: sizeText

                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    elide: Text.ElideLeft
                    text: (model.size > 0 || !model.isDir) ? Utils.formatSize(model.size) : ''
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    itemHovered = false
                    if (model.isDir) {
                        crumbBar.push({'name': model.name, 'url': model.url});
                    }
                }

                onEntered: {
                    downloadButton.state = ''
                    highlight.state = ''
                    itemHovered = true
                }
                onExited: {
                    highlight.state = 'inactive'
                    downloadButton.state = 'inactive'
                    itemHovered = false
                }
            }

            Button {
                id: downloadButton

                anchors.right: parent.right
                anchors.rightMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                iconSource: 'download.png'
                state: 'inactive'
                text: qsTr('Download')

                onClicked: {
                    view.model.download(model.index);
                }

                states: State {
                    name: 'inactive'
                    PropertyChanges { target: downloadButton; opacity: 0.5 }
                }
            }
        }
    }
}

