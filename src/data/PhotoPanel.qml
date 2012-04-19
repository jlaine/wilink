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
        iconSource: 'image://icon/photos'
        title: qsTr('Photos')
        toolBar: ToolBar {
            ToolButton {
                enabled: crumbBar.model.count > 1
                iconSource: 'image://icon/back'
                text: qsTr('Go back')

                onClicked: crumbBar.pop()
            }

            ToolButton {
                iconSource: 'image://icon/upload'
                text: qsTr('Upload')
                enabled: photoModel.canUpload
                visible: panel.state == ''

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
                iconSource: 'image://icon/stop'
                text: qsTr('Cancel')
                visible: false
            }

            ToolButton {
                iconSource: 'image://icon/add'
                text: qsTr('Create')
                enabled: photoModel.canCreateAlbum
                visible: panel.state == ''

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
        text: photoModel.canUpload ? qsTr('To upload your photos to wifirst.net, simply drag and drop them to an album.') : ''
        z: 2
    }

    FolderModel {
        id: photoModel
    }

    FocusScope {
        id: wrapper

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

            Spinner {
                anchors.centerIn: parent
                busy: photoModel.busy
            }
        }

        PhotoFlickView {
            id: flickView

            x: gridView.currentItem ? (gridView.currentItem.x - gridView.contentX) : 0
            y: gridView.currentItem ? (gridView.currentItem.y - gridView.contentY) : 0
            height: 128
            width: 128
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

    FolderQueueView {
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
            PropertyChanges {
                target: flickView
                x: 0
                y: 0
                width: wrapper.width
                height: wrapper.height
                focus: true
                opacity: 1
            }
            PropertyChanges { target: gridView; opacity: 0; focus: false }
            PropertyChanges { target: crumbBar; anchors.topMargin: -help.height; opacity: 0 }
            PropertyChanges { target: help; anchors.topMargin: -help.height; opacity: 0 }
        },
        State {
            name: 'slide'
            PropertyChanges { target: gridView; opacity: 0; focus: false }
            PropertyChanges { target: slideView; opacity: 1; running: true }
            PropertyChanges { target: crumbBar; anchors.topMargin: -help.height; opacity: 0 }
            PropertyChanges { target: help; anchors.topMargin: -help.height; opacity: 0 }
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
            properties: 'opacity,x,y,height,width'
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
