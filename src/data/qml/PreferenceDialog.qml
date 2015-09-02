/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

Dialog {
    id: dialog

    property string initialPanel: 'GeneralPreferencePanel.qml'

    minimumHeight: 500
    title: qsTr("Preferences")

    onAccepted: {
        var reloadWindow = false;

        for (var i = tabList.count - 1; i >= 0; i--) {
            var source = tabList.model.get(i).source;
            var panel = prefSwapper.findPanel(source);
            if (panel) {
                console.log("Saving " + panel);
                panel.save()
                if (source == 'AccountPreferencePanel.qml' && panel.changed) {
                    reloadWindow = true;
                }
            }
        }

        if (reloadWindow)
            window.reload();

        dialog.close();
    }

    Item {
        anchors.fill: parent

        ScrollView {
            id: tabList

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: appStyle.icon.smallSize + 3*appStyle.margin.normal + appStyle.font.normalSize * 5
            delegate: Item {
                height: appStyle.icon.smallSize + 2*appStyle.margin.normal
                width: parent.width

                Icon {
                    id: image
                    anchors.left: parent.left
                    anchors.leftMargin: appStyle.margin.normal
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: appStyle.icon.smallSize
                    style: model.iconStyle
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
            model: ListModel {}
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
            tabList.model.append({
                iconStyle: 'icon-home',
                name: qsTr('General'),
                source: 'GeneralPreferencePanel.qml'});
            tabList.model.append({
                iconStyle: 'icon-user',
                name: qsTr('Accounts'),
                source: 'AccountPreferencePanel.qml'});
            tabList.model.append({
                iconStyle: 'icon-music',
                name: qsTr('Sound'),
                source: 'SoundPreferencePanel.qml'});
            tabList.model.append({
                iconStyle: 'icon-cog',
                name: qsTr('Plugins'),
                source: 'PluginPreferencePanel.qml'});
            prefSwapper.showPanel(initialPanel);
        }
    }
}

