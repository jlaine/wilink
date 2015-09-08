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

    property alias contacts: sortedContacts.sourceModel
    property QtObject room
    property variant selection: []

    title: qsTr('Invite your contacts')

    Item {
        anchors.fill: parent

        InputBar {
            id: reasonEdit

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            focus: true
            text: "Let's talk"
        }

        ScrollView {
            anchors.top: reasonEdit.bottom
            anchors.topMargin: 8
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true
            highlight: Item {}
            model: sortedContacts
            delegate: Item {
                id: rect

                height: appStyle.icon.smallSize
                width: parent.width - 1

                function isSelected() {
                    for (var i = 0; i < selection.length; i += 1) {
                        if (selection[i] === model.jid)
                            return true;
                    }
                    return false;
                }

                Highlight {
                    id: highlight

                    anchors.fill: parent
                    state: 'inactive'
                }

                CheckBox {
                    id: check

                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: status.left
                    checked: {
                        for (var i = 0; i < selection.length; i += 1) {
                            if (selection[i] === model.jid)
                                return true;
                        }
                        return false;
                    }
                    iconSource: model.avatar
                    text: model.name
                }

                StatusPill {
                    id: status

                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.spacing.horizontal
                    anchors.verticalCenter: parent.verticalCenter
                    presenceStatus: model.status
                    width: 10
                    height: 10
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        var newSelection = [];
                        var wasSelected = false;
                        for (var i = 0; i < selection.length; i += 1) {
                            if (selection[i] === model.jid) {
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
    }

    resources: [
        SortFilterProxyModel {
            id: sortedContacts

            dynamicSortFilter: true
            sortCaseSensitivity: Qt.CaseInsensitive
            sortRole: appSettings.sortContactsByStatus ? RosterModel.StatusSortRole : RosterModel.NameRole
            Component.onCompleted: sort(0)
        }
    ]

    onAccepted: {
        var reason = reasonEdit.text;
        for (var i in selection) {
            console.log("inviting " + selection[i]);
            room.sendInvitation(selection[i], reason);
        }
        dialog.close();
    }

}

