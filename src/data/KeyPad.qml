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
import QXmpp 0.4

Item {
    id: grid

    signal keyPressed(variant key)
    signal keyReleased(variant key)

    width: 115
    height: 155

    GridView {
        cellWidth: 40
        cellHeight: 40
        interactive: false
        width: cellWidth * 3
        height: cellHeight * 4

        delegate: Button {
            id: button

            text: model.name
            width: 35
            height: 35

            onPressed: grid.keyPressed(model)
            onReleased: grid.keyReleased(model)
        }
        model: ListModel {
            ListElement { name: '1'; tone: QXmppRtpAudioChannel.Tone_1 }
            ListElement { name: '2'; tone: QXmppRtpAudioChannel.Tone_2 }
            ListElement { name: '3'; tone: QXmppRtpAudioChannel.Tone_3 }
            ListElement { name: '4'; tone: QXmppRtpAudioChannel.Tone_4 }
            ListElement { name: '5'; tone: QXmppRtpAudioChannel.Tone_5 }
            ListElement { name: '6'; tone: QXmppRtpAudioChannel.Tone_6 }
            ListElement { name: '7'; tone: QXmppRtpAudioChannel.Tone_7 }
            ListElement { name: '8'; tone: QXmppRtpAudioChannel.Tone_8 }
            ListElement { name: '9'; tone: QXmppRtpAudioChannel.Tone_9 }
            ListElement { name: '*'; tone: QXmppRtpAudioChannel.Tone_Star }
            ListElement { name: '0'; tone: QXmppRtpAudioChannel.Tone_0 }
            ListElement { name: '#'; tone: QXmppRtpAudioChannel.Tone_Pound }
        }
    }
}

