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

ListModel {
    function removePanel(source) {
        for (var i = 0; i < appPreferences.count; i++) {
            if (appPreferences.get(i).source == source) {
                appPreferences.remove(i);
                break;
            }
        }
    }

    Component.onCompleted: {
        append({
            'iconSource': 'options.png',
            'name': qsTr('General'),
            'source': 'GeneralPreferencePanel.qml'});
        append({
            'iconSource': 'peer.png',
            'name': qsTr('Accounts'),
            'source': 'AccountPreferencePanel.qml'});
        append({
            'iconSource': 'audio-output.png',
            'name': qsTr('Sound'),
            'source': 'SoundPreferencePanel.qml'});
        append({
            'iconSource': 'plugin.png',
            'name': qsTr('Plugins'),
            'source': 'PluginPreferencePanel.qml'});
    }
}
