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

Item {
    id: panel

    property variant jid

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    clip: true
    height: 200

    FolderModel {
        id: folderModel
    }

    ListModel {
        id: placeModel

        Component.onCompleted: {
            placeModel.append({'avatar': 'image://icon/file', 'name': 'Files', 'isDir': true, 'url': application.settings.homeUrl});
            placeModel.append({'avatar': 'image://icon/photos', 'name': 'Photos', 'isDir': true, 'url': 'wifirst://default'});
            placeModel.append({'avatar': 'image://icon/share', 'name': 'Shares', 'isDir': true, 'url': 'share:'})
        }
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
            if (location.url) {
                folderModel.rootUrl = location.url;
                folderView.model = folderModel;
            } else {
                folderView.model = placeModel;
            }
            folderView.forceActiveFocus();
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
            height: appStyle.icon.normalSize

            function clicked() {
                view.currentIndex = model.index;
                if (model.isDir) {
                    crumbBar.push({'name': model.name, 'isDir': model.isDir, 'url': model.url});
                }
            }

            Image {
                id: icon

                anchors.left: parent.left
                anchors.leftMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                fillMode: Image.PreserveAspectFit
                source: model.avatar
                width: appStyle.icon.smallSize
                height: appStyle.icon.smallSize
            }

            Label {
                anchors.left: icon.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                anchors.rightMargin: appStyle.margin.normal
                anchors.verticalCenter: parent.verticalCenter
                text: model.name
            }

            MouseArea {
                anchors.fill: parent
                onClicked: item.clicked()
                onDoubleClicked: {
                    console.log("send " + model.url + " to " + panel.jid);
                }
            }
        }
        model: placeModel
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
}
