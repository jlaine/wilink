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
import 'scripts/utils.js' as Utils

Item {
    id: button

    property bool active
    property bool enabled: true
    property bool notified: false
    property string iconStyle: ''
    property variant panelProperties
    property string panelSource: ''
    property string text
    property int shortcut: 0
    signal clicked

    active: {
        var item;
        if (dialogSwapper.currentIndex >= 0) {
            item = dialogSwapper.model.get(dialogSwapper.currentIndex);
        } else {
            item = swapper.model.get(swapper.currentIndex);
        }

        return item !== undefined &&
               item.source == panelSource &&
               Utils.matchProperties(item.properties, panelProperties);
    }

    height: appStyle.icon.normalSize
    width: appStyle.icon.normalSize
    state: mouseArea.pressed ? 'pressed' : (mouseArea.hovered ? 'hovered' : '')

    Icon {
        id: icon

        anchors.centerIn: parent
        style: iconStyle
        color: button.active ? 'white' : '#d6dae0'
        font.pixelSize: appStyle.icon.smallSize
    }

    Rectangle {
        width: 6
        height: 4
        radius: 2
        anchors.horizontalCenter: icon.horizontalCenter
        anchors.top: icon.bottom
        color: 'white'
        opacity: button.active ? 0.9 : 0.0
    }

    Rectangle {
        id: labelBackground

        anchors.left: button.right
        anchors.leftMargin: 20
        anchors.verticalCenter: button.verticalCenter
        color: '#333'
        opacity: 0
        radius:5
        width: label.width + 2*appStyle.spacing.horizontal
        height: label.height +2*appStyle.spacing.vertical
    }

    Label {
        id: label

        function shortcutText(shortcut) {
            var text = '';
            if (shortcut & Qt.ControlModifier)
                text += appSettings.osType == 'mac' ? 'Cmd-' : 'Ctrl-';
            var key = shortcut & 0xffffff;
            if (key >= Qt.Key_A && key <= Qt.Key_Z) {
                var alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
                text += alpha[key - Qt.Key_A];
            }
            if (text.length)
                return '<br/>' + text;
            else
                return '';
        }

        anchors.verticalCenter: labelBackground.verticalCenter
        anchors.centerIn: labelBackground
        font.pixelSize: appStyle.font.smallSize
        opacity: 0
        text: button.text
        color: 'white'
    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges { target: icon; anchors.horizontalCenterOffset: button.active ? 0 : -1 }
            PropertyChanges { target: icon; anchors.verticalCenterOffset: button.active ? 0 : -1 }
            PropertyChanges { target: label; opacity: 1 }
            PropertyChanges { target: labelBackground; opacity: 0.7 }
        }
        //State {
            //name: 'pressed'
            //PropertyChanges { target: button; width: image.width + 16; height: image.height }
            //PropertyChanges { target: image; height: appStyle.icon.normalSize; width: appStyle.icon.normalSize }
            //PropertyChanges { target: label; opacity: 0.75 }
            //PropertyChanges { target: labelBackground; opacity: 1 }
        //}
    ]

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        property bool pressed: false
        property bool hovered: false

        onClicked: {
            if (button.enabled) {
                button.clicked();
            }
        }
        onPressed: {
            if (button.enabled) {
                pressed = true;
            }
        }
        onReleased: pressed = false
        onEntered: {
            if (button.enabled) {
                hovered = true;
            }
        }
        onExited: hovered = false
    }

    SequentialAnimation on x {
        alwaysRunToEnd: true
        loops: Animation.Infinite
        running: notified && button.state == ''

        PropertyAnimation {
            duration: 100
            easing.type: Easing.OutExpo
            to: 32
        }
        PropertyAnimation {
            duration: 1200
            easing.type: Easing.OutBounce
            to: 0
        }
        PropertyAnimation {
            duration: 1000
            to: 0
        }
    }
}
