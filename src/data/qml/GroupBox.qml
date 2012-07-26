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

FocusScope {
    id: box

    default property alias content: item.children
    property color borderColor: '#DDDDDD'
    property color headerColor: '#F5F5F5'
    property Component footerComponent
    property color footerColor: '#F5F5F5'
    property Component headerComponent: boxHeader
    property int radius: appStyle.margin.small
    property real shadowOpacity: 0
    property string title

    Component {
        id: boxHeader

        Item {
            anchors.fill: parent

            Label {
                id: textItem

                anchors.centerIn: parent
                font.bold: true
                text: box.title
            }
        }
    }

    // FIXME: this is a hack waiting 'blur' or 'shadow' attribute in qml
    BorderImage {
        id: shadow

        anchors.fill: box
        anchors { leftMargin: -5; topMargin: -5; rightMargin: -8; bottomMargin: -9 }
        border { left: 10; top: 10; right: 10; bottom: 10 }
        opacity: shadowOpacity
        source: 'image://icon/shadow'
        smooth: true
        z: -1
    }

    Rectangle {
        id: background

        anchors.fill: parent
        border.color: box.borderColor
        border.width: 1
        color: 'white'
        radius: box.radius
        smooth: true
    }

    Rectangle {
        id: frame

        anchors.fill: parent
        border.color: box.borderColor
        border.width: 1
        color: 'transparent'
        radius: box.radius
        smooth: true
        z: 1
    }

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: appStyle.font.largeSize + 2 * appStyle.margin.large

        Rectangle {
            anchors.fill: parent
            color: headerColor
            radius: box.radius
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1
            color: headerColor
            height: box.radius
        }

        Rectangle {
            id: headerBorder
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: '#DDDDDD'
        }

        Loader {
            anchors.fill: parent
            anchors.margins: appStyle.margin.large
            sourceComponent: headerComponent
        }
    }

    Item {
        id: item

        anchors.top: header.bottom
        anchors.bottom: footer.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: appStyle.margin.normal
    }

    Item {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: footerComponent ? 45 : 0
        visible: footerComponent ? true : false

        Rectangle {
            anchors.fill: parent
            color: footerColor
            radius: box.radius
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 1
            color: footerColor
            height: box.radius
        }

        Rectangle {
            id: footerBorder
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: '#DDDDDD'
        }

        Loader {
            anchors.fill: parent
            anchors.margins: appStyle.margin.large
            sourceComponent: footerComponent
        }
    }
}
