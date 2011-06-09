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
    id: swapper

    property Item currentItem

    ListModel {
        id: panels
    }

    function addPanel(source, properties) {
        if (properties == undefined)
            properties = {};

        function propDump(a) {
            var dump = '';
            for (var key in a)
                dump += (dump.length > 0 ? ', ' : '') + key + ': ' + a[key];
            return '{' + dump + '}';
        }

        // if the panel already exists, return it
        var panel = findPanel(source, properties);
        if (panel) {
            return panel;
        }

        // otherwise create the panel
        console.log("creating panel " + source + " " + propDump(properties));
        var component = Qt.createComponent(source);
        // FIXME: when Qt Quick 1.1 becomes available,
        // let createObject assign the properties itself.
        var panel = component.createObject(swapper);
        for (var key in properties) {
            panel[key] = properties[key];
        }

        function hidePanel(i) {
            // if the panel was visible, show last remaining panel
            if (swapper.currentItem == panel) {
                if (panels.count == 1)
                    swapper.setCurrentItem(null);
                else if (i == panels.count - 1)
                    swapper.setCurrentItem(panels.get(i - 1).panel);
                else
                    swapper.setCurrentItem(panels.get(i + 1).panel);
            }
        }

        panel.close.connect(function() {
            for (var i = 0; i < panels.count; i += 1) {
                if (panels.get(i).panel == panel) {
                    console.log("removing panel " + panels.get(i).source + " " + propDump(panels.get(i).properties));

                    // hide panel
                    hidePanel(i);

                    // destroy panel
                    panels.remove(i);
                    panel.destroy();
                    break;
                }
            }
        });
        panel.notify.connect(function(title, text) {
            // show notification
            application.showMessage(panel, title, text);

            // alert window
            window.alert();

            // play a sound
            application.soundPlayer.play(application.incomingMessageSound);
        });
        panels.append({'source': source, 'properties': properties, 'panel': panel});
        return panel;
    }

    function findPanel(source, properties) {
        if (properties == undefined)
            properties = {};

        // helper to compare object properties
        function propEquals(a, b) {
            if (a.length != b.length)
                return false;
            for (var key in a) {
                if (a[key] != b[key])
                    return false;
            }
            return true;
        }

        for (var i = 0; i < panels.count; i += 1) {
            if (panels.get(i).source == source &&
                propEquals(panels.get(i).properties, properties)) {
                return panels.get(i).panel;
            }
        }
        return null;
    }

    function showPanel(source, properties) {
        var panel = addPanel(source, properties);
        swapper.setCurrentItem(panel);
        return panel;
    }

    function setCurrentItem(panel) {
        if (panel == currentItem)
            return;

        // show new item
        if (panel)
            panel.opacity = 1;
        else
            background.opacity = 1;

        // hide old item
        if (currentItem)
            currentItem.opacity = 0;
        else
            background.opacity = 0;

        currentItem = panel;
    }

    Rectangle {
        id: background
        anchors.fill: parent
    }
}

