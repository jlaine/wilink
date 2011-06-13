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

    PanelHeader {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        iconSource: 'share.png'
        title: '<b>' + qsTr('Shares') + '</b>'
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                iconSource: 'back.png'
                text: qsTr('Go back')
                enabled: crumbBar.model.count > 1

                onClicked: crumbBar.pop()
            }

            ToolButton {
                iconSource: 'options.png'
                text: qsTr('Options')

                onClicked: window.showPreferences('shares')
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
            height: 16
            width: 16
            source: 'search.png'
            smooth: true
        }

        Rectangle {
            id: searchRect

            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            border.color: '#c3c3c3'
            color: '#ffffff'
            height: 22

            TextEdit {
                id: searchEdit

                anchors.fill: parent
                anchors.margins: 2
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                focus: true
                text: ''

                Keys.onReturnPressed: {
                    // prevent new line
                }
            }

            Text {
                id: searchLabel

                anchors.fill: searchEdit
                color: '#999999'
                opacity: searchEdit.text == '' ? 1 : 0
                text: qsTr('Enter the name of the file you are looking for.');
            }

            states: State {
                name: 'noresults'
                PropertyChanges { target: searchRect; color: '#ffaaaa' }
            }
        }
    }

    CrumbBar {
        id: crumbBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom

        onLocationChanged: {
            view.model.rootJid = location.jid;
            view.model.rootNode = location.node;
        }
        z: 1
    }

    ShareView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: queueHelp.top

        model: ShareModel {
            client: window.client
            filter: searchEdit.text

            onShareServerChanged: {
                if (!crumbBar.model.count) {
                    crumbBar.push({'name': qsTr('Home'), 'jid': view.model.shareServer, 'node': ''});
                }
            }
        }

        onCountChanged: {
            if (count == 0 && searchEdit.text.length > 0)
                searchRect.state = 'noresults';
            else
                searchRect.state = '';
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

    ShareQueueView {
        id: queueView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: queueView.count > 0 ? 120 : 0
        model: view.model.queue
    }
}
