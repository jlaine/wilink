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

        ScrollView {
            id: tabList

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: appStyle.icon.smallSize + 3*appStyle.margin.normal + appStyle.font.normalSize * 5
            delegate: Item {
                height: appStyle.icon.smallSize + 2*appStyle.margin.normal
                width: parent.width

                Image {
                    id: image
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.margin.normal
                    anchors.verticalCenter: parent.verticalCenter
                    smooth: true
                    source: model.iconSource
                    height: appStyle.icon.smallSize
                    width: appStyle.icon.smallSize
                }

                Label {
                    anchors.left: image.right
                    anchors.leftMargin: appStyle.spacing.horizontal
                    anchors.right: parent.right
                    anchors.rightMargin: appStyle.margin.normal
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
            model: appPreferences
        }

        PanelSwapper {
            id: prefSwapper

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: tabList.right
            anchors.leftMargin: appStyle.spacing.horizontal
            anchors.right: parent.right 
            anchors.rightMargin: appStyle.margin.small

            onCurrentIndexChanged: {
                var currentSource = prefSwapper.model.get(prefSwapper.currentIndex).source;
                for (var i = 0; i < tabList.model.count; i++) {
                    if (tabList.model.get(i).source == currentSource) {
                        tabList.currentIndex = i;
                        return;
                    }
                }
                tabList.currentIndex = -1;
            }
        }

        Component.onCompleted: {
            prefSwapper.showPanel(tabList.model.get(0).source);
        }
    }
}

