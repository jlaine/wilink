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
import wiLink 2.4

Dialog {
    id: dialog

    property url editUrl
    property alias name: nameInput.text
    property alias url: urlInput.text
    property QtObject model

    property string addTitle: qsTr('Add a bookmark')
    property string editTitle: qsTr('Edit bookmark')

    minimumHeight: 150
    title: addTitle

    Item {
        anchors.fill: parent

        Image {
            id: image

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            source: 'image://icon/peer'
            sourceSize.height: 64
            sourceSize.width: 64
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
                    focus: true
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

    onAccepted: {
        if (!nameInput.acceptableInput || !urlInput.acceptableInput)
            return;

        dialog.model.addBookmark(urlInput.text, nameInput.text, dialog.editUrl);
        dialog.close();
    }

    Keys.onTabPressed: {
        if (nameInput.activeFocus)
            urlInput.forceActiveFocus();
        else
            nameInput.forceActiveFocus();
    }

    states: State {
        name: 'edit'
        when: dialog.editUrl

        PropertyChanges { target: dialog; title: dialog.editTitle }
    }
}

