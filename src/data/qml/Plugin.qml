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
    // The plugin name.
    property string name

    // The full description of the plugin.
    property string description

    // The URL's icon style.
    property string iconStyle: 'icon-cogs'

    // This signal is emitted when the plugin is enabled.
    //
    // You can add an "onLoaded" handler in your plugin for
    // instance to populate the Dock.
    signal loaded

    // This signal is emitted when the plugin is disabled.
    //
    // You can add an "onRemoved" handled in your plugin for
    // instance to remove items from the Dock.
    signal unloaded
}
