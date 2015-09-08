/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import wiLink 2.4

Dialog {
    id: dialog

    property int contactId: -1
    property alias contactName: nameInput.text
    property alias contactPhone: phoneInput.text
    property QtObject model

    minimumHeight: 150
    title: contactId >= 0 ? qsTr('Modify a contact') : qsTr('Add a contact')

    Item {
        anchors.fill: parent

        Image {
            id: image

            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            source: 'image://icon/peer'
            width: 64
            height: 64
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
                id: phoneRow

                anchors.left: parent.left
                anchors.right: parent.right
                height: phoneInput.height

                Label {
                    id: phoneLabel

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    text: qsTr('Number')
                    width: 100
                }

                InputBar {
                    id: phoneInput

                    anchors.left: phoneLabel.right
                    anchors.right: parent.right
                    validator: RegExpValidator {
                        regExp: /.{1,30}/
                    }
                }
            }
        }
    }

    onAccepted: {
        if (!nameInput.acceptableInput || !phoneInput.acceptableInput)
            return;

        if (dialog.contactId >= 0) {
            dialog.model.updateItem(dialog.contactId, {name: nameInput.text, phone: phoneInput.text});
        } else {
            dialog.model.addItem({name: nameInput.text, phone: phoneInput.text});
        }
        dialog.close();
    }

    Keys.onTabPressed: {
        if (nameInput.activeFocus)
            phoneInput.forceActiveFocus();
        else
            nameInput.forceActiveFocus();
    }
}

