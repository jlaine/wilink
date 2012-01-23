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

import QtQuick 1.1
import 'utils.js' as Utils

Plugin {
    name: qsTr('Photos')
    description: qsTr('This plugin allows you to access your photos.')
    imageSource: 'photos.png'

    onLoaded: {
        var url;
        var domain = Utils.jidToDomain(appClient.jid);
        if (domain == 'wifirst.net')
            url = 'wifirst://www.wifirst.net/w';
        else if (domain == 'gmail.com')
            url = 'picasa://default';
        else
            return;

        dock.model.add({
            'iconSource': 'dock-photo.png',
            'iconPress': 'photos.png',
            'panelSource': 'PhotoPanel.qml',
            'panelProperties': {'url': url},
            'priority': 7,
            'shortcut': Qt.ControlModifier + Qt.Key_P,
            'text': qsTr('Photos'),
            'visible': true});
    }

    onUnloaded: {
        dock.model.removePanel('PhotoPanel.qml');
    }
}
