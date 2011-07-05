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
        title: qsTr('Photos')
        toolBar: ToolBar {
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
                text: qsTr('Create')
                enabled: crumbBar.model.count == 1

                onClicked: {
                    dialogSwapper.showPanel('PhotoAlbumDialog.qml', {'model': photoModel});
                }
            }
        }
    }

    CrumbBar {
        id: crumbBar

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1

        onLocationChanged: {
            if (location.isDir) {
                photoModel.rootUrl = location.url;
                panel.state = '';
            } else {
                displayView.currentIndex = view.currentIndex;
                displayView.positionViewAtIndex(displayView.currentIndex, ListView.Beginning);
                panel.state = 'details';
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: crumbBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('To upload your photos to wifirst.net, simply drag and drop them to an album.')
        z: 1
    }

    GridView {
        id: view

        anchors.top: help.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: scrollBar.left
        cellWidth: 130
        cellHeight: 150
        focus: true

        highlight: Highlight {}

        model:  PhotoModel {
            id: photoModel
        }

        delegate: Item {
            id: item

            width: view.cellWidth
            height: view.cellHeight

            function clicked() {
                crumbBar.push({'name': model.name, 'isDir': model.isDir, 'url': model.url});
            }

            function data() {
                return model;
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

                onClicked: {
                    view.currentIndex = model.index;
                    item.clicked();
                }
            }
        }

        Keys.onPressed: {
            if (event.key == Qt.Key_Back || event.key == Qt.Key_Backspace) {
                if (crumbBar.model.count > 1) {
                    crumbBar.pop();
                }
            }
            else if (event.key == Qt.Key_Return || event.key == Qt.Key_Enter) {
                view.currentItem.clicked();
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: help.bottom
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

        ListView {
            id: displayView

            anchors.fill: parent
            highlightMoveDuration: 500
            highlightRangeMode: ListView.StrictlyEnforceRange
            model: view.model
            orientation: Qt.Horizontal
            snapMode: ListView.SnapToItem

            delegate: Component {
                Image {
                    id: image

                    width: displayView.width
                    height: displayView.height
                    source: model.image
                    fillMode: Image.PreserveAspectFit
                }
            }

            onCurrentIndexChanged: {
                if (panel.state == 'details') {
                    view.currentIndex = displayView.currentIndex;
                    var crumb = crumbBar.model.count - 1;
                    var model = view.currentItem.data();
                    crumbBar.model.setProperty(crumb, 'name', model.name);
                    crumbBar.model.setProperty(crumb, 'isDir', model.isDir);
                    crumbBar.model.setProperty(crumb, 'url', model.url);
                }
            }

            Keys.onPressed: {
                if (event.key == Qt.Key_Back || event.key == Qt.Key_Backspace) {
                    if (crumbBar.model.count > 1) {
                        crumbBar.pop();
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
        clip: true
        model: photoModel.uploads
        z: 1
    }

    states: State {
        name: 'details'
        PropertyChanges { target: display; opacity: 1 }
        PropertyChanges { target: displayView; focus: true }
        PropertyChanges { target: view; opacity: 0; focus: false }
        PropertyChanges { target: help; height: 0; opacity: 0 }
    }

    transitions: Transition {
        PropertyAnimation { target: display; properties: 'opacity'; duration: 150 }
        PropertyAnimation { target: help; properties: 'height,opacity'; duration: 150 }
    }
}
