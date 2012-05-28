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

#include <QDesktopServices>
#include <QUrl>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"
#include "QXmppRtpChannel.h"

#include "history.h"
#include "phone.h"
#include "phone/sip.h"

PhoneAudioHelper::PhoneAudioHelper(QObject *parent)
    : QObject(parent)
    , m_call(0)
{
    QSoundPlayer *player = QSoundPlayer::instance();
    m_stream = new QSoundStream(player);
    connect(m_stream, SIGNAL(inputVolumeChanged(int)),
            this, SIGNAL(inputVolumeChanged(int)));
    connect(m_stream, SIGNAL(outputVolumeChanged(int)),
            this, SIGNAL(outputVolumeChanged(int)));
    m_stream->moveToThread(player->soundThread());
}

PhoneAudioHelper::~PhoneAudioHelper()
{
    if (m_call) {
        m_call->hangup();
        m_call->deleteLater();
    }
}

SipCall *PhoneAudioHelper::call() const
{
    return m_call;
}

void PhoneAudioHelper::setCall(SipCall *call)
{
    if (call != m_call) {
        // disconnect old signals
        if (m_call)
            m_call->disconnect(this);

        // connect new signals
        m_call = call;
        if (m_call) {
            bool check;

            m_call->audioChannel()->setParent(0);
            m_call->audioChannel()->moveToThread(m_stream->thread());

            check = connect(m_call, SIGNAL(stateChanged(SipCall::State)),
                            this, SLOT(_q_callStateChanged(SipCall::State)));
            Q_ASSERT(check);
            Q_UNUSED(check);

            _q_callStateChanged(m_call->state());
        }

        // change call
        emit callChanged();
    }
}

int PhoneAudioHelper::inputVolume() const
{
    return m_stream->inputVolume();
}

int PhoneAudioHelper::maximumVolume() const
{
    return m_stream->maximumVolume();
}

int PhoneAudioHelper::outputVolume() const
{
    return m_stream->outputVolume();
}

void PhoneAudioHelper::_q_callStateChanged(SipCall::State state)
{
    Q_ASSERT(m_call);

    if (state == SipCall::ActiveState) {
        // start audio input / output
        QXmppRtpAudioChannel *channel = m_call->audioChannel();
        m_stream->setDevice(channel);
        m_stream->setFormat(
            m_call->audioChannel()->payloadType().channels(),
            m_call->audioChannel()->payloadType().clockrate());

        QMetaObject::invokeMethod(m_stream, "startOutput");
        QMetaObject::invokeMethod(m_stream, "startInput");

    } else if (state == SipCall::FinishedState) {
        // stop audio input / output
        QMetaObject::invokeMethod(m_stream, "stopInput");
        QMetaObject::invokeMethod(m_stream, "stopOutput");

        emit inputVolumeChanged(0);
        emit outputVolumeChanged(0);
    }
}

#if 0
    // register URL handler
    if (!m_registeredHandler) {
        HistoryMessage::addTransform(QRegExp("^(.*\\s)?(\\+?[0-9]{4,})(\\s.*)?$"),
            QString("\\1<a href=\"sip:\\2@%1\">\\2</a>\\3").arg(m_client->domain()));
        QDesktopServices::setUrlHandler("sip", this, "_q_openUrl");
        m_registeredHandler = true;
    }

    // response to URL
    if (url.scheme() == "sip" && !url.path().isEmpty()) {
        const QString phoneNumber = url.path().split('@').first();
        const QString recipient = QString("\"%1\" <%2>").arg(phoneNumber, url.toString());
        m_client->call(recipient);
    }
#endif

