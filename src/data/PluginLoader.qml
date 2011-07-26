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

        ListElement { source: 'DebugPlugin.qml' }
        ListElement { source: 'DiagnosticPlugin.qml'; locked: true }
        ListElement { source: 'PlayerPlugin.qml' }
        ListElement { source: 'NewsPlugin.qml' }
        ListElement { source: 'WifirstPlugin.qml'; locked: true }
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
        if (component.status == Component.Loading)
            component.statusChanged.connect(finishCreation);
        else
            finishCreation();

        function finishCreation() {
            switch (component.status) {
            case Component.Error:
                console.log("PluginLoader could not load plugin " + source);
                console.log(component.errorString());
                return;
            case Component.Ready:
                break;
            default:
                return;
            }

            var plugin = component.createObject(loader);
            plugin.loaded();

            // update plugins
            for (var i = 0; i < pluginModel.count; i++) {
                if (pluginModel.get(i).source == source) {
                    pluginModel.setProperty(i, 'loaded', plugin);
                    return;
                }
            }
            pluginModel.append({'source': source, 'loaded': plugin});
        }
    }

    function unloadPlugin(source) {
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.source == source) {
                if (plugin.loaded) {
                    console.log("PluginLoader unloading plugin " + source);
                    plugin.loaded.unloaded();
                    plugin.loaded.destroy();
                    pluginModel.setProperty(i, 'loaded', undefined);
                }
                break;
            }
        }
    }

    function storePreferences() {
        // Store preference
        var plugins = [];
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.locked != true && plugin.loaded != undefined)
                plugins.push(plugin.source);
        }
        application.settings.enabledPlugins = plugins;
    }

    // FIXME : get / set preferences
    Component.onCompleted: {
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.locked)
                loadPlugin(plugin.source);
        }
        for (var i in application.settings.enabledPlugins) {
            loadPlugin(application.settings.enabledPlugins[i]);
        }
    }
}
