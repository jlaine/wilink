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

Panel {
    id: panel

    color: '#dfdfdf'

    PanelHeader {
        id: header

        property string help: qsTr('You can select the folders you want to share with other users from the shares options.')

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        icon: 'share.png'
        title: '<b>' + qsTr('Shares') + '</b>'
        z: 1

        Row {
            id: toolBar

            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right

            ToolButton {
                icon: 'options.png'
                text: qsTr('Options')

                onClicked: window.showPreferences('shares')
            }

            ToolButton {
                icon: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
            }
        }
    }

    Item {
        id: searchBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.margins: 8
        height: 32

        Image {
            id: searchIcon

            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            height: 16
            width: 16
            source: 'search.png'
            smooth: true
        }

        Text {
            id: searchLabel

            anchors.left: searchIcon.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            text: qsTr('Enter the name of the file you are looking for.');
        }

        Rectangle {
            anchors.left: searchLabel.right
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            border.color: '#c3c3c3'
            color: 'white'
            height: searchEdit.paintedHeight + 4

            TextEdit {
                id: searchEdit

                anchors.fill: parent
                anchors.margins: 2
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                focus: true
            }
        }
    }

    ShareView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom
        anchors.bottom: queueHelp.top
        anchors.bottomMargin: 8

        model: shareModel
    }

    Text {
        id: queueHelp

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        anchors.margins: 8
        height: paintedHeight

        text: qsTr('Received files are stored in your <a href="%1">downloads folder</a>. Once a file is received, you can double click to open it.')
        wrapMode: Text.WordWrap
    }

    ShareView {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: model.count * 30

        model: ListModel {
            ListElement { name: 'Foo Bar.png'; size: 12434 }
            ListElement { name: 'Foo Bar.png'; size: 45567437 }
            ListElement { name: 'Foo Bar.png'; size: 87654724 }
            ListElement { name: 'Foo Bar.png'; size: 573474 }
        }
        z: 1
    }
}
