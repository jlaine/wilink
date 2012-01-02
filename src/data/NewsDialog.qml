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

import QtQuick 1.1
import wiLink 2.0

Dialog {
    id: dialog

    property alias name: nameInput.text
    property alias url: urlInput.text
    property QtObject model

    minimumHeight: 150
    title: qsTr('Add a bookmark')

    Item {
        anchors.fill: contents

        Image {
            id: image

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            height: 64
            width: 64
            source: '128x128/peer.png'
        }

        Column {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: image.right
            anchors.leftMargin: 16
            anchors.right: parent.right
            spacing: appStyle.spacing.vertical

            Item {
                id: nameRow

                anchors.left: parent.left
                anchors.right: parent.right
                height: nameInput.height

                Label {
                    id: nameLabel

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    text: qsTr('Name')
                    width: 100
                }

                InputBar {
                    id: nameInput

                    anchors.left: nameLabel.right
                    anchors.right: parent.right
                    validator: RegExpValidator {
                        regExp: /.{1,30}/
                    }
                }
            }

            Item {
                id: urlRow

                anchors.left: parent.left
                anchors.right: parent.right
                height: urlInput.height

                Label {
                    id: urlLabel

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    text: qsTr('URL')
                    width: 100
                }

                InputBar {
                    id: urlInput

                    anchors.left: urlLabel.right
                    anchors.right: parent.right
                    validator: RegExpValidator {
                        regExp: /(http|https):\/\/.+/
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        nameInput.forceActiveFocus();
    }

    onAccepted: {
        if (!nameInput.acceptableInput || !urlInput.acceptableInput)
            return;

        dialog.model.addBookmark(urlInput.text, nameInput.text);
        dialog.close();
    }

    Keys.onTabPressed: {
        if (nameInput.activeFocus)
            urlInput.forceActiveFocus();
        else
            nameInput.forceActiveFocus();
    }
}

