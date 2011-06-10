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

    property variant url

    onUrlChanged: {
        if (!crumbBar.model.count) {
            crumbBar.push({'name': qsTr('Home'), 'isDir': true, 'url': url});
        }
    }

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'photos.png'
        title: '<b>' + qsTr('Photos') + '</b>'
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                enabled: crumbBar.model.count > 1
                iconSource: 'back.png'
                text: qsTr('Go back')

                onClicked: crumbBar.pop()
            }

            ToolButton {
                iconSource: 'upload.png'
                text: qsTr('Upload')
                enabled: crumbBar.model.count > 1

                onClicked: {
                    var dialog = window.fileDialog();
                    dialog.fileMode = QFileDialog.ExistingFiles;
                    dialog.windowTitle = qsTr('Upload photos');
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            photoModel.upload(dialog.selectedFiles[i]);
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'stop.png'
                text: qsTr('Cancel')
                visible: false
            }

            ToolButton {
                iconSource: 'add.png'
                text: qsTr('Create an album')
                enabled: crumbBar.model.count == 1

                onClicked: {
                    dialog.source = 'InputDialog.qml';
                    dialog.item.title = qsTr('Create an album');
                    dialog.item.labelText = qsTr('Album name:');
                    dialog.item.accepted.connect(function() {
                        var name = dialog.item.textValue;
                        if (name.length > 0) {
                            photoModel.createAlbum(name);
                        }
                        dialog.item.hide();
                    });
                }
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('To upload your photos to wifirst.net, simply drag and drop them to an album.')
        z: 1
    }

    CrumbBar {
        id: crumbBar

        anchors.top: help.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        onLocationChanged: {
            if (location.isDir) {
                photoModel.rootUrl = location.url;
                image.url = '';
                image.source = '';
            } else {
                image.url = location.url;
                image.source = location.image;
            }
        }
        z: 1
    }

    GridView {
        id: view

        anchors.top: crumbBar.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cellWidth: 130
        cellHeight: 150

        model:  PhotoModel {
            id: photoModel
        }

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Highlight {
                id: highlight

                anchors.fill: parent
                state: 'inactive'
            }

            Column {
                anchors.fill: parent

                Image {
                    id: thumbnail

                    anchors.horizontalCenter: parent.horizontalCenter
                    fillMode: Image.PreserveAspectFit
                    width: 128
                    height: 128
                    source: model.avatar
                    smooth: true
                }

                Text {
                    id: text

                    anchors.left: parent.left
                    anchors.right: parent.right
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                    text: model.name
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true

                onClicked: crumbBar.push(model)
                onEntered: highlight.state = ''
                onExited: highlight.state = 'inactive'
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: crumbBar.bottom
        anchors.bottom: footer.top
        anchors.right: parent.right
        flickableItem: view
    }

    Rectangle {
        id: display

        anchors.top: crumbBar.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        opacity: 0
        z: 1

        Image {
            id: image

            property variant url

            anchors.fill: parent
            fillMode: Image.PreserveAspectFit

            Connections {
                target: view.model
                onPhotoChanged: {
                    if (url == image.url) {
                        image.source = source;
                    }
                }
            }
        }
    }

    PhotoUploadView {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: photoModel.uploads
        z: 1
    }

    state: image.source  != '' ? 'details' : ''
    states: State {
        name: 'details'
        PropertyChanges { target: display; opacity: 1 }
        PropertyChanges { target: view; opacity: 0 }
        PropertyChanges { target: help; height: 0; opacity: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: display; properties: 'opacity'; duration: 150 }
        PropertyAnimation { target: help; properties: 'height,opacity'; duration: 150 }
    }
}
