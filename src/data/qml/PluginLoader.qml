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

import QtQuick 1.1

Item {
    id: loader

    property alias model: pluginModel

    ListModel {
        id: pluginModel
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
            pluginModel.append({source: source, loaded: plugin});
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
        var disabledPlugins = [];
        var enabledPlugins = [];
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if (plugin.autoload == true && plugin.loaded == undefined)
                disabledPlugins.push(plugin.source);
            else if (plugin.autoload != true && plugin.loaded != undefined)
                enabledPlugins.push(plugin.source);
        }
        appSettings.disabledPlugins = disabledPlugins;
        appSettings.enabledPlugins = enabledPlugins;
    }

    function load() {
        for (var i = 0; i < pluginModel.count; i++) {
            var plugin = pluginModel.get(i);
            if ((plugin.autoload && appSettings.disabledPlugins.indexOf(plugin.source) < 0)
                || appSettings.enabledPlugins.indexOf(plugin.source) >= 0) {
                loadPlugin(plugin.source);
            }
        }
    }

    function unload() {
        for (var i = pluginModel.count - 1; i >= 0; i--) {
            var plugin = pluginModel.get(i);
            unloadPlugin(plugin.source);
        }
    }

    Component.onCompleted: {
        //pluginModel.append({ source: 'LandingPagePlugin.qml', autoload: true });
        pluginModel.append({ source: 'ChatPlugin.qml', autoload: true });
        pluginModel.append({ source: 'DebugPlugin.qml', autoload: false });
        pluginModel.append({ source: 'DiagnosticPlugin.qml', autoload: true });
        if (!appSettings.isMobile)
            pluginModel.append({ source: 'PhonePlugin.qml', autoload: true });
        pluginModel.append({ source: 'SharePlugin.qml', autoload: true });
        pluginModel.append({ source: 'WebPlugin.qml', autoload: true });
    }
}
