/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

import QtQuick 2.3
import QtMultimedia 5.4
import wiLink 2.5

NotificationDialog {
    id: dialog

    property QtObject call
    property string caller
    property Item swapper

    iconStyle: 'icon-phone'
    text: qsTr('%1 wants to talk to you.\n\nDo you accept?').replace('%1', caller)
    title: qsTr('Call from %1').replace('%1', caller)

    resources: [
        SoundEffect {
            loops: SoundEffect.Infinite
            source: 'sounds/call-incoming.wav'
            Component.onCompleted: play()
        },
        Connections {
            target: call
            onFinished: {
                call.destroyLater();
                dialog.close();
            }
        }
    ]

    onAccepted: {
        swapper.showPanel('PhonePanel.qml');
        dialog.call.accept();
        dialog.close();
    }

    onRejected: {
        dialog.call.hangup();
        dialog.call.destroyLater();
    }
}

