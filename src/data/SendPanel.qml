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

    width: 600
    height: 100

    Style {
        id: appStyle
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: '#888'
    }

    CrumbBar {
        id: crumbBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        z: 1

        onLocationChanged: {
            if (location.isDir) {
                folderModel.rootUrl = location.url;
            }
        }
    }

    ToolBar {
        id: rootView

        anchors.centerIn: parent
        spacing: appStyle.icon.normalSize

        ToolButton {
            iconSize: appStyle.icon.largeSize
            iconSource: '128x128/file.png'
            width: appStyle.icon.largeSize * 1.5
            text: qsTr('Send a file')

            onClicked: {
                crumbBar.push({'name': 'Files', 'isDir': true, 'url': 'file:///'});
            }
        }

        ToolButton {
            iconSize: appStyle.icon.largeSize
            iconSource: '128x128/photos.png'
            width: appStyle.icon.largeSize * 1.5
            text: qsTr('Send a photo')

            onClicked: {
                crumbBar.push({'name': 'Photos', 'isDir': true, 'url': 'wifirst://www.wifirst.net/w'});
            }
        }
    }

    ScrollView {
        id: folderView

        anchors.top: crumbBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        delegate: Item {
            id: item

            width: view.width
            height: 64

            function clicked() {
                crumbBar.push({'name': model.name, 'isDir': model.isDir, 'url': model.url});
                folderModel.rootUrl = model.url;
                folderView.forceActiveFocus();
            }

            Image {
                id: icon

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                source: model.isDir ? '128x128/album.png' : '128x128/file.png'
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
                anchors.fill: parent
                onClicked: item.clicked()
            }
        }
        model: PhotoModel {
            id: folderModel
        }
        opacity: 0
    }

    Component.onCompleted: {
        crumbBar.push({'name': 'Send', 'isDir': true, url: ''});
    }
    Keys.onPressed: {
        if (event.key == Qt.Key_Back ||
                 event.key == Qt.Key_Backspace ||
                 event.key == Qt.Key_Escape) {
            if (crumbBar.model.count > 1) {
                crumbBar.pop();
            }
        }
        else if (event.key == Qt.Key_Enter ||
                 event.key == Qt.Key_Return) {
            folderView.currentItem.clicked();
        }
    }

    states: State {
        name: 'browsing'
        when: (folderModel.rootUrl != '')
        PropertyChanges { target: folderView; opacity: 1 }
        PropertyChanges { target: rootView; opacity: 0 }
    }
}
