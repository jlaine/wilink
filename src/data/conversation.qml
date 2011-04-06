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

Rectangle {
    width: 320
    height: 400
    anchors.topMargin: 10

/*
    ListModel {
        id: historyModel

        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Hello</p><p>How are you?</p>"
            received: true
        }
        ListElement {
            avatar: "home.png"
            date: ""
            from: "Me"
            html: "<p>Just fine, how about yourself?</p><p>How are you?</p>"
            received: false
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Where did you spend your holidays?</p>"
            received: true
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Hello</p><p>How are you?</p>"
            received: true
        }
        ListElement {
            avatar: "home.png"
            date: ""
            from: "Me"
            html: "<p>Just fine, how about yourself?</p><p>How are you?</p>"
            received: false
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Where did you spend your holidays?</p>"
            received: true
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Hello</p><p>How are you?</p>"
            received: true
        }
        ListElement {
            avatar: "home.png"
            date: ""
            from: "Me"
            html: "<p>Just fine, how about yourself?</p><p>How are you?</p>"
            received: false
        }
        ListElement {
            avatar: "peer.png"
            date: ""
            from: "Osborne Cox"
            html: "<p>Where did you spend your holidays?</p>"
            received: true
        }
    }
*/

    HistoryView {
        id: historyView

        anchors.left: parent.left
        anchors.top: parent.top
        height: parent.height
        width: parent.width - scrollBar.width
        model: historyModel
    }

    ScrollBar {
        id: scrollBar
        anchors.right: parent.right
        height: parent.height
        flickableItem: historyView
    }
}
