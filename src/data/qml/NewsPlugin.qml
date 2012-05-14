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
    name: qsTr('News reader')
    description: qsTr('This plugin allows you to read RSS news feeds.')
    imageSource: 'image://icon/rss'

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            if (account.type == 'xmpp' && account.realm == 'wifirst.net') {
                dock.model.add({
                    'iconSource': 'image://icon/dock-rss',
                    'iconPress': 'image://icon/rss',
                    'panelProperties': {accountJid: account.username},
                    'panelSource': 'NewsPanel.qml',
                    'shortcut': Qt.ControlModifier + Qt.Key_N,
                    'text': qsTr('News'),
                    'visible': true});
            }
        }
    }

    onUnloaded: {
        dock.model.removePanel('NewsPanel.qml');
    }

}
