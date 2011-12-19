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
import wiLink 2.0

Item {
    property variant url

    ListModel {
        id: photoModel

        ListElement { imageColor: 'blue'; imageSource: 'blue.png' }
        ListElement { imageColor: 'green'; imageSource: 'green.png' }
        ListElement { imageColor: 'red'; imageSource: 'red.png' }
        ListElement { imageColor: 'pink'; imageSource: 'pink.png' }
        ListElement { imageColor: 'yellow'; imageSource: 'yellow.png' }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: 'black'
    }

    Repeater {
        id: view

        anchors.fill: parent
        delegate: Component {
            Rectangle {
                id: rect

                width: 100 * z
                height: 75 * z
                color: model.imageColor
                y: view.height + height
                z: 1
                states: State {
                    name: 'finished'
                    PropertyChanges { target: rect; y: -rect.height }
                }
                transitions: Transition {
                    to: 'finished'

                    SequentialAnimation {
                        NumberAnimation {
                            duration: 30000 / z
                            target: rect
                            properties: 'y'
                        }
                    }
                }

                Component.onCompleted: {
                    z = 1 + Math.floor(Math.random() * 3);
                    x = Math.floor(Math.random() * view.width - 0.5 * rect.width);
                    state = 'finished';
                }
            }
        }
        model: ListModel {}
    }

    Timer {
        id: timer

        property int modelIndex: 0

        interval: 2000
        repeat: true
        running: true

        onTriggered: {
            view.model.append(photoModel.get(modelIndex));
            modelIndex = (modelIndex + 1) % photoModel.count;
        }

        Component.onCompleted: timer.triggered()
    }
}
