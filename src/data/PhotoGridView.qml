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

FocusScope {
    id: wrapper

    property alias contentX: view.contentX
    property alias contentY: view.contentY
    property alias currentIndex: view.currentIndex
    property alias currentItem: view.currentItem
    property alias model: view.model
    property int iconSize: 128

    GridView {
        id: view

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.margins: appStyle.margin.small
        cellWidth: iconSize + appStyle.margin.normal
        cellHeight: iconSize + appStyle.font.normalSize * 1.4 + appStyle.spacing.vertical
        focus: true
        highlight: Highlight {}

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
                spacing: appStyle.spacing.vertical

                PhotoDelegate {
                    id: thumbnail

                    anchors.horizontalCenter: parent.horizontalCenter
                    width: iconSize
                    height: iconSize
                }

                Label {
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

        DropArea {
            anchors.fill: parent
            enabled: photoModel.canUpload

            onFilesDropped: {
                for (var i in files) {
                    photoModel.upload(files[i]);
                }
            }

            // This force DropArea to ignore mouse event when disabled
            visible: enabled
        }

        Keys.onPressed: {
            if (event.key == Qt.Key_Delete ||
               (event.key == Qt.Key_Backspace && event.modifiers == Qt.ControlModifier)) {
                if (view.currentIndex >= 0) {
                    var model = view.currentItem.data();
                    dialogSwapper.showPanel('PhotoDeleteDialog.qml', {
                        avatar: model.avatar,
                        index: view.currentIndex,
                        model: photoModel,
                        name: model.name
                    });
                }
            }
            else if (event.key == Qt.Key_Back ||
                     event.key == Qt.Key_Backspace ||
                     event.key == Qt.Key_Escape) {
                if (crumbBar.model.count > 1) {
                    crumbBar.pop();
                }
            }
            else if (event.key == Qt.Key_Enter ||
                     event.key == Qt.Key_Return) {
                view.currentItem.clicked();
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}

