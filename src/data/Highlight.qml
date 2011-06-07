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

Rectangle {
    border.color: '#ffb0c4de'
    border.width: 1
    gradient: Gradient {
        GradientStop { id:stop1; position:0.0; color: '#33b0c4de' }
        GradientStop { id:stop2; position:0.5; color: '#ffb0c4de' }
        GradientStop { id:stop3; position:1.0; color: '#33b0c4de' }
    }
    radius: 5
    smooth: true
}

