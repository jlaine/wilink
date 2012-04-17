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
    name: qsTr('Chat')
    description: qsTr('This plugin allows you to chat with your friends.')
    imageSource: 'chat.png'

    onLoaded: {
        for (var i = 0; i < accountModel.count; ++i) {
            var account = accountModel.get(i);
            var title = qsTr('Chat');
            if (accountModel.count > 1)
                title += '<br/><small>' + Utils.jidToDomain(account.jid) + '</small>';
            dock.model.add({
                'iconSource': 'dock-chat.png',
                'iconPress': 'chat.png',
                'notified': false,
                'panelProperties': {'accountJid': account.jid},
                'panelSource': 'ChatPanel.qml',
                'priority': 10,
                'text': title,
                'visible': true});
            swapper.showPanel('ChatPanel.qml', {'accountJid': account.jid});
        }
    }

    onUnloaded: {
        dock.model.removePanel('ChatPanel.qml');
    }
}
