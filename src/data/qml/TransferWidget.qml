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
import 'utils.js' as Utils

Item {
    id: item

    property QtObject job: null

    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined
    height: appStyle.icon.smallSize + 4 * appStyle.margin.normal

    Rectangle {
        id: background
        anchors.fill: parent
        border.color: '#93b9f2'
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0; color: '#e7effd' }
            GradientStop { position: 1; color: '#cbdaf1' }
        }
    }

    Image {
        id: icon

        width: appStyle.icon.smallSize
        height: appStyle.icon.smallSize
        anchors.left: parent.left
        anchors.leftMargin: appStyle.margin.normal
        anchors.verticalCenter: parent.verticalCenter
        source: (Qt.isQtObject(job) && job.direction == QXmppTransferJob.OutgoingDirection) ? 'image://icon/upload' : 'image://icon/download'
    }

    ProgressBar {
        id: progressBar

        anchors.margins: appStyle.margin.normal
        anchors.left: icon.right
        anchors.right: openButton.left
        anchors.verticalCenter: parent.verticalCenter

        Label {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            clip: true
            text: {
                if (!job)
                    return '';

                var progress = Math.round(progressBar.value / progressBar.maximumValue * 100) + '%';
                var text = '<b>' + job.fileName + '</b>';
                text += ' - ';
                text += qsTr('%1 of %2').replace('%1', progress).replace('%2', Utils.formatSize(job.fileSize));
                return text;
            }
        }
    }

    Button {
        id: openButton

        anchors.right: cancelButton.left
        anchors.verticalCenter: parent.verticalCenter
        iconSource: 'image://icon/file'
        text: qsTr('Open')
        //visible: job && job.state == QXmppTransferJob.FinishedState && job.error == QXmppTransferJob.NoError
        visible: Qt.isQtObject(job) && job.state == QXmppTransferJob.FinishedState && job.direction == QXmppTransferJob.IncomingDirection

        onClicked: Qt.openUrlExternally(job.localFileUrl)

        states: State {
            name: 'collapsed'
            when: item.width < 200
            PropertyChanges { target: openButton; width: 0; opacity: 0 }
        }
    }

    Button {
        id: cancelButton

        anchors.right: closeButton.left
        anchors.rightMargin: appStyle.margin.normal
        anchors.verticalCenter: parent.verticalCenter
        text: qsTr('Cancel')
        visible: Qt.isQtObject(job) && job.state != QXmppTransferJob.FinishedState

        onClicked: job.abort()
    }

    Button {
        id: closeButton

        anchors.right: parent.right
        anchors.rightMargin: appStyle.margin.normal
        anchors.verticalCenter: parent.verticalCenter
        text: qsTr('Close')
        visible: Qt.isQtObject(job) && job.state == QXmppTransferJob.FinishedState

        onClicked: {
            item.height = 0
            item.visible = false
        }
    }

    Connections {
        target: job
        onProgress: {
            progressBar.maximumValue = total;
            progressBar.value = done;
        }
    }
}
