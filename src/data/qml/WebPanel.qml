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

Panel {
    id: panel

    property variant url

    ListView {
        id: tabView

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        currentIndex: Qt.isQtObject(tabSwapper) ? tabSwapper.currentIndex : -1
        height: appStyle.icon.smallSize
        highlightMoveDuration: appStyle.highlightMoveDuration
        model: tabSwapper.model
        orientation: ListView.Horizontal
        spacing: 2

        delegate: Item {
            id: rect

            height: tabView.height
            width: 200
            z: model.panel.z

            Rectangle {
                id: frame

                anchors.fill: parent
                anchors.bottomMargin: -radius
                border.color: '#DDDDDD'
                border.width: 1
                opacity: 0
                radius: 4
                height: tabView.height

                Behavior on opacity {
                    PropertyAnimation { duration: appStyle.animation.normalDuration }
                }
            }

            Image {
                id: icon

                anchors.left: parent.left
                anchors.leftMargin: appStyle.margin.large
                anchors.verticalCenter: parent.verticalCenter
                smooth: true
                source: model.panel.iconSource
                sourceSize.height: appStyle.icon.tinySize
                sourceSize.width: appStyle.icon.tinySize
            }

            Label {
                anchors.margins: appStyle.margin.normal
                anchors.left: icon.right
                anchors.right: closeButton.left
                anchors.verticalCenter: parent.verticalCenter
                elide: Text.ElideRight
                text: model.panel.title
            }

            MouseArea {
                id: mouseArea

                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    tabSwapper.currentIndex = model.index;
                }
            }

            ToolButton {
                id: closeButton

                anchors.right: parent.right
                anchors.rightMargin: appStyle.margin.large
                anchors.verticalCenter: parent.verticalCenter
                iconStyle: 'icon-remove'
                width: iconSize
                visible: tabView.model.count > 1

                onClicked: model.panel.close()
            }

            states: State {
                name: 'current'
                when: (model.index == tabView.currentIndex)

                PropertyChanges { target: frame; opacity: 1 }
            }
        }

        footer: ToolButton {
            id: addButton

            anchors.verticalCenter: parent.verticalCenter
            iconStyle: 'icon-plus'
            width: iconSize

            onClicked: tabSwapper.addPanel('WebTab.qml', {}, true);
        }

        states: State {
            name: 'singleTab'
            when: tabView.model.count <= 1
            PropertyChanges { target: tabView; visible: false; height: 0 }
        }
    }

    PanelSwapper {
        id: tabSwapper

        anchors.top: tabView.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        focus: true
    }

    Component.onCompleted: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);

            if (account.type == 'web' && account.realm == 'www.wifirst.net') {
                var js = "var f = document.getElementsByTagName('form')[0];\n";
                js += "f.login.value = '" + account.username + "';\n";
                js += "f.password.value = '" + account.password + "';\n";
                js += "f.submit();";
                tabSwapper.addPanel('WebTab.qml', {url: 'https://apps.wifirst.net/', loadScript: js}, true)
            } else if (account.type == 'web' && account.realm == 'www.google.com') {
                var js = "var f = document.getElementById('gaia_loginform');\n";
                js += "f.Email.value = '" + account.username + "';\n";
                js += "f.Passwd.value = '" + account.password + "';\n";
                js += "f.submit();";
                tabSwapper.addPanel('WebTab.qml', {url: 'https://mail.google.com/mail/', loadScript: js}, true)
            }
        }
    }

    Keys.onPressed: {
        if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_T) {
            tabSwapper.addPanel('WebTab.qml', {}, true);
        } else if (event.modifiers == Qt.ControlModifier && event.key == Qt.Key_W) {
            if (tabSwapper.model.count > 1)
                tabSwapper.model.get(tabSwapper.currentIndex).panel.close();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Left) {
            tabSwapper.decrementCurrentIndex();
        } else if ((event.modifiers  & Qt.ControlModifier) && (event.modifiers & Qt.AltModifier) && event.key == Qt.Key_Right) {
            tabSwapper.incrementCurrentIndex();
        }
    }
}
