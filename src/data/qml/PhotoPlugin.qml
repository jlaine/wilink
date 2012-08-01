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

Plugin {
    name: qsTr('Photos')
    description: qsTr('This plugin allows you to access your photos.')
    imageSource: 'image://icon/photos'

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'web' && account.realm == 'www.google.com') {
                var title = qsTr('Photos');
                title += '<br/><small>Picasa</small>';

                dock.model.add({
                    'iconStyle': 'icon-picture',
                    'panelSource': 'PhotoPanel.qml',
                    'panelProperties': {'url': 'picasa://default'},
                    'priority': 7,
                    'shortcut': Qt.ControlModifier + Qt.Key_P,
                    'text': title,
                    'visible': true});
            }
        }
    }

    onUnloaded: {
        dock.model.removePanel('PhotoPanel.qml');
    }
}
