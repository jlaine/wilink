/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

Plugin {
    name: qsTr('Diagnostics')
    description: qsTr('This plugin allows you to run tests to diagnostic network issues.')
    iconStyle: 'icon-info-sign'

    onLoaded: {
        dock.model.add({
            iconStyle: iconStyle,
            panelSource: 'DiagnosticPanel.qml',
            priority: 6,
            shortcut: Qt.ControlModifier + Qt.Key_I,
            text: qsTr('Diagnostics'),
            visible: true,
        });
    }

    onUnloaded: {
        dock.model.removePanel('DiagnosticPanel.qml');
    }
}
