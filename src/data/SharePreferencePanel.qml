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
        application.sharesLocation = sharesLocation.text;
        application.sharesDirectories = folderModel.selectedFolders;
    }

    color: 'transparent'

    GroupBox {
        id: places

        anchors.top: parent.top
        anchors.bottom: downloads.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Shared folders')

        ShareFolderModel {
            id: folderModel

            forcedFolder: application.sharesLocation
            selectedFolders: application.sharesDirectories
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

            Item {
                id: toggle

                anchors.top: help.bottom
                anchors.topMargin: appStyle.spacing.vertical
                anchors.bottom: placeButton.top
                anchors.bottomMargin: appStyle.spacing.vertical
                anchors.left: parent.left
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                anchors.rightMargin: appStyle.spacing.horizontal

                Item {
                    id: placePanel

                    anchors.fill: parent

                    ListView {
                        id: placeView

                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: placeBar.left
                        model: placeModel

                        delegate: CheckBox {
                            height: appStyle.icon.smallSize
                            width: placeView.width - 1
                            checked: model.checkState == 2
                            iconSource: 'album.png'
                            text: model.name
                            onClicked: {
                                folderModel.setCheckState(model.path, checked ? 0 : 2);
                            }
                        }

                        highlight: Highlight{}
                    }

                    ScrollBar {
                        id: placeBar

                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        flickableItem: placeView
                    }
                }

                Item {
                    id: folderPanel

                    anchors.fill: parent
                    opacity: 0

                    CrumbBar {
                        id: crumbs

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        onLocationChanged: {
                            visualModel.rootIndex = folderModel.index(location.path);
                        }
                    }

                    ListView {
                        id: folderView

                        anchors.top: crumbs.bottom
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: folderBar.left
                        clip: true
                        model: VisualDataModel {
                            id: visualModel

                            model: folderModel
                            delegate: Item {
                                height: appStyle.icon.smallSize
                                width: folderView.width - 1

                                CheckBox {
                                    id: check

                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    checked: model.checkState == 2
                                    iconSource: 'album.png'
                                    width: 12 + appStyle.icon.smallSize + appStyle.spacing.horizontal
                                    onClicked: {
                                        folderModel.setCheckState(model.path, checked ? 0 : 2);
                                    }
                                }

                                Text {
                                    anchors.left: check.right
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: model.name

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            crumbs.push({'name': model.name, 'path': model.path});
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ScrollBar {
                        id: folderBar

                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        flickableItem: folderView
                    }
                }
            }

            Button {
                id: placeButton

                property string moreString: qsTr('More folders..')
                property string lessString: qsTr('Fewer folders..')

                anchors.bottom: parent.bottom
                anchors.right: parent.right
                text: moreString

                onClicked: {
                    places.state = (places.state == '') ? 'folders': ''
                }
            }
        }

        states: State {
            name: 'folders'
            PropertyChanges { target: folderPanel; opacity: 1 }
            PropertyChanges { target: placePanel; opacity: 0 }
            PropertyChanges { target: placeButton; text: placeButton.lessString }
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
                height: sharesLocation.height

                InputBar {
                    id: sharesLocation

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: button.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    text: application.sharesLocation
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

