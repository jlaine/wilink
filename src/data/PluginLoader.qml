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

Item {
    id: loader

    property alias availablePlugins: pluginModel

    ListModel {
        id: pluginModel

        ListElement { source: 'DiagnosticPlugin.qml'; loaded: false }
        ListElement { source: 'PlayerPlugin.qml'; loaded: false }
        ListElement { source: 'RssPlugin.qml'; loaded: false; }
        ListElement { source: 'LogPlugin.qml'; loaded: false; }
    }

    function loadPlugin(source) {
        console.log("PluginLoader loading plugin " + source);
        var component = Qt.createComponent(source);
        if (component.status == Component.Ready)
            finishCreation();
        else
            component.statusChanged.connect(finishCreation);

        function finishCreation() {
            if (component.status != Component.Ready)
                return;

            var plugin = component.createObject(loader);
            plugin.loaded();

            // update plugins
            for (var i = 0; i < pluginModel.count; i++) {
                if (pluginModel.get(i).source == source) {
                    pluginModel.setProperty(i, 'loaded', true);
                    break;
                }
            }
        }
    }
}
