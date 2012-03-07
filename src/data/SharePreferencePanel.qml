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
import wiLink 2.0

Panel {
    id: panel

    function save() {
        application.settings.sharesLocation = sharesLocation.text;
        application.settings.sharesDirectories = folderModel.selectedFolders;
    }

    color: 'transparent'

    GroupBox {
        id: places

        anchors.top: parent.top
        anchors.bottom: downloads.top
        anchors.bottomMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Shared folders')

        ShareFolderModel {
            id: folderModel

            forcedFolder: application.settings.sharesLocation
            selectedFolders: application.settings.sharesDirectories
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
                anchors.right: parent.right
                clip: true

                ScrollView {
                    id: placeView

                    anchors.fill: parent
                    highlight: Item {}
                    model: placeModel
                    delegate: Item {
                        height: appStyle.icon.smallSize
                        width: parent.width

                        CheckBox {
                            anchors.left: parent.left
                            anchors.leftMargin: appStyle.margin.normal
                            anchors.right: parent.right
                            anchors.rightMargin: appStyle.margin.normal
                            anchors.verticalCenter: parent.verticalCenter
                            checked: model.checkState == 2
                            iconSource: 'album.png'
                            text: model.name

                            onClicked: {
                                folderModel.setCheckState(model.path, checked ? 0 : 2);
                            }
                        }
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
                        z: 1

                        onLocationPopped: {
                            visualModel.rootIndex = visualModel.parentModelIndex();
                        }
                    }

                    ScrollView {
                        id: folderView

                        anchors.top: crumbs.bottom
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        highlight: Item {}
                        model: VisualDataModel {
                            id: visualModel

                            model: folderModel
                            delegate: Item {
                                height: appStyle.icon.smallSize
                                width: parent.width

                                CheckBox {
                                    id: check

                                    anchors.left: parent.left
                                    anchors.leftMargin: appStyle.margin.normal
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: model.checkState == 2
                                    width: 12
                                    onClicked: {
                                        folderModel.setCheckState(model.path, checked ? 0 : 2);
                                    }
                                }

                                Image {
                                    id: icon

                                    anchors.left: check.right
                                    anchors.leftMargin: appStyle.spacing.horizontal
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: 'album.png'
                                    width: appStyle.icon.smallSize
                                    height: appStyle.icon.smallSize
                                }

                                Label {
                                    anchors.left: icon.right
                                    anchors.leftMargin: appStyle.spacing.horizontal
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: model.name
                                }

                                MouseArea {
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    anchors.left: check.right
                                    anchors.right: parent.right

                                    onClicked: {
                                        crumbs.push({'name': model.name, 'path': model.path});
                                        visualModel.rootIndex = visualModel.modelIndex(model.index);
                                    }
                                }

                            }

                            Component.onCompleted: {
                                crumbs.push({'name': qsTr('My computer')});

                                // navigate into '/' on *nix
                                if (folderModel.isUnix) {
                                    rootIndex = modelIndex(0);
                                }
                            }
                        }
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
            PropertyChanges { target: placeView; opacity: 0 }
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
                height: button.height

                InputBar {
                    id: sharesLocation

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: button.left
                    anchors.rightMargin: appStyle.spacing.horizontal
                    readOnly: true
                    text: application.settings.sharesLocation
                }

                Button {
                    id: button

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    iconSource: 'album.png'

                    onClicked: {
                        var dialog = window.fileDialog();
                        dialog.directory = application.settings.sharesLocation;
                        dialog.fileMode = QFileDialog.Directory;
                        dialog.options = QFileDialog.ShowDirsOnly;
                        dialog.windowTitle = qsTr('Downloads folder');
                        if (dialog.exec() && dialog.selectedFiles.length > 0) {
                            application.settings.sharesLocation = dialog.selectedFiles[0];
                        }
                    }
                }
            }
        }
    }
}

