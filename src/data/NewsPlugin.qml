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

Plugin {
    name: qsTr('News reader')
    summary: 'Follow your friends over RSS'
    description: 'This plugin adds an RSS reader to wiLink.'
    imageSource: 'peer.png'

    onLoaded: {
        dock.model.append({
            'iconSource': 'peer.png',
            'iconPress': 'peer.png',
            'panelSource': 'NewsPanel.qml',
            'shortcut': Qt.ControlModifier + Qt.Key_N,
            'text': qsTr('News'),
            'visible': true});
    }

    onUnloaded: {
        dock.model.removePanel('NewsPanel.qml');
    }

}
