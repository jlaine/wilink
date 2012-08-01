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
               (panelProperties === undefined || Utils.equalProperties(item.properties, panelProperties));
    }

    height: appStyle.icon.normalSize
    width: appStyle.icon.normalSize
    state: mouseArea.pressed ? 'pressed' : (mouseArea.hovered ? 'hovered' : '')

    Icon {
        id: icon

        anchors.centerIn: parent
        color: button.active ? '#597fbe' : '#e1e5eb'
        font.pixelSize: 24
        style: iconStyle
    }

    Rectangle {
        id: labelBackground

        anchors.left: parent.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        opacity: 0
        color: '#597fbe'
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

        anchors.left: parent.right
        anchors.leftMargin: 8 + appStyle.spacing.horizontal
        anchors.verticalCenter: parent.verticalCenter
        opacity: 0
        color: 'white'
        font.pixelSize: appStyle.font.smallSize
        text: '<b>' + button.text + '</b>' + shortcutText(button.shortcut)
    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges { target: label; opacity: 1 }
            PropertyChanges { target: labelBackground; opacity: 1 }
        },
        State {
            name: 'pressed'
            PropertyChanges { target: label; opacity: 0.5 }
            PropertyChanges { target: labelBackground; opacity: 1 }
        }
    ]

    transitions: Transition {
            from: ''
            to: 'hovered'
            reversible: true
            PropertyAnimation { target: label; properties: 'opacity'; duration: appStyle.animation.normalDuration }
            PropertyAnimation { target: labelBackground; properties: 'opacity'; duration: appStyle.animation.normalDuration }
        }

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
