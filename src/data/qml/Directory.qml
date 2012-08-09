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

Item {
    id: directory

    GridView {
        id: directoryGrid

        cacheBuffer: 1000
        height: parent.height
        cellWidth: 120
        cellHeight: 120
        width: parent.width
        model: directoryXml
        delegate: directoryDelegate
    }

    DirectoryXmlModel {
        id: directoryXml

        query: '/wilink/directory'

        XmlRole { name: 'name'; query: 'name/string()' }
        XmlRole { name: 'jabber_id'; query: 'jabber_id/string()' }
        XmlRole { name: 'avatar_url'; query: 'avatar_url/string()' }
        XmlRole { name: 'gender'; query: 'gender/string()' }
        XmlRole { name: 'birthday'; query: 'birthday/string()' }
        XmlRole { name: 'hometown'; query: 'hometown/string()' }
        XmlRole { name: 'first_name'; query: 'first_name/string()' }
        XmlRole { name: 'last_name'; query: 'last_name/string()' }
        XmlRole { name: 'interested_in'; query: 'interested_in/array()' }
        XmlRole { name: 'relationship_status'; query: 'relationship_status/string()' }
    }

    Component {
        id: directoryDelegate

        Image {
            id: avatar

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            source: appSettings.wifirstBaseUrl + avatar_url
            width: 120
            height: 120
        }

        Text {
            anchors.top: avatar.top
            text: name
        }
    }
}

