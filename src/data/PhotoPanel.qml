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
import wiLink 2.0

Panel {
    id: panel

    property bool canUpload: crumbBar.model.count > 1
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
                enabled: panel.canUpload

                onClicked: {
                    var dialog = window.fileDialog();
                    dialog.fileMode = QFileDialog.ExistingFiles;
                    dialog.nameFilters = [qsTr("Image files") + " (*.jpg *.jpeg *.png *.gif)",
                                          qsTr("All files") +" (*)"];
                    dialog.windowTitle = qsTr('Upload photos');
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            photoModel.upload(dialog.selectedFiles[i]);
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'start.png'
                text: qsTr('Slideshow')
                onClicked: panel.state = 'slide'
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
        z: 3
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
                flickView.currentIndex = gridView.currentIndex;
                flickView.positionViewAtIndex(flickView.currentIndex, ListView.Beginning);
                panel.state = 'details';
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: crumbBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: panel.canUpload ? qsTr('To upload your photos to wifirst.net, simply drag and drop them to an album.') : ''
        z: 2
    }

    PhotoModel {
        id: photoModel
    }

    FocusScope {
        anchors.top: help.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        focus: true

        PhotoGridView {
            id: gridView

            anchors.fill: parent
            focus: true
            model:  photoModel
        }

        PhotoFlickView {
            id: flickView

            anchors.fill: parent
            model: photoModel
            opacity: 0
            z: 1

            onCurrentIndexChanged: {
                if (panel.state == 'details') {
                    gridView.currentIndex = flickView.currentIndex;
                    var crumb = crumbBar.model.count - 1;
                    var model = gridView.currentItem.data();
                    crumbBar.model.setProperty(crumb, 'name', model.name);
                    crumbBar.model.setProperty(crumb, 'isDir', model.isDir);
                    crumbBar.model.setProperty(crumb, 'url', model.url);
                }
            }
        }

        PhotoSlideView {
            id: slideView

            anchors.fill: parent
            model: photoModel
            opacity: 0
            z: 1
        }
    }

    ShareQueueView {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: footer.count > 0 ? 120 : 0
        clip: true
        model: photoModel.uploads
        z: 1
    }

    states: [
        State {
            name: 'details'
            PropertyChanges { target: flickView; opacity: 1 }
            PropertyChanges { target: flickView; focus: true }
            PropertyChanges { target: gridView; opacity: 0; focus: false }
            PropertyChanges { target: crumbBar; anchors.topMargin: -help.height; opacity: 0 }
            PropertyChanges { target: help; anchors.topMargin: -help.height; opacity: 0 }
        },
        State {
            name: 'slide'
            PropertyChanges { target: slideView; opacity: 1 }
        }
    ]

    transitions: Transition {
        // slide the top bars up while fading out
        PropertyAnimation {
            target: help
            properties: 'anchors.topMargin,opacity'
            duration: appStyle.animation.longDuration
            easing.type: Easing.InOutQuad
        }
        PropertyAnimation {
            target: crumbBar
            properties: 'anchors.topMargin,opacity'
            duration: appStyle.animation.longDuration
            easing.type: Easing.InOutQuad
        }

        // replace the list view by the selected picture
        PropertyAnimation {
            target: flickView
            properties: 'opacity'
            duration: appStyle.animation.longDuration
            easing.type: Easing.InOutQuad
        }
        PropertyAnimation {
            target: gridView
            properties: 'opacity'
            duration: appStyle.animation.longDuration
            easing.type: Easing.InOutQuad
        }
    }
}
