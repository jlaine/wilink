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
import 'utils.js' as Utils

Panel {
    function trim(text)
    {
        return text.replace(/^\s+|\s+$/g, '');
    }

    PanelHeader {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        iconSource: 'peer.png'
        title: qsTr('News reader')
    }

    ListView {
        id: view

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: scrollBar.left

        delegate: Item {
            height: 32
            width: view.width - 1

            Text {
                anchors.fill: parent
                text: model.title
            }
        }

        model: XmlListModel {
            id: newsModel

            query: '/rss/channel/item'
            source: 'http://feeds.bbci.co.uk/news/rss.xml';

            XmlRole { name: 'description'; query: 'description/string()' }
            XmlRole { name: 'link'; query: 'description/string()' }
            XmlRole { name: 'pubDate'; query: 'pubDate/string()' }
            XmlRole { name: 'title'; query: 'title/string()' }
        }
    }

    ScrollBar {
        id: scrollBar

        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        flickableItem: view
    }
}

