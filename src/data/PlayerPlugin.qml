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
    name: qsTr('Music')
    summary: qsTr('Play your favorite music!')
    description: qsTr('This plugin adds a music player to wiLink.')
    imageSource: 'start.png'

    onLoaded: {
        dock.model.append({
            'iconSource': 'dock-start.png',
            'iconPress': 'start.png',
            'panelSource': 'PlayerPanel.qml',
            'shortcut': Qt.ControlModifier + Qt.Key_U,
            'text': qsTr('Music'),
            'visible': true});
    }

    onUnloaded: {
        dock.model.removePanel('PlayerPanel.qml');
    }
}
