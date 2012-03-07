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

Dialog {
    id: dialog

    property QtObject appUpdater: application.updater
    property bool prompting: Qt.isQtObject(appUpdater) && appUpdater.state == Updater.PromptState

    footerComponent: Item {
        anchors.fill: parent

        Row {
            anchors.centerIn: parent
            spacing: appStyle.margin.large

            Button {
                id: refreshButton

                enabled: Qt.isQtObject(appUpdater) && appUpdater.state == Updater.IdleState
                iconSource: 'refresh.png'
                text: qsTr('Check for updates')
                visible: !prompting
                onClicked: appUpdater.check()
            }

            Button {
                id: installButton

                iconSource: 'start.png'
                text: qsTr('Install')
                visible: prompting
                onClicked: appUpdater.install()
            }

            Button {
                id: rejectButton

                iconSource: 'close.png'
                text: prompting ? qsTr('Cancel') : qsTr('Close')
                onClicked: {
                    if (prompting)
                        appUpdater.refuse();
                    dialog.rejected();
                }
            }

            Keys.onPressed: {
                if (event.key == Qt.Key_Enter ||
                    event.key == Qt.Key_Return) {
                    if (installButton.visible && installButton.enabled)
                        installButton.clicked();
                    else if (refreshButton.visible && refreshButton.enabled)
                        refreshButton.clicked();
                }
                else if (event.key == Qt.Key_Escape) {
                    rejectButton.clicked()
                }
            }
        }
    }

    minimumHeight: 280
    title: qsTr('About %1').replace('%1', application.applicationName)

    Item {
        anchors.fill: dialog.contents

        Item {
            id: appBanner

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            height: 48
            width: 48 + appVersion.width

            Image {
                id: appIcon
                anchors.top: parent.top
                anchors.left: parent.left
                height: 48
                width: 48
                smooth: true
                source: '128x128/wiLink.png'
            }

            Label {
                id: appName

                anchors.top: parent.top
                anchors.left: appIcon.right
                anchors.leftMargin: appStyle.spacing.horizontal
                font.bold: true
                font.pixelSize: appStyle.font.largeSize
                text: application.applicationName
            }

            Label {
                id: appVersion

                anchors.left: appIcon.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.top: appName.bottom
                anchors.topMargin: appStyle.spacing.vertical
                text: qsTr('version %1').replace('%1', application.applicationVersion)
            }
        }

        PanelHelp {
            id: prompt
            anchors.top: appBanner.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            text: {
                if (!Qt.isQtObject(appUpdater))
                    return '';

                switch (appUpdater.state) {
                case Updater.IdleState: {
                    if (appUpdater.error == Updater.NoError) {
                        return "<p>Copyright (C) 2009-2012 Wifirst<br/>"
                        + "See AUTHORS file for a full list of contributors.</p>"
                        + "<p>This program is free software: you can redistribute it and/or modify "
                        + "it under the terms of the GNU General Public License as published by "
                        + "the Free Software Foundation, either version 3 of the License, or "
                        + "(at your option) any later version.</p>";
                    } else if (appUpdater.error == Updater.NoUpdateError) {
                        return qsTr('Your version of %1 is up to date.').replace('%1', application.applicationName);
                    } else {
                        var text = '<p>' + qsTr('Could not run software update, please try again later.') + '</p>'
                        text += '<pre>' + appUpdater.errorString + '</pre>';
                        return text;
                    }
                }
                case Updater.CheckState:
                    return qsTr('Checking for updates..');
                case Updater.DownloadState:
                    return qsTr('Downloading update..');
                case Updater.PromptState: {
                    var text = "<p>" + qsTr('Version %1 of %2 is ready to be installed.')
                                .replace('%1', appUpdater.updateVersion)
                                .replace('%2', application.applicationName) + "</p>";
                    text += "<p><b>" + qsTr('Changes') + "</b></p>";
                    text += "<pre>" + appUpdater.updateChanges + "</pre>";
                    return text;
                }
                case Updater.InstallState:
                    return qsTr('Installing update..');
                }
            }
        }

        ProgressBar {
            anchors.top: prompt.bottom
            anchors.topMargin: appStyle.spacing.vertical
            anchors.left: parent.left
            anchors.right: parent.right
            maximumValue: Qt.isQtObject(appUpdater) ? appUpdater.progressMaximum : 100
            value: Qt.isQtObject(appUpdater) ? appUpdater.progressValue : 0
            visible: Qt.isQtObject(appUpdater) && appUpdater.state == Updater.DownloadState
        }
    }
}

