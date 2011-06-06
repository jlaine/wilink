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
    property variant presenceStatus

    opacity: (presenceStatus != undefined) ? 1 : 0
    state: {
        switch(presenceStatus) {
            case QXmppPresence.Online:
            case QXmppPresence.Chat:
                return 'available';
            case QXmppPresence.Away:
            case QXmppPresence.XA:
                return 'away';
            case QXmppPresence.DND:
                return 'busy';
            case QXmppPresence.Offline:
            case QXmppPresence.Invisible:
            default:
                return 'offline';
        }
    }

    Gradient { // Available
        id: availableGradient
        GradientStop { position: 0.0; color: '#64ff64' }
        GradientStop { position: 1.0; color: '#009600' }
    }
    Gradient { // Away
        id: awayGradient
        GradientStop { position: 0.0; color: '#ffc800' }
        GradientStop { position: 1.0; color: '#d28c00' }
    }
    Gradient { // Busy
        id: busyGradient
        GradientStop { position: 0.0; color: '#ff6464' }
        GradientStop { position: 1.0; color: '#dd0000' }
    }
    Gradient { // Offline
        id: offlineGradient
        GradientStop { position: 0.0; color: '#dfdfdf' }
        GradientStop { position: 1.0; color: '#999999' }
    }

    Rectangle {
        id: status
        anchors.fill: parent
        border.width: 1
        border.color: 'transparent'
        color: 'transparent'
        radius: Math.max(parent.width, parent.height) * 2
        smooth: true
    }

    states: [
        State {
            name: 'available'
            PropertyChanges {
                target: status
                gradient: availableGradient
                border.color: '#006400'
            }
        },
        State {
            name: 'away'
            PropertyChanges {
                target: status
                gradient: awayGradient
                border.color: '#c86400'
            }
        },
        State {
            name: 'busy'
            PropertyChanges {
                target: status
                gradient: busyGradient
                border.color: '#640000'
            }
        },
        State {
            name: 'offline'
            PropertyChanges {
                target: status
                gradient: offlineGradient
                border.color: '#999999'
            }
        }
    ]
}
