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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include "phone/sip.h"

class QSoundPlayer;
class QSoundStream;
class QUrl;
class SipCall;
class SipClient;

/** The PhoneHistoryModel class represents the user's phone call history.
 */
class PhoneHistoryModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(SipClient* client READ client CONSTANT)
    Q_PROPERTY(int currentCalls READ currentCalls NOTIFY currentCallsChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)

public:
    PhoneHistoryModel(QObject *parent = 0);
    ~PhoneHistoryModel();

    SipClient *client() const;
    int currentCalls() const;
    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;

signals:
    void currentCallsChanged();
    void inputVolumeChanged(int inputVolume);
    void outputVolumeChanged(int outputVolume);

public slots:
    bool call(const QString &address);
    void startTone(int tone);
    void stopTone(int tone);

private slots:
    void _q_callStarted(SipCall *call);
    void _q_callStateChanged(SipCall::State state);
    void _q_openUrl(const QUrl &url);

private:
    QMap<SipCall*, QSoundStream*> m_activeCalls;
    SipClient *m_client;
    QSoundPlayer *m_player;
    bool m_registeredHandler;
};

#endif
