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

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'share.png'
        title: qsTr('Shares')
        toolBar: ToolBar {
            ToolButton {
                iconSource: 'back.png'
                text: qsTr('Go back')
                enabled: crumbBar.model.count > 1

                onClicked: crumbBar.pop()
            }

            ToolButton {
                id: preferenceButton

                iconSource: 'options.png'
                text: qsTr('Options')

                onClicked: {
                    dialogSwapper.showPanel('PreferenceDialog.qml');
                    var panel = dialogSwapper.findPanel('PreferenceDialog.qml');
                    panel.showPanel('SharePreferencePanel.qml');
                }

                // run one-time configuration dialog
                Component.onCompleted: {
                    if (!application.settings.sharesConfigured && !application.isMobile) {
                        preferenceButton.clicked();
                        application.settings.sharesConfigured = true;
                    }
                }
            }
        }
    }

    PanelHelp {
        id: help

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        text: qsTr('You can select the folders you want to share with other users from the shares options.')
        z: 1
    }

    Rectangle {
        id: searchBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: help.bottom
        color: '#e1eafa'
        height: 32
        z: 1

        Image {
            id: searchIcon

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: appStyle.icon.tinySize
            width: appStyle.icon.tinySize
            source: 'search.png'
            smooth: true
        }

        InputBar {
            id: searchEdit

            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            hintText: qsTr('Enter the name of the file you are looking for.')
            onTextChanged: searchTimer.restart()

            Timer {
                id: searchTimer
                interval: 500
                onTriggered: view.model.filter = searchEdit.text
            }
        }
    }

    CrumbBar {
        id: crumbBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom

        onLocationChanged: {
            view.model.rootUrl = location.url;
        }
        z: 1
    }

    ShareModel {
        client: appClient

        onConnectedChanged: {
            if (connected) {
                // update breadcrumbs
                if (!crumbBar.model.count) {
                    crumbBar.push({'name': qsTr('Home'), 'url': 'share:'});
                } else {
                    crumbBar.model.setProperty(0, 'url', 'share:');
                    refresh();
                }

                // show dock icon
                for (var i = 0; i < dock.model.count; i++) {
                    if (dock.model.get(i).panelSource == 'SharePanel.qml') {
                        dock.model.setProperty(i, 'visible', true);
                        break;
                    }
                }
            }
        }
    }

    ShareView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: queueHelp.top

        model: FolderModel {
            onBusyChanged: {
                if (view.count == 0 && !busy && searchEdit.text.length > 0)
                    searchEdit.state = 'error';
                else
                    searchEdit.state = '';
            }
        }
    }

    PanelHelp {
        id: queueHelp

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: queueView.top
        text: qsTr('Received files are stored in your <a href="%1">downloads folder</a>.').replace('%1', view.model.shareUrl)
        z: 1
    }

    FolderQueueView {
        id: queueView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        height: queueView.count > 0 ? 120 : 0
        model: view.model.uploads
        style: 'shares'
    }
}
