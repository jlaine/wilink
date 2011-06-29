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
import wiLink 1.2

Menu {
    id: menu

    property alias jid: vcard.jid
    property bool profileEnabled: (vcard.url != undefined && vcard.url != '')

    onProfileEnabledChanged: menu.model.setProperty(0, 'enabled', profileEnabled)

    VCard {
        id: vcard
    }

    onItemClicked: {
        var item = menu.model.get(index);
        if (item.action == 'profile') {
            Qt.openUrlExternally(vcard.url);
        } else if (item.action == 'rename') {
            dialogSwapper.showPanel('ContactRenameDialog.qml', {'jid': jid});
        } else if (item.action == 'remove') {
            dialogSwapper.showPanel('ContactRemoveDialog.qml', {'jid': jid});
        }
    }

    Component.onCompleted: {
        menu.model.append({
            'action': 'profile',
            'enabled': profileEnabled,
            'icon': 'information.png',
            'text': qsTr('Show profile')});
        menu.model.append({
            'action': 'rename',
            'icon': 'options.png',
            'name': model.name,
            'text': qsTr('Rename contact')});
        menu.model.append({
            'action': 'remove',
            'icon': 'remove.png',
            'name': model.name,
            'text': qsTr('Remove contact')});
    }
}

