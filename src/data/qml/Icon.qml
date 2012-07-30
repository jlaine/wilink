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

Label {
    property string style

    font.family: appStyle.icon.fontFamily
    text: {
        switch (style) {
        case 'icon-search':
            return '\uF002';
        case 'icon-remove':
            return '\uF00D';
        case 'icon-home':
            return '\uF015';
        case 'icon-download':
            return '\uF01A';
        case 'icon-upload':
            return '\uF01B';
        case 'icon-refresh':
            return '\uF021';
        case 'icon-lock':
            return '\uF023';
        case 'icon-play':
            return '\uF04B';
        case 'icon-pause':
            return '\uF04B';
        case 'icon-stop':
            return '\uF04D';
        case 'icon-eject':
            return '\uF052';
        case 'icon-chevron-left':
            return '\uF053';
        case 'icon-chevron-right':
            return '\uF054';
        case 'icon-question-sign':
            return '\uF059';
        case 'icon-info-sign':
            return '\uF05A';
        case 'icon-plus':
            return '\uF067';
        case 'icon-minus':
            return '\uF068';
        case 'icon-exclamation-sign':
            return '\uF06A';
        case 'icon-comment':
            return '\uF075';
        case 'icon-chevron-up':
            return '\uF077';
        case 'icon-chevron-down':
            return '\uF078';
        case 'icon-phone':
            return '\uF095';
        case 'icon-rss':
            return '\uF09E';
        case 'icon-wrench':
            return '\uF0AD';
        case 'icon-fullscreen':
            return '\uF0B2';
        case 'icon-cut':
            return '\uF0C4';
        case 'icon-copy':
            return '\uF0C5';
        case 'icon-reorder':
            return '\uF0C9';
        case 'icon-paste':
            return '\uF0EA';
        default:
            return '';
        }
    }
}
