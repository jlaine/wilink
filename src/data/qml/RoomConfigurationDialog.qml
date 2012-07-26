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

    property alias room: configurationModel.room

    title: qsTr('Chat room configuration')
    minimumWidth: 300
    minimumHeight: 220

    RoomConfigurationModel {
        id: configurationModel
    }

    ScrollView {
        id: view

        anchors.fill: parent
        clip: true
        model: configurationModel
        delegate: Item {
            id: item

            width: parent.width
            height: model.type == QXmppDataForm.HiddenField ? 0 : appStyle.icon.smallSize

            Item {
                anchors.fill: parent
                visible: model.type == QXmppDataForm.TextSingleField

                Label {
                    id: label

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    text: model.label
                }

                InputBar {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: label.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.spacing.horizontal
                    text: model.value

                    onTextChanged: {
                        if (model.type == QXmppDataForm.TextSingleField) {
                            configurationModel.setValue(model.index, text);
                        }
                    }
                }
            }

            CheckBox {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                checked: model.value == true
                text: model.label
                visible: model.type == QXmppDataForm.BooleanField

                onClicked: configurationModel.setValue(model.index, !checked)
            }
        }
    }

    onAccepted: {
        configurationModel.submit();
        dialog.close();
    }
}
