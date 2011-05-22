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

    ListModel {
        id: crumbs
    }

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        icon: 'photos.png'
        title: '<b>' + qsTr('Photos') + '</b>'
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'back.png'
                enabled: crumbs.count > 0
                text: qsTr('Go back')

                onClicked: {
                    var crumb = crumbs.get(crumbs.count - 1);
                    view.model.rootUrl = crumb.url;
                    image.source = '';
                    crumbs.remove(crumbs.count - 1);
                }
            }

            ToolButton {
                icon: 'upload.png'
                text: qsTr('Upload')
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
                icon: 'stop.png'
                text: qsTr('Cancel')
                visible: false
            }

            ToolButton {
                icon: 'add.png'
                text: qsTr('Create an album')
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }

    }

    GridView {
        id: view

        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: header.bottom
        cellWidth: 130
        cellHeight: 150

        model:  PhotoModel {
            id: photoModel
            rootUrl: baseUrl
        }

        delegate: Item {
            id: item
            width: view.cellWidth
            height: view.cellHeight

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: '#00ffffff'
                border.width: 1
                gradient: Gradient {
                    GradientStop { id: stop1; position: 0.0; color: '#00ffffff'  }
                    GradientStop { id: stop2; position: 1.0; color: '#00ffffff'  }
                }
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
                }

                Text {
                    id: text
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: model.name
                }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onDoubleClicked: {
                    crumbs.append({'url': view.model.rootUrl})
                    if (model.isDir) {
                        view.model.rootUrl = model.url;
                    } else {
                        image.source = model.avatar
                    }
                }

                onEntered: {
                    parent.state = 'hovered'
                }
                onExited: {
                    parent.state = ''
                }
            }

            states: State {
                name: 'hovered'
                PropertyChanges { target: itemBackground; border.color: '#b0e2ff' }
                PropertyChanges { target: stop1;  color: '#ffffff' }
                PropertyChanges { target: stop2;  color: '#b0e2ff' }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.right: parent.right
        flickableItem: view
    }

    Rectangle {
        id: display

        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        visible: false
        z: 1

        Image {
            id: image

            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
        }
    }

    PhotoUploadView {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        model: photoModel.uploads
/*
        model: ListModel {
            ListElement { name: 'Foo Bar.png'; avatar: 'file.png' }
            ListElement { name: 'Foo Bar.png'; avatar: 'file.png' }
            ListElement { name: 'Foo Bar.png'; avatar: 'file.png' }
            ListElement { name: 'Foo Bar.png'; avatar: 'file.png' }
        } 
*/
        z: 1
    }

    state: image.source  != '' ? 'details' : ''
    states: State {
        name: 'details'
        PropertyChanges { target: display; visible: true }
    }
}
