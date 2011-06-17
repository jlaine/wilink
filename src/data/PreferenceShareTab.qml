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
import wiLink 1.2

Panel {
    id: panel

    function save() {
    }

    GroupBox {
        id: places

        anchors.top: parent.top
        anchors.bottom: downloads.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Shared folders')

        ShareFolderModel {
            id: folderModel
        }

        SharePlaceModel {
            id: placeModel

            sourceModel: folderModel
        }

        Item {
            anchors.fill: places.contents

            PanelHelp {
                id: help

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr('Select the folders you want to share. The files you share will only be visible in your residence, they can never be accessed outside your residence.')
            }

            ListView {
                id: view

                anchors.top: help.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: scrollBar.left
                model: placeModel

                delegate: Item {
                    height: appStyle.icon.smallSize
                    width: view.width - 1

                    Image {
                        id: image

                        anchors.left: parent.left
                        anchors.top: parent.top
                        height: appStyle.icon.smallSize
                        width: appStyle.icon.smallSize
                        source: 'album.png'
                    }

                    Text {
                        anchors.left: image.right
                        anchors.leftMargin: appStyle.spacing.horizontal
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: model.name
                    }
                }
            }

            ScrollBar {
                id: scrollBar

                anchors.top: help.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                flickableItem: view
            }
        }
    }

    GroupBox {
        id: downloads

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 130
        title: qsTr('Downloads folder')

        Column {
            anchors.fill: downloads.contents
            spacing: appStyle.spacing.vertical

            PanelHelp {
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr('Select the folder in which received files will be stored.')
            }

            Item {
                anchors.left: parent.left
                anchors.right: parent.right
                height: input.height

                InputBar {
                    id: input

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: button.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                }

                Button {
                    id: button

                    anchors.top: parent.top
                    anchors.right: parent.right
                    iconSource: 'album.png'
                }
            }
        }
    }
}

