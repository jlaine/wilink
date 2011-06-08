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

Dialog {
    id: dialog

    property alias reason: reasonEdit.text
    property QtObject room
    property variant selection: []

    minWidth: 200
    minHeight: 150
    title: qsTr('Invite your contacts')

    Rectangle {
        id: bar

        anchors.margins: 8
        anchors.top: contents.top
        anchors.left: parent.left
        anchors.right: parent.right
        border.color: '#c3c3c3'
        border.width: 1
        color: 'white'
        width: 100
        height: reasonEdit.paintedHeight + 8

        TextEdit {
            id: reasonEdit

            anchors.fill: parent
            anchors.margins: 4
            focus: true
            smooth: true
            text: "Let's talk"
            textFormat: TextEdit.PlainText

            Keys.onReturnPressed: {
                return false;
            }
        }
    }

    ListView {
        id: view

        anchors.topMargin: 8
        anchors.leftMargin: 8
        anchors.bottomMargin: 8
        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: bar.bottom
        anchors.bottom: contents.bottom
        clip: true
        model: SortFilterProxyModel {
            dynamicSortFilter: true
            filterKeyColumn: 1
            filterRegExp: /^(?!offline).+/
            sortCaseSensitivity: Qt.CaseInsensitive
            sourceModel: window.rosterModel
            Component.onCompleted: sort(0)
        }
        delegate: Item {
            id: rect

            height: 24
            width: parent.width - 1

            function isSelected() {
                for (var i = 0; i < selection.length; i += 1) {
                    if (selection[i] == model.jid)
                        return true;
                }
                return false;
            }

            Highlight {
                id: highlight

                anchors.fill: parent
                state: 'inactive'
            }

            Rectangle {
                id: check

                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                border.width: 1
                border.color: '#ffb0c4de'
                color: isSelected() ? '#597fbe' : 'white'
                radius: 6
                width: 12
                height: 12
            }

            Image {
                id: image

                anchors.left: check.right
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                smooth: true
                source: model.avatar
            }

            Text {
                anchors.left: image.right
                anchors.leftMargin: 4
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: model.name
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    var newSelection = [];
                    var wasSelected = false;
                    for (var i = 0; i < selection.length; i += 1) {
                        if (selection[i] == model.jid) {
                            wasSelected = true;
                        } else {
                            newSelection[newSelection.length] = selection[i];
                        }
                    }
                    if (!wasSelected)
                        newSelection[newSelection.length] = model.jid;
                    dialog.selection = newSelection;
                }
                onEntered: highlight.state = ''
                onExited: highlight.state = 'inactive'
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.topMargin: 8
        anchors.bottomMargin: 8
        anchors.rightMargin: 8
        anchors.right: parent.right
        anchors.top: bar.bottom
        anchors.bottom: contents.bottom
        clip: true
        flickableItem: view
    }

    onAccepted: {
        for (var i in selection) {
            console.log("inviting " + selection[i]);
            room.sendInvitation(selection[i], reason);
        }
        selection = [];
        hide()
    }

}

