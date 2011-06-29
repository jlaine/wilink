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

Dialog {
    id: dialog

    minimumHeight: 500
    minimumWidth: 360
    title: qsTr("Preferences")

    function showPanel(source) {
        prefSwapper.showPanel(source);
    }

    onAccepted: {
        for (var i = tabList.count - 1; i >= 0; i--) {
            var panel = prefSwapper.findPanel(tabList.model.get(i).source);
            if (panel) {
                console.log("Saving " + panel);
                panel.save()
            }
        }
        dialog.close();
    }

    Item {
        anchors.fill: contents

        ListView {
            id: tabList

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            interactive: false
            width: 100

            model: ListModel {
                ListElement {
                    avatar: 'options.png'
                    name: 'General'
                    source: 'GeneralPreferencePanel.qml'
                }
                ListElement {
                    avatar: 'peer.png'
                    name: 'Accounts'
                    source: 'AccountPreferencePanel.qml'
                }
                ListElement {
                    avatar: 'audio-output.png'
                    name: 'Sound'
                    source: 'SoundPreferencePanel.qml'
                }
                ListElement {
                    avatar: 'share.png'
                    name: 'Shares'
                    source: 'SharePreferencePanel.qml'
                }

                Component.onCompleted: {
                    setProperty(0, 'name', qsTr('General'));
                    setProperty(1, 'name', qsTr('Accounts'));
                    setProperty(2, 'name', qsTr('Sound'));
                    setProperty(3, 'name', qsTr('Shares'));
                }
            }

            delegate: Item {
                height: 32
                width: tabList.width - 1

                Image {
                    id: image
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    smooth: true
                    source: model.avatar
                    height: appStyle.icon.smallSize
                    width: appStyle.icon.smallSize
                }

                Text {
                    anchors.left: image.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    text: model.name
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton

                    onClicked: {
                        prefSwapper.showPanel(model.source);
                    }
                }
            }

            highlight: Highlight {}

            Component.onCompleted: prefSwapper.showPanel(model.get(0).source)
        }

        PanelSwapper {
            id: prefSwapper

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: tabList.right
            anchors.leftMargin: 8
            anchors.right: parent.right 

            onCurrentSourceChanged: {
                for (var i = 0; i < tabList.model.count; i++) {
                    if (tabList.model.get(i).source == prefSwapper.currentSource) {
                        tabList.currentIndex = i;
                        return;
                    }
                }
                tabList.currentIndex = -1;
            }
        }
    }
}

