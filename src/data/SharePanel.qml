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
                enabled: crumbs.count > 0

                onClicked: {
                    var crumb = crumbs.get(crumbs.count - 1);
                    view.model.rootJid = crumb.jid;
                    view.model.rootNode = crumb.node;
                    view.model.rootName = crumb.name;
                    crumbs.remove(crumbs.count - 1);
                }
            }

            ToolButton {
                iconSource: 'options.png'
                text: qsTr('Options')

                onClicked: window.showPreferences('shares')
            }

            ToolButton {
                iconSource: 'close.png'
                text: qsTr('Close')
                onClicked: panel.close()
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
            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 8
            border.color: '#c3c3c3'
            color: '#aaffffff'
            height: 22

            TextEdit {
                id: searchEdit

                anchors.fill: parent
                anchors.margins: 2
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                focus: true
                text: ''

                Timer {
                    id: searchTimer
                }
            }

            Text {
                id: searchLabel

                anchors.fill: searchEdit
                color: '#999999'
                opacity: searchEdit.text == '' ? 1 : 0
                text: qsTr('Enter the name of the file you are looking for.');
            }
        }
    }

    Item {
        id: crumbBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: searchBar.bottom
        height: 24

        ListView {
            id: crumbView

            anchors.left: parent.left
            anchors.top:  parent.top
            anchors.bottom:  parent.bottom
            orientation: Qt.Horizontal
            width: model.count * 70

            model: ListModel {
                id: crumbs
            }

            delegate: Item {
                height: crumbView.height
                width: 70

                Text {
                    anchors.fill:  parent
                    elide:  Text.ElideMiddle
                    text: model.name + " >"
                }

                MouseArea {
                    anchors.fill:  parent
                    onClicked: {
                        console.log("clicked: " + model.name);
                        var row = -1;
                        for (var i = 0; i < crumbs.count; i++) {
                            var obj = crumbs.get(i);
                            if (obj.jid == model.jid && obj.node == model.node) {
                                row = i;
                                break;
                            }
                        }
                        if (row < 0) {
                            console.log("unknown row clicked");
                            return;
                        }

                        // navigate to the specified location
                        view.model.rootJid = model.jid;
                        view.model.rootNode = model.node;
                        view.model.rootName = model.name;

                        // remove crumbs below current location
                        for (var i = crumbs.count - 1; i >= row; i--) {
                            crumbs.remove(i);
                        }
                    }
                }
            }
        }

        Text {
            anchors.left: crumbView.right
            anchors.top:  parent.top
            anchors.bottom:  parent.bottom

            text: view.model.rootName
            width: 40
        }
    }

    ShareView {
        id: view

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: crumbBar.bottom
        anchors.bottom: queueHelp.top

        model: ShareModel {
            property string rootName: qsTr('Home')

            client: window.client            
        }
    }

    PanelHelp {
        id: queueHelp

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: qsTr('Received files are stored in your <a href="%1">downloads folder</a>. Once a file is received, you can double click to open it.')
        z: 1
    }

/*
    ShareView {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: model.count * 30
        model: ShareModel {}
        z: 1
    }
*/
}
