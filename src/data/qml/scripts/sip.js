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

.pragma library

// Builds a full SIP address from a short recipient
function buildAddress(recipient, sipDomain)
{
    var address;
    var name;
    var bits = recipient.split('@');
    if (bits.length > 1) {
        name = bits[0];
        address = recipient;
    } else {
        name = recipient;
        address = recipient + "@" + sipDomain;
    }
    return '"' + name + '" <sip:' + address + '>';
}

// Extracts the shortest possible recipient from a full SIP address.
function parseAddress(sipAddress, sipDomain)
{
    var cap = sipAddress.match(/(.*)<(sip:([^>]+))>(;.+)?/);
    if (!cap)
        return '';

    var recipient = cap[2].substr(4);
    var bits = recipient.split('@');
    if (bits[1] == sipDomain || bits[1].match(/^[0-9]+/))
        return bits[0];
    else
        return recipient;
}
