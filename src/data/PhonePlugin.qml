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
import 'utils.js' as Utils

Plugin {
    id: plugin

    property bool isLoaded: false

    name: qsTr('Phone')
    description: qsTr('This plugin allows you to make phone calls.')
    imageSource: 'phone.png'

    onLoaded: {
        if (Utils.jidToDomain(appClient.jid) != 'wifirst.net')
            return;
        dock.model.append({
            'iconSource': 'dock-phone.png',
            'iconPress': 'phone.png',
            'panelSource': 'PhonePanel.qml',
            'shortcut': Qt.ControlModifier + Qt.Key_T,
            'text': qsTr('Phone'),
            'visible': true});
        swapper.addPanel('PhonePanel.qml');
    }

    onUnloaded: {
        dock.model.removePanel('PhonePanel.qml');
    }
}

