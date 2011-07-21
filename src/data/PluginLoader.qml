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

    property alias model: pluginModel

    ListModel {
        id: pluginModel

        ListElement { source: 'DiagnosticPlugin.qml'; loaded: false }
        ListElement { source: 'PlayerPlugin.qml'; loaded: false }
        ListElement { source: 'RssPlugin.qml'; loaded: false }
        ListElement { source: 'DebugPlugin.qml'; loaded: false }
    }

    function loadPlugin(source) {
        // check plugin is not already loaded
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.source == source && plugin.loaded) {
                return;
            }
        }

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

    function unloadPlugin(source) {
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.source == source) {
                if (plugin.loaded) {
                    console.log("PluginLoader unloading plugin " + source);
                    // TODO: unload!
                    pluginModel.setProperty(i, 'loaded', false);
                }
                break;
            }
        }
    }
}
