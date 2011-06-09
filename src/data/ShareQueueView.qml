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

            Image {
                id: thumbnail

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.margins: 4
                width: 24
                height: 24
                smooth: true
                source: model.isDir ? (model.node.length ? 'album.png' : 'peer.png') : 'file.png'
            }

            Item {
                id: main

                anchors.left: thumbnail.right
                anchors.right: cancelButton.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: 4

                ProgressBar {
                    anchors.fill: parent
                    maximumValue: model.totalBytes
                    value: model.doneBytes
                    //visible: model.doneBytes > 0
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: {
                        var text = model.name + ' - ';
                        if (model.totalFiles > 1)
                            text +=  qsTr('%1 files').replace('%1', model.totalFiles) + ', ';
                        text += Utils.formatSize(model.totalBytes);
                        if (model.speed > 0)
                            text += ' - ' + Utils.formatSpeed(model.speed);
                        return text;
                    }
                }
            }

            Button {
                id: cancelButton

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.rightMargin: 4
                iconSource: 'close.png'
                text: qsTr('Cancel')

                onClicked: view.model.cancel(model.index)
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

