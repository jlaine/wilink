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
import QXmpp 0.4

Item {
    id: block

    property alias model: view.model

    ListView {
        id: view

        anchors.left: parent.left
        anchors.right: scrollBar.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 4
        clip: true

        delegate: Item {
            id: item
            width: view.width
            height: 32

            Rectangle {
                id: itemBackground
                anchors.fill: parent
                border.color: '#ffffff'
                color: '#ffffff'
                gradient: Gradient {
                    GradientStop { id: stop1; position: 0.0; color: '#ffffff'  }
                    GradientStop { id: stop2; position: 1.0; color: '#ffffff'  }
                }


                Image {
                    id: image

                    anchors.top: parent.top
                    anchors.left: parent.left
                    source: model.direction == QXmppCall.OutgoingDirection ? 'call-outgoing.png' : 'call-incoming.png'
                }

                Text {
                    anchors.top: parent.top
                    anchors.left: image.right
                    anchors.right: date.left
                    elide: Text.ElideRight
                    text: model.name
                }

                Text {
                    id: date

                    anchors.top: parent.top
                    anchors.right: duration.left
                    text: Qt.formatDateTime(model.date)
                    width: 150
                }

                Text {
                    id: duration

                    anchors.top: parent.top
                    anchors.right: parent.right
                    text: model.duration + 's'
                    width: 60
                }
            }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}
