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
    property Component footerComponent
    property color footerColor: '#F5F5F5'
    property Component headerComponent: boxHeader
    property bool headerBackground: true
    property int radius: appStyle.margin.small
    property real shadowOpacity: 0
    property string title

    Component {
        id: boxHeader

        Item {
            anchors.fill: parent

            Label {
                id: textItem

                anchors.fill: parent
                color: '#858585'
                font.bold: true
                font.capitalization: Font.AllUppercase
                style:Text.Sunken
                horizontalAlignment: Text.AlignLeft
                text: box.title

            }
        }
    }

    Item {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: headerComponent ? (appStyle.font.largeSize + 2 * appStyle.margin.large) : 0
        visible: headerComponent ? true : false

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
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: '#DDDDDD'
        }

        Rectangle {
            anchors.top: footerBorder.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: 'white'
        }

        Loader {
            anchors.fill: parent
            anchors.margins: appStyle.margin.large
            sourceComponent: footerComponent
        }
    }
}
