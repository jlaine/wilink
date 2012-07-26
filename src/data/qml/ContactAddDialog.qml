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
import 'utils.js' as Utils

Dialog {
    id: dialog

    property string accountJid

    title: qsTr('Add a contact')

    Column {
        anchors.fill: contents
        spacing: 8

        PanelHelp {
            id: help

            anchors.left: parent.left
            anchors.right: parent.right
            iconSize: 32
            text: qsTr('Enter the address of the contact you want to add.')
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: accountCombo.height

            Label {
                id: accountLabel

                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr('Account')
                width: 96
            }

            ComboBox {
                id: accountCombo

                anchors.left: accountLabel.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                model: ListModel {}

                onCurrentIndexChanged: {
                    var wasDefault = (bar.text == bar.defaultText);
                    accountJid = model.get(currentIndex).text;
                    bar.defaultText = '@' + Utils.jidToDomain(accountJid);
                    if (wasDefault) {
                        bar.text = bar.defaultText;
                        bar.cursorPosition = 0;
                    }
                }

                Component.onCompleted: {
                    for (var i = 0; i < accountModel.count; ++i) {
                        var account = accountModel.get(i);
                        if (account.type == 'xmpp')
                            model.append({iconSource: 'image://icon/chat', text: account.username});
                    }
                    accountCombo.currentIndex = 0;
                }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            height: bar.height

            Label {
                id: contactLabel

                anchors.left:  parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr('Contact')
                width: 96
            }

            InputBar {
                id: bar

                property string defaultText

                anchors.left: contactLabel.right
                anchors.leftMargin: appStyle.spacing.horizontal
                anchors.right: parent.right
                focus: true
                validator: RegExpValidator {
                    regExp: /^[^@/ ]+@[^@/ ]+$/
                }
            }
        }
    }

    onAccepted: {
        if (!bar.acceptableInput)
            return;

        var client = accountModel.clientForJid(accountJid);
        var jid = bar.text;
        console.log("Add contact " + jid + " to account " + accountJid);
        client.rosterManager.subscribe(jid);
        dialog.close();
    }
}

