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
#include <QDomDocument>
#include <QDomElement>
#include <QTimer>
#include <QUrl>

#include "QSoundMeter.h"
#include "QSoundPlayer.h"
#include "QSoundStream.h"
#include "QXmppRtpChannel.h"

#include "history.h"
#include "phone.h"
#include "phone/sip.h"


/** Constructs a new model representing the call history.
 *
 * @param network
 * @param parent
 */
PhoneHistoryModel::PhoneHistoryModel(QObject *parent)
    : QObject(parent)
    , m_registeredHandler(false)
{
    bool check;
    Q_UNUSED(check);

    // sound
    m_player = QSoundPlayer::instance();

    // sip
    m_client = new SipClient;
    check = connect(m_client, SIGNAL(callStarted(SipCall*)),
                    this, SLOT(_q_callStarted(SipCall*)));
    Q_ASSERT(check);
    m_client->moveToThread(m_player->soundThread());

    // register URL handler
    if (!m_registeredHandler) {
        HistoryMessage::addTransform(QRegExp("^(.*\\s)?(\\+?[0-9]{4,})(\\s.*)?$"),
            QString("\\1<a href=\"sip:\\2@%1\">\\2</a>\\3").arg(m_client->domain()));
        QDesktopServices::setUrlHandler("sip", this, "_q_openUrl");
        m_registeredHandler = true;
    }
}

PhoneHistoryModel::~PhoneHistoryModel()
{
    // try to exit SIP client cleanly
    if (m_client->state() == SipClient::ConnectedState)
        QMetaObject::invokeMethod(m_client, "disconnectFromServer");
    m_client->deleteLater();
}

bool PhoneHistoryModel::call(const QString &address)
{
    if (m_client->state() == SipClient::ConnectedState &&
        !currentCalls()) {
        QMetaObject::invokeMethod(m_client, "call", Q_ARG(QString, address));
        return true;
    }
    return false;
}

/** Returns the underlying SIP client.
 *
 * \note Use with care as the SIP client lives in a different thread!
 */
SipClient *PhoneHistoryModel::client() const
{
    return m_client;
}

int PhoneHistoryModel::currentCalls() const
{
    return m_activeCalls.size();
}

int PhoneHistoryModel::inputVolume() const
{
    foreach (QSoundStream *audioStream, m_activeCalls.values()) {
        if (audioStream)
            return audioStream->inputVolume();
    }
    return 0;
}

int PhoneHistoryModel::maximumVolume() const
{
    return QSoundMeter::maximum();
}

int PhoneHistoryModel::outputVolume() const
{
    foreach (QSoundStream *audioStream, m_activeCalls.values()) {
        if (audioStream)
            return audioStream->outputVolume();
    }
    return 0;
}

/** Starts sending a tone.
 *
 * @param tone
 */
void PhoneHistoryModel::startTone(int toneValue)
{
    if (toneValue < QXmppRtpAudioChannel::Tone_0 || toneValue > QXmppRtpAudioChannel::Tone_D)
        return;
    QXmppRtpAudioChannel::Tone tone = static_cast<QXmppRtpAudioChannel::Tone>(toneValue);
    foreach (SipCall *call, m_activeCalls.keys())
        call->audioChannel()->startTone(tone);
}

/** Stops sending a tone.
 *
 * @param tone
 */
void PhoneHistoryModel::stopTone(int toneValue)
{
    if (toneValue < QXmppRtpAudioChannel::Tone_0 || toneValue > QXmppRtpAudioChannel::Tone_D)
        return;
    QXmppRtpAudioChannel::Tone tone = static_cast<QXmppRtpAudioChannel::Tone>(toneValue);
    foreach (SipCall *call, m_activeCalls.keys())
        call->audioChannel()->stopTone(tone);
}

QUrl PhoneHistoryModel::url() const
{
    return m_url;
}

void PhoneHistoryModel::setUrl(const QUrl &url)
{
    if (url != m_url) {
        m_url = url;
        emit urlChanged();
    }
}

QString PhoneHistoryModel::username() const
{
    return m_username;
}

void PhoneHistoryModel::setUsername(const QString &username)
{
    if (username != m_username) {
        m_username = username;
        emit usernameChanged();
    }
}

QString PhoneHistoryModel::password() const
{
    return m_password;
}

void PhoneHistoryModel::setPassword(const QString &password)
{
    if (password != m_password) {
        m_password = password;
        emit passwordChanged();
    }
}

void PhoneHistoryModel::_q_callStarted(SipCall *sipCall)
{
    m_activeCalls.insert(sipCall, 0);
    connect(sipCall, SIGNAL(stateChanged(SipCall::State)),
            this, SLOT(_q_callStateChanged(SipCall::State)));
    emit currentCallsChanged();
}

void PhoneHistoryModel::_q_callStateChanged(SipCall::State state)
{
    SipCall *call = qobject_cast<SipCall*>(sender());
    Q_ASSERT(call);

    // update the item
    if (state == SipCall::ActiveState) {

        // start audio input / output
        if (!m_activeCalls.value(call)) {
            QXmppRtpAudioChannel *channel = call->audioChannel();

            QSoundStream *audioStream = new QSoundStream(m_player);
            audioStream->setDevice(channel);
            audioStream->setFormat(
                channel->payloadType().channels(),
                channel->payloadType().clockrate());
            m_activeCalls.insert(call, audioStream);

            connect(audioStream, SIGNAL(inputVolumeChanged(int)),
                    this, SIGNAL(inputVolumeChanged(int)));
            connect(audioStream, SIGNAL(outputVolumeChanged(int)),
                    this, SIGNAL(outputVolumeChanged(int)));

            audioStream->moveToThread(m_player->soundThread());
            QMetaObject::invokeMethod(audioStream, "startOutput");
            QMetaObject::invokeMethod(audioStream, "startInput");
        }

    } else if (state == SipCall::FinishedState) {
        // stop audio input / output
        QSoundStream *audioStream = m_activeCalls.value(call);
        if (audioStream) {
            audioStream->stopInput();
            audioStream->stopOutput();
        }

        call->disconnect(this);
        m_activeCalls.remove(call);

        emit currentCallsChanged();
        emit inputVolumeChanged(0);
        emit outputVolumeChanged(0);

        // FIXME: ugly workaround for a race condition causing a crash in
        // PhoneNotification.qml
        QTimer::singleShot(1000, call, SLOT(deleteLater()));
    }
}

void PhoneHistoryModel::_q_openUrl(const QUrl &url)
{
    if (url.scheme() != "sip") {
        qWarning("PhoneHistoryModel got a non-SIP URL!");
        return;
    }

    if (!url.path().isEmpty()) {
        const QString phoneNumber = url.path().split('@').first();
        const QString recipient = QString("\"%1\" <%2>").arg(phoneNumber, url.toString());
        call(recipient);
    }
    // FIXME
    //emit showPanel();
}


