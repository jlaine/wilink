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
import wiLink 2.5

CallWidget {
    id: callWidget

    audio: PhoneAudioHelper {
        call: callWidget.call
    }
    video: QtObject {
        property int openMode: 0
    }
    videoEnabled: false

    Connections {
        target: call
        onStateChanged: {
            if (call.state == QXmppCall.FinishedState) {
                if (call.errorString) {
                    dialogSwapper.showPanel('ErrorNotification.qml', {
                        title: qsTr('Call failed'),
                        text: qsTr('Sorry, but the call could not be completed.') + '\n\n' + call.errorString,
                    });
                }

                // destroy call
                call.destroyLater();
                callWidget.call = null;

                // make widget go away
                callWidget.opacity = 0;
                callWidget.destroy(1000);
            }
        }
    }

    Connections {
        target: keypad

        onKeyPressed: {
            audio.startTone(key.tone);
        }

        onKeyReleased: {
            audio.stopTone(key.tone);
        }
    }
}
