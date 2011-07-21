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

FocusScope {
    id: panelSwapper

    property Item currentItem
    property string currentSource

    ListModel {
        id: panels
    }

    function addPanel(source, properties, show) {
        if (properties == undefined)
            properties = {};

        // if the panel already exists, return it
        var panel = findPanel(source, properties);
        if (panel) {
            if (show)
                panelSwapper.setCurrentItem(panel);
            return;
        }

        // otherwise create the panel
        console.log("PanelSwapper creating panel " + source + " " + Utils.dumpProperties(properties));
        var component = Qt.createComponent(source);
        if (component.status == Component.Ready)
            finishCreation();
        else
            component.statusChanged.connect(finishCreation);

        function finishCreation() {
            if (component.status != Component.Ready)
                return;

            // FIXME: when Qt Quick 1.1 becomes available,
            // let createObject assign the properties itself.
            var panel = component.createObject(panelSwapper);
            for (var key in properties) {
                panel[key] = properties[key];
            }

            panels.append({'source': source, 'properties': properties, 'panel': panel});
            var savedProperties = panels.get(panels.count-1).properties;
            panel.close.connect(function() {
                panelSwapper.removePanel(source, savedProperties);
            });

            if (show)
                panelSwapper.setCurrentItem(panel);
        }
    }

    function findPanel(source, properties) {
        if (properties == undefined)
            properties = {};

        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).source == source &&
                Utils.equalProperties(panels.get(i).properties, properties)) {
                return panels.get(i).panel;
            }
        }
        return null;
    }

    function removePanel(source, properties) {
        if (properties == undefined)
            properties = {};

        for (var i = 0; i < panels.count; i++) {
            if (panels.get(i).source == source &&
                Utils.equalProperties(panels.get(i).properties, properties)) {
                var panel = panels.get(i).panel;

                console.log("PanelSwapper removing panel " + source + " " + Utils.dumpProperties(properties));

                // if the panel was visible, show last remaining panel
                if (panelSwapper.currentItem == panel) {
                    if (panels.count == 1)
                        panelSwapper.setCurrentItem(null);
                    else if (i == panels.count - 1)
                        panelSwapper.setCurrentItem(panels.get(i - 1).panel);
                    else
                        panelSwapper.setCurrentItem(panels.get(i + 1).panel);
                }

                // destroy panel
                panels.remove(i);
                panel.destroy();
                break;
            }
        }
    }

    function showPanel(source, properties) {
        addPanel(source, properties, true);
    }

    function setCurrentItem(panel) {
        if (panel == currentItem)
            return;

        // show new item
        if (panel) {
            panel.opacity = 1;
            panel.focus = true;
        } else {
            background.opacity = 1;
        }

        // hide old item
        if (currentItem)
            currentItem.opacity = 0;
        else
            background.opacity = 0;

        currentItem = panel;
        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).panel == panel) {
                currentSource = panels.get(i).source;
                return;
            }
        }
        currentSource = '';
    }

    Rectangle {
        id: background
        anchors.fill: parent
    }
}

