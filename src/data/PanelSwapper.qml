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
import 'utils.js' as Utils

FocusScope {
    id: panelSwapper

    property alias model: panels
    property int currentIndex: -1
    property Item currentItem
    property string currentSource

    ListModel {
        id: panels
    }

    function addPanel(source, properties, show) {
        if (properties == undefined)
            properties = {};

        // create the panel
        console.log("PanelSwapper creating panel " + source + " " + Utils.dumpProperties(properties));
        var component = Qt.createComponent(source);
        if (component.status == Component.Loading)
            component.statusChanged.connect(finishCreation);
        else
            finishCreation();

        function finishCreation() {
            switch (component.status) {
            case Component.Error:
                console.log("PanelSwapper could not create panel " + source);
                console.log(component.errorString());
                return;
            case Component.Ready:
                break;
            default:
                return;
            }

            // FIXME: when Qt Quick 1.1 becomes available,
            // let createObject assign the properties itself.
            var panel = component.createObject(panelSwapper);
            for (var key in properties) {
                panel[key] = properties[key];
            }

            panels.append({'source': source, 'properties': properties, 'panel': panel});
            panel.close.connect(function() {
                d.removePanel(panel);
            });

            if (show)
                panelSwapper.currentIndex = panels.count - 1;
        }
    }

    function findPanel(source, properties) {
        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).source == source &&
                (properties === undefined || Utils.equalProperties(panels.get(i).properties, properties))) {
                return panels.get(i).panel;
            }
        }
        return null;
    }

    function showPanel(source, properties) {
        var panel = findPanel(source, properties);
        if (panel) {
            // if the panel already exists, show it
            panelSwapper.currentIndex = d.findPanelIndex(panel);
        } else {
            // otherwise create and show it
            addPanel(source, properties, true);
        }
    }

    function decrementCurrentIndex() {
        if (panelSwapper.currentIndex < 0 || panels.count < 2)
            return;

        if (panelSwapper.currentIndex == 0) {
            panelSwapper.currentIndex = panels.count - 1;
        } else {
            panelSwapper.currentIndex -= 1;
        }
    }

    function incrementCurrentIndex() {
        if (panelSwapper.currentIndex < 0 || panels.count < 2)
            return;

        if (panelSwapper.currentIndex == panels.count - 1) {
            panelSwapper.currentIndex = 0;
        } else {
            panelSwapper.currentIndex += 1;
        }
    }

    // private implementation
    Item {
        id: d

        function findPanelIndex(panel) {
            for (var i = 0; i < panels.count; i++) {
                if (panels.get(i).panel == panel)
                    return i;
            }
            return -1;
        }

        function removePanel(panel) {
            var i = d.findPanelIndex(panel);
            if (i < 0) return;

            console.log("PanelSwapper removing panel " + panels.get(i).source + " " + Utils.dumpProperties(panels.get(i).properties));

            // remove panel
            panels.remove(i);
            if (panels.count == 0) {
                panelSwapper.currentIndex = -1;
            } else if (i <= panelSwapper.currentIndex) {
                panelSwapper.currentIndex -= 1;
            }

            // destroy panel
            panel.destroy();
        }
    }

    onCurrentIndexChanged: {
        var panel = null, source = '';

        if (panelSwapper.currentIndex >= 0 && panelSwapper.currentIndex < panels.count) {
            var item = panels.get(panelSwapper.currentIndex);
            panel = item.panel;
            source = item.source;
        }

        if (panel != panelSwapper.currentItem) {
            // show new item
            if (panel) {
                panel = panel;
                panel.opacity = 1;
                panel.focus = true;
                panel.z = 1;
            }

            // hide old item
            if (panelSwapper.currentItem) {
                panelSwapper.currentItem.opacity = 0;
                panelSwapper.currentItem.z = -1;
            }

            panelSwapper.currentItem = panel;
            panelSwapper.currentSource = source;
        }
    }
}

