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

import QtQuick 1.1
import wiLink 2.4

Item {
    id: slideView

    property QtObject model
    property alias running: timer.running
    property variant url

    Repeater {
        id: view

        anchors.fill: parent
        delegate: Component {
            PhotoDelegate {
                id: rect

                width: 128 * z
                height: 128 * z
                y: view.height
                z: 1

                Component.onCompleted: {
                    z = 1 + Math.floor(Math.random() * 3);
                    x = Math.floor(Math.random() * (view.width - rect.width));
                    animation.running = true;
                }

                SequentialAnimation {
                    id: animation

                    NumberAnimation {
                        duration: Math.ceil(30000.0 / Math.sqrt(z))
                        target: rect
                        property: 'y'
                        to: -rect.height
                    }
                    ScriptAction {
                        script: view.model.remove(index)
                    }
                }
            }
        }
        model: ListModel {}
    }

    Timer {
        id: timer

        property int modelIndex: 0

        interval: 4000
        repeat: true
        triggeredOnStart: true

        onTriggered: {
            var item = slideView.model.get(modelIndex);
            if (item)
                view.model.append(item);
            modelIndex = (modelIndex + 1) % slideView.model.count;
        }
    }
}
