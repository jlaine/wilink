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

    property QtObject soundJob

    function finished() {
        panel.soundJob = null;
    }

    function play(model) {
        stop();
        panel.soundJob = application.soundPlayer.play(model.url);
        panel.soundJob.finished.connect(panel.finished);
    }

    function stop() {
        if (panel.soundJob) {
            panel.soundJob.finished.disconnect(panel.finished);
            panel.soundJob.stop();
            panel.soundJob = null;
        }
    }

    ListHelper {
        id: listHelper
        model: playerView.model
    }

    Component {
        id: playerDelegate
        Item {
            id: item
            property bool isSelected: playerView.currentItem == item
            height: 42
            width: parent.width

            function formatDuration(ms) {
                var secs = ms / 1000;
                var hours = Number(secs / 3600).toFixed();
                var minutes = Number(secs / 60).toFixed() % 60;
                var seconds = Number(secs).toFixed() % 60;

                function padNumber(n) {
                    var s = n.toString();
                    if (s.length == 1)
                        return '0' + s;
                    return s;
                }

                if (hours > 0)
                    return hours.toString() + ':' + padNumber(minutes) + ':' + padNumber(seconds);
                else
                    return minutes.toString() + ':' + padNumber(seconds);
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    playerView.currentIndex = index;
                }

                onDoubleClicked: {
                    panel.play(model);
                }
            }

            Item {
                id: rect
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.bottomMargin: 2
                anchors.leftMargin: 3
                anchors.rightMargin: 3

                Image {
                    id: imageColumn
                    anchors.left: parent.left
                    anchors.leftMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    fillMode: Image.PreserveAspectFit
                    width: 32
                    height: 32
                    source: {
                        if (!Qt.isQtObject(panel.soundJob) || panel.soundJob.url != model.url)
                            return model.imageUrl;
                        if (panel.soundJob.state == SoundPlayerJob.DownloadingState)
                            return "download.png";
                        else
                            return "start.png";
                    }
                }

                Column {
                    id: textColumn
                    anchors.left: imageColumn.right
                    anchors.verticalCenter: parent.verticalCenter

                    Label { text: '<b>' + title + '</b>' }
                    Label { text: (item.isSelected && album) ? artist + ' - ' + album : artist }
                }

                Label {
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                    text: formatDuration(duration)
                }
            }
        }
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'start.png'
        title: qsTr('Music player')
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'add.png'
                text: qsTr('Add')

                onClicked: {
                    var dialog = window.fileDialog();
                    dialog.fileMode = QFileDialog.ExistingFiles;
                    dialog.nameFilters = [qsTr("Sound files") + " (*.mp3 *.ogg *.wav)",
                                          qsTr("All files") +" (*)"];
                    dialog.windowTitle = qsTr('Add files');
                    if (dialog.exec()) {
                        for (var i in dialog.selectedFiles) {
                            playerModel.addLocalFile(dialog.selectedFiles[i]);
                        }
                    }
                }
            }

            ToolButton {
                iconSource: 'stop.png'
                text: qsTr('Stop')
                enabled: Qt.isQtObject(panel.soundJob)

                onClicked: {
                    panel.stop();
                }
            }

            ToolButton {
                iconSource: 'clear.png'
                text: qsTr('Clear')

                onClicked: {
                    panel.stop();
                    playerModel.clear();
                }
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('To add music to the player, simply drag and drop files to the playlist.');
        z: 1
    }

    ScrollView {
        id: playerView

        anchors.top: help.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        model: PlayerModel {
            id: playerModel
        }
        delegate: playerDelegate
        focus: true

        DropArea {
            anchors.fill:  parent

            onFilesDropped: {
                for (var i in files) {
                    playerModel.addLocalFile(files[i]);
                }
            }
        }

        Keys.onPressed: {
            if (event.key == Qt.Key_Delete ||
               (event.key == Qt.Key_Backspace && event.modifiers == Qt.ControlModifier)) {
                if (currentIndex >= 0)
                    playerModel.removeRow(currentIndex);
            }
            else if (event.key == Qt.Key_Enter ||
                     event.key == Qt.Key_Return) {
                if (currentIndex >= 0)
                    panel.play(listHelper.get(currentIndex));
            }
        }
    }
}
