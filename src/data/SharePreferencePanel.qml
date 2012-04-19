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
        var urls = [];
        for (var i = 0; i < selectionModel.count; ++i) {
            urls[i] = selectionModel.get(i).url;
        }

        application.settings.sharesLocation = sharesLocation.text;
        application.settings.sharesDirectories = urls;
    }

    GroupBox {
        id: places

        anchors.top: parent.top
        anchors.bottom: downloads.top
        anchors.bottomMargin: appStyle.spacing.vertical
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr('Shared folders')

        ListModel {
            id: selectionModel

            function getState(url) {
                for (var i = 0; i < selectionModel.count; ++i) {
                    if (selectionModel.get(i).url == url)
                        return true;
                }
                return false;
            }

            function setState(url, checked) {
                // FIXME: disallow change on root
                //if (changedPath == m_forced || changedPath.startsWith(m_forced + "/"))
                //    return;

                if (checked) {
                    var changedUrl = '' + url;
                    var found = false;
                    for (var i = selectionModel.count - 1; i >= 0; --i) {
                        var currentUrl = '' + selectionModel.get(i).url;
                        if (currentUrl == changedUrl) {
                            found = true;
                        } else if ((currentUrl.indexOf(changedUrl + '/') == 0) ||
                                   (changedUrl.indexOf(currentUrl + '/') == 0)) {
                            // unselect any children or parents
                            selectionModel.remove(i);
                        }
                    }
                    if (!found)
                        selectionModel.append({'url': url});
                } else {
                    for (var i = selectionModel.count - 1; i >= 0; --i) {
                        if (selectionModel.get(i).url == url) {
                            selectionModel.remove(i);
                            break;
                        }
                    }
                }
            }

            Component.onCompleted: {
                var urls = application.settings.sharesDirectories;
                for (var i in urls) {
                    selectionModel.append({'url': urls[i]});
                }
            }
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
                    model: PlaceModel {}
                    delegate: Item {
                        height: appStyle.icon.smallSize
                        width: parent.width

                        CheckBox {
                            anchors.left: parent.left
                            anchors.leftMargin: appStyle.margin.normal
                            anchors.right: parent.right
                            anchors.rightMargin: appStyle.margin.normal
                            anchors.verticalCenter: parent.verticalCenter
                            checked: selectionModel.getState(model.url)
                            iconSource: model.avatar
                            text: model.name

                            onClicked: selectionModel.setState(model.url, !checked)
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

                        onLocationChanged: {
                            folderView.model.rootUrl = location.url;
                        }
                    }

                    ScrollView {
                        id: folderView

                        anchors.top: crumbs.bottom
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        highlight: Item {}
                        model: FolderModel {
                            showFiles: false
                        }
                        delegate: Item {
                            height: appStyle.icon.smallSize
                            width: parent.width

                            CheckBox {
                                id: check

                                anchors.left: parent.left
                                anchors.leftMargin: appStyle.margin.normal
                                anchors.verticalCenter: parent.verticalCenter
                                checked: selectionModel.getState(model.url)
                                width: 12
                                onClicked: selectionModel.setState(model.url, !checked)
                            }

                            Image {
                                id: icon

                                anchors.left: check.right
                                anchors.leftMargin: appStyle.spacing.horizontal
                                anchors.verticalCenter: parent.verticalCenter
                                source: model.avatar
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
                                    crumbs.push({'name': model.name, 'url': model.url});
                                }
                            }
                        }
                    }

                    Component.onCompleted: {
                        crumbs.push({'name': qsTr('My computer'), 'url': 'file:///'});
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
                    iconSource: 'image://icon/album'

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

