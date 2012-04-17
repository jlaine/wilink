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
import 'utils.js' as Utils

Plugin {
    name: qsTr('Photos')
    description: qsTr('This plugin allows you to access your photos.')
    imageSource: 'photos.png'

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);

            var url;
            if (account.type == 'wifirst')
                url = 'wifirst://default';
            else if (account.type == 'google')
                url = 'picasa://default';
            else
                continue;

            var title = qsTr('Photos');
            if (accountModel.count > 1)
                title += '<br/><small>' + Utils.jidToDomain(account.jid) + '</small>';

            dock.model.add({
                'iconSource': 'dock-photo.png',
                'iconPress': 'photos.png',
                'panelSource': 'PhotoPanel.qml',
                'panelProperties': {'url': url},
                'priority': 7,
                'shortcut': Qt.ControlModifier + Qt.Key_P,
                'text': title,
                'visible': true});
        }
    }

    onUnloaded: {
        dock.model.removePanel('PhotoPanel.qml');
    }
}
