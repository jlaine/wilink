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
import 'scripts/utils.js' as Utils

Icon {
    id: socialIcon

    property string jid

    width: style ? appStyle.icon.tinySize : 0
    height: style ? appStyle.icon.tinySize : 0
    font.pixelSize: appStyle.icon.tinySize

    onJidChanged: {
        var domain = Utils.jidToDomain(jid);
        if (domain == 'chat.facebook.com') {
            color = '#3d589d';
            style = 'icon-facebook-sign';
        } else if (domain == 'gmail.com') {
            color = '#f71100';
            style = 'icon-google-plus-sign';
        } else {
            style = '';
        }
    }
}
