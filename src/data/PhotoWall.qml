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

GridView {
    id: root
    width: 600; height: 400
    cellHeight: 240
    cellWidth: 240

    property int imageSize: 200

    model: XmlListModel {
        id: listModel
        query: '/rss/channel/item'
        source: 'http://api.flickr.com/services/feeds/groups_pool.gne?id=863004@N20&lang=fr-fr&format=rss2'

        XmlRole { name: 'source'; query: 'enclosure[@type="image/jpeg"]/@url/string()' }
        XmlRole { name: 'title'; query: 'title/string()' }
    }

    delegate: Item {
        id: item
        property int animDuration: 0

        Image {
            parent: root
            x: item.x + 15
            y: item.y

            fillMode: Image.PreserveAspectFit
            width: 200
            height: 200
            smooth: true
            source: model.source
            transform: Rotation {
                id: rotation
                Behavior on angle {
                    NumberAnimation { duration: item.animDuration }
                }
            }

            Behavior on x {
                NumberAnimation { duration: item.animDuration }
            }

            Behavior on y {
                NumberAnimation { duration: item.animDuration }
            }

            Timer {
                interval: 100
                repeat: false
                running: true
                onTriggered: {
                    item.animDuration = 400;
                    rotation.angle = (Math.random() - 0.5) * 10;
                }
            }
        }
    }
}

