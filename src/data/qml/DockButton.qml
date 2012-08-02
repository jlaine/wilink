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
    property string iconSource: ''
    property string iconPress: ''
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
        style: iconStyle
        size: 24
        /* Sparrow */
        //color: button.active ? '#2d8ffe' : '#e1e5eb'
        //color: button.active ? 'white' : '#e1e5eb'

        /* Wifirst */
        //color: button.active ? '#597fbe' : '#e1e5eb'
        color: button.active ? 'white' : '#d6dae0'

        styleColor: '#969ea7'
        textStyle: Text.Normal
    }

    Rectangle {
        width: 6
        height: 4
        radius: 2
        anchors.horizontalCenter: icon.horizontalCenter
        anchors.top: icon.bottom
        //anchors.topMargin: 40
        color: 'white'
        opacity: button.active ? 0.9 : 0.0
    }

//    Image {
//        id: image

//        anchors.top: parent.top
//        anchors.horizontalCenter: parent.horizontalCenter
//        opacity: button.enabled ? 1 : 0.5
//        smooth: true
//        source: (button.iconPress != '' && button.active) ? button.iconPress : button.iconSource
//        sourceSize.height: appStyle.icon.normalSize
//        sourceSize.width: appStyle.icon.normalSize
//        width: parent.width
//        height: parent.height
//    }

    Rectangle {
        id: labelBackground

        anchors.left: button.right
        anchors.leftMargin: 20
        anchors.verticalCenter: button.verticalCenter
        opacity: 0
        radius:5
        /* Wifirst */
        //color: '#597fbe'

        /* Sparrow */
        color: '#333'
        
        width: label.width + 2*appStyle.spacing.horizontal
        height: label.height +2*appStyle.spacing.vertical
    }

    Label {
        id: label

        anchors.verticalCenter: labelBackground.verticalCenter
        anchors.centerIn: labelBackground
        font.pixelSize: 12
        opacity: 0
        text: button.text
        color: 'white'
    }
//    Label {
//        id: label

//        function shortcutText(shortcut) {
//            var text = '';
//            if (shortcut & Qt.ControlModifier)
//                text += appSettings.osType == 'mac' ? 'Cmd-' : 'Ctrl-';
//            var key = shortcut & 0xffffff;
//            if (key >= Qt.Key_A && key <= Qt.Key_Z) {
//                var alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
//                text += alpha[key - Qt.Key_A];
//            }
//            if (text.length)
//                return '<br/>' + text;
//            else
//                return '';
//        }

//        anchors.left: image.right
//        anchors.leftMargin: appStyle.spacing.horizontal
//        anchors.verticalCenter: image.verticalCenter
//        opacity: 0
//        color: 'white'
//        font.pixelSize: appStyle.font.smallSize
//        text: '<b>' + button.text + '</b>' + shortcutText(button.shortcut)
//    }

    states: [
        State {
            name: 'hovered'
            PropertyChanges   { target: icon; textStyle: button.active ? Text.Normal : Text.Sunken }
            //PropertyChanges { target: button; width: image.width + 16; height: image.height }
            //PropertyChanges { target: image; height: appStyle.icon.normalSize; width: appStyle.icon.normalSize }
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

//    transitions: Transition {
//            from: ''
//            to: 'hovered'
//            reversible: true
//            PropertyAnimation { target: button; properties: 'height,width'; duration: appStyle.animation.normalDuration }
//            PropertyAnimation { target: image; properties: 'height,width'; duration: appStyle.animation.normalDuration }
//            PropertyAnimation { target: label; properties: 'opacity'; duration: appStyle.animation.normalDuration }
//            PropertyAnimation { target: labelBackground; properties: 'opacity'; duration: appStyle.animation.normalDuration }
//        }

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
