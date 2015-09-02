/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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

import QtQuick 2.3

Menu {
    id: menu

    property Item target

    onItemClicked: {
        var item = menu.model.get(index);
        if (item.action == 'cut') {
            target.cut();
        } else if (item.action == 'copy') {
            target.copy();
        } else if (item.action == 'paste') {
            target.paste();
        }
    }

    Component.onCompleted: {
        menu.model.append({
            action: 'cut',
            iconStyle: 'icon-cut',
            text: qsTr('Cut')});
        menu.model.append({
            action: 'copy',
            iconStyle: 'icon-copy',
            text: qsTr('Copy')});
        menu.model.append({
            action: 'paste',
            iconStyle: 'icon-paste',
            text: qsTr('Paste')});
    }
}

