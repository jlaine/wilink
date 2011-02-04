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

#include <QAudioInput>
#include <QAudioOutput>
#include <QByteArrayMatcher>
#include <QCoreApplication>
#include <QDateTime>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QPair>
#include <QUdpSocket>
#include <QTimer>

#include "QXmppRtpChannel.h"
#include "QXmppSaslAuth.h"
#include "QXmppSrvInfo.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "sip_p.h"

static const int RTP_COMPONENT = 1;
static const int RTCP_COMPONENT = 2;

#define QXMPP_DEBUG_SIP
#define QXMPP_DEBUG_STUN

#define EXPIRE_SECONDS 3600

#define STUN_RETRY_MS   500
#define STUN_EXPIRE_MS  30000

#define SIP_T1_TIMER 500
#define SIP_T2_TIMER 4000

const char *sipAddressPattern = "(.*)<(sip:([^>]+))>(;.+)?";

enum StunStep {
    StunConnectivity = 0,
    StunChangeServer,
};

QString sipAddressToName(const QString &address)
{
    QRegExp rx(sipAddressPattern);
    if (!rx.exactMatch(address)) {
        qWarning("Bad address %s", qPrintable(address));
        return QString();
    }
    QString name = rx.cap(1).trimmed();
    if (name.startsWith('"') && name.endsWith('"'))
        name = name.mid(1, name.size() - 2);
    return name.isEmpty() ? rx.cap(3).split('@').first() : name;
}

static QString sipAddressToUri(const QString &address)
{
    QRegExp rx(sipAddressPattern);
    if (!rx.exactMatch(address)) {
        qWarning("Bad address %s", qPrintable(address));
        return QString();
    }
    return rx.cap(2);
}

SdpMessage::SdpMessage(const QByteArray &ba)
{
    const QByteArrayMatcher crlf("\r\n");

    int i = 0;
    while (i < ba.count() - 1) {
        const char field = ba[i];
        int j = i + 1;
        if (ba[j] != '=')
            break;
        j++;

        i = crlf.indexIn(ba, j);
        if (i == -1)
            break;

        m_fields.append(qMakePair(field, ba.mid(j, i - j)));
        i += 2;
    }
}

void SdpMessage::addField(char name, const QByteArray &data)
{
    m_fields.append(qMakePair(name, data));
}

QList<QPair<char, QByteArray> > SdpMessage::fields() const
{
    return m_fields;
}

QByteArray SdpMessage::toByteArray() const
{
    QByteArray ba;
    QList<QPair<char, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it) {
        ba += it->first;
        ba += "=";
        ba += it->second;
        ba += "\r\n";
    }
    return ba;
}

SipCallContext::SipCallContext()
    : cseq(1)
{
    id = generateStanzaHash().toLatin1();
    tag = generateStanzaHash(8).toLatin1();
}

bool SipCallContext::handleAuthentication(const SipMessage &reply)
{
    bool isProxy = reply.statusCode() == 407;
    QMap<QByteArray, QByteArray> *lastChallenge = isProxy ? &proxyChallenge : &challenge;

    const QByteArray auth = reply.headerField(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate");
    int spacePos = auth.indexOf(' ');
    if (spacePos < 0 || auth.left(spacePos) != "Digest") {
        qWarning("Unsupported authentication method");
        return false;
    }
    QMap<QByteArray, QByteArray> challenge = QXmppSaslDigestMd5::parseMessage(auth.mid(spacePos + 1));
    if (lastChallenge->value("realm") == challenge.value("realm") &&
        lastChallenge->value("nonce") == challenge.value("nonce")) {
        qWarning("Authentication failed");
        return false;
    }
    *lastChallenge = challenge;
    return true;
}

SipCallPrivate::SipCallPrivate(SipCall *qq)
    : state(QXmppCall::OfferState),
    invitePending(false),
    inviteQueued(false),
    activeTime("0 0"),
    q(qq)
{
}

void SipCallPrivate::handleReply(const SipMessage &reply)
{
    // store information
    if (!reply.headerField("To").isEmpty())
        remoteRecipient = reply.headerField("To");
    if (!reply.headerField("Contact").isEmpty()) {
        const QString contact = QString::fromUtf8(reply.headerField("Contact"));
        remoteUri = sipAddressToUri(contact).toUtf8();
    }
    const QList<QByteArray> recordRoutes = reply.headerFieldValues("Record-Route");
    if (!recordRoutes.isEmpty())
        remoteRoute = recordRoutes;

    // find transaction
    const QMap<QByteArray, QByteArray> viaParams = SipMessage::valueParameters(reply.headerField("Via"));
    foreach (SipTransaction *transaction, transactions) {
        if (transaction->branch() == viaParams.value("branch")) {
            transaction->messageReceived(reply);
            return;
        }
    }

    // if the command was not an INVITE, stop here
    const QByteArray command = reply.headerField("CSeq").split(' ').last();
    if (command != "INVITE")
        return;

    // send ack for final responses
    if  (reply.statusCode() >= 200) {
        invitePending = false;

        SipMessage request = client->d->buildRequest("ACK", remoteUri, this, inviteRequest.sequenceNumber());
        for (int i = remoteRoute.size() - 1; i >= 0; --i)
            request.addHeaderField("Route", remoteRoute[i]);
        request.setHeaderField("To", remoteRecipient);
        request.setHeaderField("Via", inviteRequest.headerField("Via"));
        request.removeHeaderField("Contact");
        client->sendMessage(request);
    }

    // handle authentication
    if (reply.statusCode() == 407) {
        if (handleAuthentication(reply)) {
            SipMessage request = client->d->buildRetry(inviteRequest, this);
            client->sendMessage(request);
            invitePending = true;
            inviteRequest = request;
        } else {
            setState(QXmppCall::FinishedState);
        }
        return;
    }

    // handle invite status
    if (reply.statusCode() == 180)
    {
        emit q->ringing();
    }
    else if (reply.statusCode() == 200)
    {
        q->debug(QString("SIP call %1 established").arg(QString::fromUtf8(id)));
        timeoutTimer->stop();

        if (reply.headerField("Content-Type") == "application/sdp" &&
            handleSdp(SdpMessage(reply.body())))
        {
            setState(QXmppCall::ActiveState);
        } else {
            q->hangup();
        }

    } else if (reply.statusCode() >= 300) {

        q->warning(QString("SIP call %1 failed").arg(
            QString::fromUtf8(id)));
        timeoutTimer->stop();
        setState(QXmppCall::FinishedState);
    }
}

void SipCallPrivate::handleRequest(const SipMessage &request)
{
    // store information
    if (!request.headerField("From").isEmpty())
        remoteRecipient = request.headerField("From");
    if (!request.headerField("Contact").isEmpty()) {
        const QString contact = QString::fromUtf8(request.headerField("Contact"));
        remoteUri = sipAddressToUri(contact).toUtf8();
    }
    const QList<QByteArray> recordRoutes = request.headerFieldValues("Record-Route");
    if (!recordRoutes.isEmpty())
        remoteRoute = recordRoutes;

    // respond
    SipMessage response = client->d->buildResponse(request);
    if (request.method() == "ACK") {
        setState(QXmppCall::ActiveState);
     } else if (request.method() == "BYE") {
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
        client->sendMessage(response);
        setState(QXmppCall::FinishedState);
    } else if (request.method() == "CANCEL") {
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
        client->sendMessage(response);
        setState(QXmppCall::FinishedState);
    } else if (request.method() == "INVITE") {

        if (request.headerField("Content-Type") == "application/sdp" &&
            handleSdp(SdpMessage(request.body())))
        {
            inviteRequest = request;
            response.setStatusCode(180);
            response.setReasonPhrase("Ringing");
        } else {
            response.setStatusCode(400);
            response.setReasonPhrase("Bad request");
        }
        client->sendMessage(response);
    } else {
        response.setStatusCode(405);
        response.setReasonPhrase("Method not allowed");
        client->sendMessage(response);
    }
}

static QString addressToSdp(const QHostAddress &host)
{
    return QString("IN %1 %2").arg(
        host.protocol() == QAbstractSocket::IPv6Protocol ? "IP6" : "IP4",
        host.toString());
}

SdpMessage SipCallPrivate::buildSdp(const QList<QXmppJinglePayloadType> &payloadTypes) const
{
    QXmppJingleCandidate localCandidate;
    foreach (const QXmppJingleCandidate &candidate, iceConnection->localCandidates()) {
        if (candidate.component() == RTP_COMPONENT &&
            candidate.type() == QXmppJingleCandidate::ServerReflexiveType) {
            localCandidate = candidate;
            break;
        }
    }

    static const QDateTime ntpEpoch(QDate(1900, 1, 1));
    quint32 ntpSeconds = ntpEpoch.secsTo(QDateTime::currentDateTime());

    SdpMessage sdp;
    sdp.addField('v', "0");
    sdp.addField('o', QString("- %1 %2 %3").arg(
        QString::number(ntpSeconds),
        QString::number(ntpSeconds),
        addressToSdp(client->d->localAddress)).toUtf8());
    sdp.addField('s', "-");
    sdp.addField('c', addressToSdp(localCandidate.host()).toUtf8());
    sdp.addField('t', activeTime);

    // ICE credentials
    sdp.addField('a', "ice-ufrag:" + iceConnection->localUser().toUtf8());
    sdp.addField('a', "ice-pwd:" + iceConnection->localPassword().toUtf8());

    // RTP profile
    QByteArray profiles = "RTP/AVP";
    QList<QByteArray> attrs;
    foreach (const QXmppJinglePayloadType &payload, payloadTypes) {
        profiles += " " + QByteArray::number(payload.id());
        QByteArray rtpmap = QByteArray::number(payload.id()) + " " + payload.name().toUtf8() + "/" + QByteArray::number(payload.clockrate());
        if (payload.channels() > 1)
            rtpmap += "/" + QByteArray::number(payload.channels());
        attrs << "rtpmap:" + rtpmap;

        // FIXME: make this generic
        if (payload.name() == "telephone-event")
            attrs << "fmtp:" + QByteArray::number(payload.id()) + " 0-15";
    }
    attrs << "sendrecv";

    sdp.addField('m', "audio " + QByteArray::number(localCandidate.port()) + " "  + profiles);
    foreach (const QByteArray &attr, attrs)
        sdp.addField('a', attr);

    // ICE candidates
    foreach (const QXmppJingleCandidate &candidate, iceConnection->localCandidates()) {
        QByteArray ba = "candidate:" + QByteArray::number(candidate.foundation()) + " ";
        ba += QByteArray::number(candidate.component()) + " ";
        ba += candidate.protocol().toUpper().toAscii() + " ";
        ba += QByteArray::number(candidate.priority()) + " ";
        ba += candidate.host().toString().toUtf8() + " ";
        ba += QByteArray::number(candidate.port()) + " ";
        ba += "typ " + QXmppJingleCandidate::typeToString(candidate.type()).toAscii();
        sdp.addField('a', ba);
    }
    return sdp;
}

bool SipCallPrivate::handleSdp(const SdpMessage &sdp)
{
    QList<QXmppJinglePayloadType> payloads;
    QXmppJingleCandidate remoteCandidate;
    remoteCandidate.setComponent(RTP_COMPONENT);
    remoteCandidate.setProtocol("udp");
    remoteCandidate.setType(QXmppJingleCandidate::HostType);

    // parse descriptor
    QPair<char, QByteArray> field;
    foreach (field, sdp.fields()) {
        if (field.first == 'c') {
            // determine remote host
            if (field.second.startsWith("IN IP4 ") || field.second.startsWith("IN IP6 ")) {
                remoteCandidate.setHost(QHostAddress(QString::fromUtf8(field.second.mid(7))));
            }
        } else if (field.first == 'm') {
            QList<QByteArray> bits = field.second.split(' ');
            if (bits.size() < 3 || bits[0] != "audio" || bits[2] != "RTP/AVP")
                continue;

            // determine remote port
            remoteCandidate.setPort(bits[1].toUInt());

            // parse payload types
            for (int i = 3; i < bits.size(); ++i) {
                bool ok = false;
                int id = bits[i].toInt(&ok);
                if (!ok)
                    continue;
                QXmppJinglePayloadType payload;
                payload.setId(id);
                payloads << payload;
            }
        } else if (field.first == 'a') {
            const int pos = field.second.indexOf(':');
            if (pos < 0)
                continue;
            const QByteArray attrName = field.second.left(pos);
            const QByteArray attrValue = field.second.mid(pos+1);

            if (attrName == "ice-ufrag") {
                // ICE user
                iceConnection->setRemoteUser(QString::fromUtf8(attrValue));
            } else if (attrName == "ice-pwd") {
                // ICE password
                iceConnection->setRemotePassword(QString::fromUtf8(attrValue));
            } else if (attrName == "candidate") {
                // ICE candidate
                QList<QByteArray> bits = attrValue.split(' ');
                QXmppJingleCandidate candidate;
                if (bits.size() == 8 && bits[6] == "typ") {
                    candidate.setFoundation(bits[0].toInt());
                    candidate.setComponent(bits[1].toInt());
                    candidate.setProtocol(QString::fromAscii(bits[2]).toLower());
                    candidate.setPriority(bits[3].toInt());
                    candidate.setHost(QHostAddress(QString::fromAscii(bits[4])));
                    candidate.setPort(bits[5].toInt());
                    candidate.setType(QXmppJingleCandidate::typeFromString(QString::fromAscii(bits[7])));

                    iceConnection->addRemoteCandidate(candidate);
                }
            } else if (attrName == "ptime") {
                // packetization time
                bool ok = false;
                int ptime = attrValue.toInt(&ok);
                if (ok && ptime > 0) {
                    q->debug(QString("Setting RTP payload time to %1 ms").arg(QString::number(ptime)));
                    for (int i = 0; i < payloads.size(); ++i)
                        payloads[i].setPtime(ptime);
                }
            } else if (attrName == "rtpmap") {
                // payload type map
                const QList<QByteArray> bits = attrValue.split(' ');
                if (bits.size() != 2)
                    continue;
                bool ok = false;
                const int id = bits[0].toInt(&ok);
                if (!ok)
                    continue;

                const QList<QByteArray> args = bits[1].split('/');
                for (int i = 0; i < payloads.size(); ++i) {
                    if (payloads[i].id() == id) {
                        payloads[i].setName(args[0]);
                        if (args.size() > 1)
                            payloads[i].setClockrate(args[1].toInt());
                    }
                }
            }
        } else if (field.first == 't') {
            // active time
            if (direction == QXmppCall::IncomingDirection)
                activeTime = field.second;
            else if (field.second != activeTime)
                q->warning(QString("Answerer replied with a different active time %1").arg(QString::fromUtf8(activeTime)));
        }
    }

    // add remote candidate
    iceConnection->addRemoteCandidate(remoteCandidate);
    remoteCandidate.setComponent(RTCP_COMPONENT);
    remoteCandidate.setPort(remoteCandidate.port() + 1);
    iceConnection->addRemoteCandidate(remoteCandidate);

    // assign remote payload types
    audioChannel->setRemotePayloadTypes(payloads);
    if (!audioChannel->isOpen()) {
        q->warning("Could not assign codec to RTP channel");
        return false;
    }

    return true;
}

void SipCallPrivate::sendInvite()
{
    const SdpMessage sdp = buildSdp(audioChannel->localPayloadTypes());

    SipMessage request = client->d->buildRequest("INVITE", remoteUri, this, cseq++);
    request.setHeaderField("To", remoteRecipient);
    request.setHeaderField("Content-Type", "application/sdp");
    request.setBody(sdp.toByteArray());
    client->sendMessage(request);
    invitePending = true;
    inviteRequest = request;

    timeoutTimer->start(64 * SIP_T1_TIMER);
}

void SipCallPrivate::setState(QXmppCall::State newState)
{
    if (state != newState)
    {
        state = newState;
        onStateChanged();
        emit q->stateChanged(state);

        if (state == QXmppCall::ActiveState)
            emit q->connected();
        else if (state == QXmppCall::FinishedState)
        {
            q->debug(QString("SIP call %1 finished").arg(QString::fromUtf8(id)));
            emit q->finished();
        }
    }
}

void SipCallPrivate::onStateChanged()
{
    if (state == QXmppCall::ActiveState)
    {
        startTime.start();

        // prepare audio format
        QAudioFormat format;
        format.setFrequency(audioChannel->payloadType().clockrate());
        format.setChannels(audioChannel->payloadType().channels());
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        // 128ms at 8kHz
        const int bufferSize = 2048 * format.channels();

        // initialise audio output
        if (!audioOutput) {
            QTime tm;
            tm.start();
            audioOutput = new QAudioOutput(format, q);
            QObject::connect(audioOutput, SIGNAL(stateChanged(QAudio::State)),
                             q, SLOT(audioStateChanged()));
            audioOutput->setBufferSize(bufferSize);
            audioOutput->start(audioChannel);
            q->debug(QString("Audio output initialized in %1 ms").arg(QString::number(tm.elapsed())));
            q->debug(QString("Audio output buffer size %1 (asked for %2)").arg(QString::number(audioOutput->bufferSize()), QString::number(bufferSize)));
        }

        // initialise audio input
        if (!audioInput) {
            QTime tm;
            tm.start();
            audioInput = new QAudioInput(format, q);
            QObject::connect(audioInput, SIGNAL(stateChanged(QAudio::State)),
                             q, SLOT(audioStateChanged()));
            audioInput->setBufferSize(bufferSize);
            audioInput->start(audioChannel);
            q->debug(QString("Audio input initialized in %1 ms").arg(QString::number(tm.elapsed())));
            q->debug(QString("Audio input buffer size %1 (asked for %2)").arg(QString::number(audioInput->bufferSize()), QString::number(bufferSize)));
        }

    } else {
        if (startTime.isValid() && !stopTime.isValid())
            stopTime.start();

        // stop audio input
        if (audioInput)
        {
            audioInput->stop();
            delete audioInput;
            audioInput = 0;
        }

        // stop audio output
        if (audioOutput)
        {
            audioOutput->stop();
            delete audioOutput;
            audioOutput = 0;
        }
    }
}

SipCall::SipCall(const QString &recipient, QXmppCall::Direction direction, SipClient *parent)
    : QXmppLoggable(parent)
{
    bool check;

    d = new SipCallPrivate(this);
    d->client = parent;
    d->direction = direction;
    d->inviteQueued = (direction == QXmppCall::OutgoingDirection);
    d->remoteRecipient = recipient.toUtf8();
    d->remoteUri = sipAddressToUri(recipient).toUtf8();

    d->audioChannel = new QXmppRtpChannel(this);
    d->audioInput = 0;
    d->audioOutput = 0;

    // bind sockets
    bool iceControlling = (d->direction == QXmppCall::OutgoingDirection);
    d->iceConnection = new QXmppIceConnection(iceControlling, this);
    d->iceConnection->addComponent(RTP_COMPONENT);
    d->iceConnection->addComponent(RTCP_COMPONENT);
    d->iceConnection->setStunServer(d->client->d->stunServerAddress,
                                    d->client->d->stunServerPort);
    check = connect(d->iceConnection, SIGNAL(localCandidatesChanged()),
                    this, SLOT(localCandidatesChanged()));
    Q_ASSERT(check);

    // setup RTP transport
    QXmppIceComponent *rtpComponent = d->iceConnection->component(RTP_COMPONENT);
    check = connect(rtpComponent, SIGNAL(datagramReceived(QByteArray)),
                    d->audioChannel, SLOT(datagramReceived(QByteArray)));
    Q_ASSERT(check);

    check = connect(d->audioChannel, SIGNAL(sendDatagram(QByteArray)),
                    rtpComponent, SLOT(sendDatagram(QByteArray)));
    Q_ASSERT(check);

    // Timer B
    d->timeoutTimer = new QTimer(this);
    d->timeoutTimer->setSingleShot(true);
    check = connect(d->timeoutTimer, SIGNAL(timeout()),
                    this, SLOT(handleTimeout()));
    Q_ASSERT(check);

    // start ICE
    if (!d->iceConnection->bind(QList<QHostAddress>() << d->client->d->localAddress))
        warning("Could not start listening for RTP");
}

SipCall::~SipCall()
{
    delete d;
}

/// Call this method if you wish to accept an incoming call.
///

void SipCall::accept()
{
    if (d->direction == QXmppCall::IncomingDirection && d->state == QXmppCall::OfferState)
    {
#if 0
        QByteArray rtcp;
        QDataStream stream(&rtcp, QIODevice::WriteOnly);

        // receiver report
        stream << quint8(0x80);
        stream << quint8(0xc9); // receiver report
        stream << quint16(1);   // length
        stream << d->audioChannel->synchronizationSource();

        // source description
        stream << quint8(0x81);
        stream << quint8(0xca); // source description
        stream << quint16(2);   // length
        stream << d->audioChannel->synchronizationSource();
        stream << quint8(1); // cname
        stream << quint8(0);
        stream << quint8(0); // end
        stream << quint8(0);
        d->rtcpSocket->writeDatagram(rtcp, d->remoteHost, d->remotePort+1);
#endif

        const SdpMessage sdp = d->buildSdp(d->audioChannel->localPayloadTypes());

        SipMessage response = d->client->d->buildResponse(d->inviteRequest);
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
        response.setHeaderField("Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE");
        response.setHeaderField("Supported", "replaces");
        response.setHeaderField("Content-Type", "application/sdp");
        response.setBody(sdp.toByteArray());
        d->client->sendMessage(response);

        d->setState(QXmppCall::ConnectingState);
    }
}

QXmppRtpChannel *SipCall::audioChannel() const
{
    return d->audioChannel;
}

void SipCall::audioStateChanged()
{
    QObject *audio = sender();
    if (!audio)
        return;
    else if (audio == d->audioInput) {
        debug(QString("Audio input state %1 error %2").arg(
            QString::number(d->audioInput->state()),
            QString::number(d->audioInput->error())));

        // restart audio input if we get an underrun
        if (d->audioInput->state() == QAudio::StoppedState &&
            d->audioInput->error() == QAudio::UnderrunError)
        {
            warning("Audio input needs restart due to buffer underrun");
            d->audioInput->start(d->audioChannel);
        }
    }
    else if (audio == d->audioOutput) {
        debug(QString("Audio output state %1 error %2").arg(
            QString::number(d->audioOutput->state()),
            QString::number(d->audioOutput->error())));

        // restart audio output if we get an underrun
        if (d->audioOutput->state() == QAudio::StoppedState &&
            d->audioOutput->error() == QAudio::UnderrunError)
        {
            warning("Audio output needs restart due to buffer underrun");
            d->audioOutput->start(d->audioChannel);
        }
    }
}

/// Returns the call's direction.
///

QXmppCall::Direction SipCall::direction() const
{
    return d->direction;
}

/// Return's the call's duration in seconds.
///

int SipCall::duration() const
{
    if (d->startTime.isValid()) {
        if (d->stopTime.isValid())
            return d->startTime.secsTo(d->stopTime);
        else
            return d->startTime.secsTo(QTime::currentTime());
    }
    return 0;
}

QByteArray SipCall::id() const
{
    return d->id;
}

void SipCall::localCandidatesChanged()
{
    // check whether we have server-reflexive candidates for all components
    bool foundRtp = false;
    bool foundRtcp = false;
    foreach (const QXmppJingleCandidate &candidate, d->iceConnection->localCandidates()) {
        if (candidate.type() == QXmppJingleCandidate::ServerReflexiveType) {
            if (candidate.component() == RTP_COMPONENT)
                foundRtp = true;
            else if (candidate.component() == RTCP_COMPONENT)
                foundRtcp = true;
        }
    }

    // send INVITE if required
    if (d->inviteQueued && foundRtp && foundRtcp) {
        d->sendInvite();
        d->inviteQueued = false;
    }
}

QString SipCall::recipient() const
{
    return QString::fromUtf8(d->remoteRecipient);
}

QXmppCall::State SipCall::state() const
{
    return d->state;
}

void SipCall::transactionFinished()
{
    SipTransaction *transaction = qobject_cast<SipTransaction*>(sender());
    if (!transaction || !d->transactions.removeAll(transaction))
        return;
    transaction->deleteLater();

    const QByteArray method = transaction->request().method();
    if (method == "BYE") {
        if (d->invitePending) {
            SipMessage request = d->client->d->buildRequest("CANCEL", d->inviteRequest.uri(), d, d->inviteRequest.sequenceNumber());
            request.setHeaderField("To", d->inviteRequest.headerField("To"));
            request.setHeaderField("Via", d->inviteRequest.headerField("Via"));
            request.removeHeaderField("Contact");
            d->transactions << new SipTransaction(request, d->client, this);
        } else {
            d->setState(QXmppCall::FinishedState);
        }
    }
    else if (method == "CANCEL") {
        d->setState(QXmppCall::FinishedState);
    }
}

void SipCall::handleTimeout()
{
    warning(QString("SIP call %1 timed out").arg(QString::fromUtf8(d->id)));
    d->setState(QXmppCall::FinishedState);
}

/// Hangs up the call.
///

void SipCall::hangup()
{
    if (d->state == QXmppCall::DisconnectingState ||
        d->state == QXmppCall::FinishedState)
        return;

    debug(QString("SIP call %1 hangup").arg(
            QString::fromUtf8(d->id)));
    d->setState(QXmppCall::DisconnectingState);
    d->iceConnection->close();
    d->timeoutTimer->stop();

    SipMessage request = d->client->d->buildRequest("BYE", d->remoteUri, d, d->cseq++);
    request.setHeaderField("To", d->remoteRecipient);
    for (int i = d->remoteRoute.size() - 1; i >= 0; --i)
        request.addHeaderField("Route", d->remoteRoute[i]);
    d->transactions << new SipTransaction(request, d->client, this);
}

SipClientPrivate::SipClientPrivate(SipClient *qq)
    : state(SipClient::DisconnectedState),
    contactPort(0),
    stunCookie(0),
    stunDone(false),
    stunReflexivePort(0),
    stunServerPort(0),
    q(qq)
{
}

QByteArray SipClientPrivate::authorization(const SipMessage &request, const QMap<QByteArray, QByteArray> &challenge) const
{
    QXmppSaslDigestMd5 digest;
    digest.setCnonce(digest.generateNonce());
    digest.setNc("00000001");
    digest.setNonce(challenge.value("nonce"));
    digest.setQop(challenge.value("qop"));
    digest.setRealm(challenge.value("realm"));
    digest.setUsername(username.toUtf8());
    digest.setPassword(password.toUtf8());

    const QByteArray A1 = digest.username() + ':' + digest.realm() + ':' + password.toUtf8();
    const QByteArray A2 = request.method() + ':' + request.uri();

    QMap<QByteArray, QByteArray> response;
    response["username"] = digest.username();
    response["realm"] = digest.realm();
    response["nonce"] = digest.nonce();
    response["uri"] = request.uri();
    response["response"] = digest.calculateDigest(A1, A2);
    if (challenge.contains("qop")) {
        response["qop"] = digest.qop();
        response["cnonce"] = digest.cnonce();
        response["nc"] = digest.nc();
    }
    response["algorithm"] = "MD5";
    if (challenge.contains("opaque"))
        response["opaque"] = challenge.value("opaque");

    return QByteArray("Digest ") + QXmppSaslDigestMd5::serializeMessage(response);
}

SipMessage SipClientPrivate::buildRequest(const QByteArray &method, const QByteArray &uri, SipCallContext *ctx, int seqNum)
{
    QString addr;
    if (!displayName.isEmpty())
        addr += QString("\"%1\"").arg(displayName);
    addr += QString("<sip:%1@%2>").arg(username, domain);

    const QString branch = "z9hG4bK-" + generateStanzaHash();
    const QString host = QString("%1:%2").arg(
        localAddress.toString(),
        QString::number(socket->localPort()));
    const QString via = QString("SIP/2.0/UDP %1;branch=%2;rport").arg(
        host, branch);

    SipMessage packet;
    packet.setMethod(method);
    packet.setUri(uri);
    packet.setHeaderField("Via", via.toUtf8());
    packet.setHeaderField("Max-Forwards", "70");
    packet.setHeaderField("Call-ID", ctx->id);
    packet.setHeaderField("CSeq", QByteArray::number(seqNum) + ' ' + method);
    setContact(packet);
    packet.setHeaderField("To", addr.toUtf8());
    packet.setHeaderField("From", addr.toUtf8() + ";tag=" + ctx->tag);

    // authentication
    if (!ctx->challenge.isEmpty())
        packet.setHeaderField("Authorization", authorization(packet, ctx->challenge));
    if (!ctx->proxyChallenge.isEmpty())
        packet.setHeaderField("Proxy-Authorization", authorization(packet, ctx->proxyChallenge));

    packet.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());
    if (method != "ACK" && method != "CANCEL")
        packet.setHeaderField("Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE");
    return packet;
}

SipMessage SipClientPrivate::buildResponse(const SipMessage &request)
{
    SipMessage response;
    foreach (const QByteArray &via, request.headerFieldValues("Via"))
        response.addHeaderField("Via", via);
    response.setHeaderField("From", request.headerField("From"));
    response.setHeaderField("To", request.headerField("To"));
    response.setHeaderField("Call-ID", request.headerField("Call-ID"));
    response.setHeaderField("CSeq", request.headerField("CSeq"));
    foreach (const QByteArray &route, request.headerFieldValues("Record-Route"))
        response.addHeaderField("Record-Route", route);
    setContact(response);
    response.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());
    return response;
}

SipMessage SipClientPrivate::buildRetry(const SipMessage &original, SipCallContext *ctx)
{
    SipMessage request = original;

    request.setHeaderField("CSeq", QByteArray::number(ctx->cseq++) + ' ' + request.method());
    if (!ctx->challenge.isEmpty())
        request.setHeaderField("Authorization", authorization(request, ctx->challenge));
    if (!ctx->proxyChallenge.isEmpty())
        request.setHeaderField("Proxy-Authorization", authorization(request, ctx->proxyChallenge));
    setContact(request);

    return request;
}

void SipClientPrivate::handleReply(const SipMessage &reply)
{
    // store information
    const QList<QByteArray> vias = reply.headerFieldValues("Via");
    if (!vias.isEmpty()) {
        QMap<QByteArray, QByteArray> params = SipMessage::valueParameters(vias.first());
        if (params.contains("received"))
            contactAddress = QHostAddress(QString::fromLatin1(params.value("received")));
        if (params.contains("rport"))
            contactPort = params.value("rport").toUInt();
    }

    // find transaction
    const QMap<QByteArray, QByteArray> viaParams = SipMessage::valueParameters(reply.headerField("Via"));
    foreach (SipTransaction *transaction, transactions) {
        if (transaction->branch() == viaParams.value("branch")) {
            transaction->messageReceived(reply);
            return;
        }
    }
}

void SipClientPrivate::setContact(SipMessage &request)
{
    request.setHeaderField("Contact", QString("<sip:%1@%2:%3>").arg(
        username,
        contactAddress.toString(),
        QString::number(contactPort)).toUtf8());
}

void SipClientPrivate::setState(SipClient::State newState)
{
    if (state != newState)
    {
        state = newState;
        emit q->stateChanged(state);

        if (state == SipClient::ConnectedState)
            emit q->connected();
        else if (state == SipClient::DisconnectedState)
            emit q->disconnected();
    }
}

SipClient::SipClient(QObject *parent)
    : QXmppLoggable(parent)
{
    qRegisterMetaType<SipClient::State>("SipClient::State");
    qRegisterMetaType<SipMessage>("SipMessage");

    d = new SipClientPrivate(this);
    d->socket = new QUdpSocket(this);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(datagramReceived()));

    d->connectTimer = new QTimer(this);
    connect(d->connectTimer, SIGNAL(timeout()),
            this, SLOT(connectToServer()));

    d->stunTimer = new QTimer(this);
    d->stunTimer->setSingleShot(true);
    connect(d->stunTimer, SIGNAL(timeout()),
            this, SLOT(sendStun()));
}

SipClient::~SipClient()
{
    if (d->state == ConnectedState)
        disconnectFromServer();
    delete d;
}

SipCall *SipClient::call(const QString &recipient)
{
    info(QString("SIP call to %1").arg(recipient));

    if (d->state != ConnectedState) {
        warning("Cannot dial call, not connected to server");
        return 0;
    }

    // construct call
    SipCall *call = new SipCall(recipient, QXmppCall::OutgoingDirection, this);

    // register call
    connect(call, SIGNAL(destroyed(QObject*)),
            this, SLOT(callDestroyed(QObject*)));
    d->calls << call;

    emit callDialled(call);
    return call;
}

void SipClient::callDestroyed(QObject *object)
{
    d->calls.removeAll(static_cast<SipCall*>(object));
}

void SipClient::connectToServer()
{
    // schedule retry
    d->connectTimer->start(60000);

    // listen for SIP
    if (d->socket->state() == QAbstractSocket::UnconnectedState) {
        if (!d->socket->bind()) {
            warning("Could not start listening for SIP");
            return;
        }
        debug(QString("Listening for SIP on port %1").arg(
            QString::number(d->socket->localPort())));
    }

    // perform DNS SRV lookups
    debug(QString("Looking up STUN server for domain %1").arg(d->domain));
    QXmppSrvInfo::lookupService("_stun._udp." + d->domain, this,
                                SLOT(setStunServer(QXmppSrvInfo)));

    debug(QString("Looking up SIP server for domain %1").arg(d->domain));
    QXmppSrvInfo::lookupService("_sip._udp." + d->domain, this,
                                SLOT(setSipServer(QXmppSrvInfo)));
}

void SipClient::datagramReceived()
{
    if (!d->socket->hasPendingDatagrams())
        return;

    // receive datagram
    const qint64 size = d->socket->pendingDatagramSize();
    QByteArray buffer(size, 0);
    QHostAddress remoteHost;
    quint16 remotePort;
    d->socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    // check whether it's a STUN packet
    quint32 messageCookie;
    QByteArray messageId;
    quint16 messageType = QXmppStunMessage::peekType(buffer, messageCookie, messageId);
    if (messageType && messageCookie == d->stunCookie && messageId == d->stunId)
    {
        QXmppStunMessage message;
        if (!message.decode(buffer))
            return;

#ifdef QXMPP_DEBUG_STUN
        logReceived(QString("STUN packet from %1 port %2\n%3").arg(
            remoteHost.toString(),
            QString::number(remotePort),
            message.toString()));
#endif

        const QHostAddress oldReflexiveAddress = d->stunReflexiveAddress;
        const quint16 oldReflexivePort = d->stunReflexivePort;

        // store reflexive address
        if (!message.xorMappedHost.isNull() && message.xorMappedPort != 0) {
            d->stunReflexiveAddress = message.xorMappedHost;
            d->stunReflexivePort = message.xorMappedPort;
        } else if (!message.mappedHost.isNull() && message.mappedPort != 0) {
            d->stunReflexiveAddress = message.mappedHost;
            d->stunReflexivePort = message.mappedPort;
        }

        // check whether the reflexive address has changed
        bool doRegister = false;
        if (d->stunReflexiveAddress != oldReflexiveAddress || d->stunReflexivePort != oldReflexivePort) {
            debug(QString("STUN reflexive address changed to %1 port %2").arg(
                d->stunReflexiveAddress.toString(),
                QString::number(d->stunReflexivePort)));

            // update contact address
            d->contactAddress = d->stunReflexiveAddress;
            d->contactPort = d->stunReflexivePort;

            // update local address
            const QList<QHostAddress> addresses = QXmppIceComponent::discoverAddresses();
            foreach (const QHostAddress &address, addresses) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                    d->localAddress = address;
                    break;
                }
            }

            // clear credentials
            d->challenge.clear();
            d->proxyChallenge.clear();

            doRegister = true;
        }
        d->stunDone = true;

        if (doRegister)
            registerWithServer();

        d->stunTimer->start(STUN_EXPIRE_MS);
        return;
    }

#ifdef QXMPP_DEBUG_SIP
    logReceived(QString("SIP packet from %1\n%2").arg(remoteHost.toString(), QString::fromUtf8(buffer)));
#endif

    // parse packet
    SipMessage reply(buffer);

    // find corresponding call
    SipCall *currentCall = 0;
    const QByteArray callId = reply.headerField("Call-ID");
    if (callId != d->id)
    {
        foreach (SipCall *potentialCall, d->calls)
        {
            if (potentialCall->id() == callId)
            {
                currentCall = potentialCall;
                break;
            }
        }
    }

    // check whether it's a request or a response
    if (reply.isRequest()) {
        bool emitCall = false;
        if (!currentCall && reply.method() == "INVITE") {
            const QByteArray from = reply.headerField("From");
            const QByteArray to = reply.headerField("To");
            info(QString("SIP call from %1").arg(QString::fromUtf8(from)));

            // construct call
            currentCall = new SipCall(from, QXmppCall::IncomingDirection, this);
            currentCall->d->id = reply.headerField("Call-ID");

            QMap<QByteArray, QByteArray> params = SipMessage::valueParameters(to);
            if (params.contains("tag")) {
                currentCall->d->tag = params.value("tag");
            } else {
                reply.setHeaderField("To", to + ";tag=" + currentCall->d->tag);
            }

            // register call
            connect(currentCall, SIGNAL(destroyed(QObject*)),
                    this, SLOT(callDestroyed(QObject*)));
            d->calls << currentCall;
            emitCall = true;
        }
        if (currentCall) {
            currentCall->d->handleRequest(reply);
            if (emitCall)
                emit callReceived(currentCall);
        }
    } else if (reply.isReply()) {
        if (currentCall)
            currentCall->d->handleReply(reply);
        else if (callId == d->id)
            d->handleReply(reply);
    } else {
        //warning("SIP packet is neither request nor reply");
    }
}

void SipClient::disconnectFromServer()
{
    // stop timers
    d->connectTimer->stop();
    d->stunTimer->stop();
    d->stunDone = false;

    // terminate calls
    foreach (SipCall *call, d->calls)
        call->hangup();

    // unregister
    if (d->state == SipClient::ConnectedState) {
        debug(QString("Disconnecting from SIP server %1:%2").arg(d->serverAddress.toString(), QString::number(d->serverPort)));
        const QByteArray uri = QString("sip:%1").arg(d->domain).toUtf8();
        SipMessage request = d->buildRequest("REGISTER", uri, d, d->cseq++);
        request.setHeaderField("Contact", request.headerField("Contact") + ";expires=0");
        d->transactions << new SipTransaction(request, this, this);

        d->setState(DisconnectingState);
    } else {
        d->setState(DisconnectedState);
    }

}

void SipClient::registerWithServer()
{
    if (d->serverAddress.isNull() || !d->serverPort)
        return;

    // register
    debug(QString("Connecting to SIP server %1:%2").arg(d->serverAddress.toString(), QString::number(d->serverPort)));

    const QByteArray uri = QString("sip:%1").arg(d->domain).toUtf8();
    SipMessage request = d->buildRequest("REGISTER", uri, d, d->cseq++);
    request.setHeaderField("Expires", QByteArray::number(EXPIRE_SECONDS));
    d->transactions << new SipTransaction(request, this, this);

    d->setState(ConnectingState);
}

/** Send a SIP message.
 *
 * @param message
 */
void SipClient::sendMessage(const SipMessage &message)
{
#ifdef QXMPP_DEBUG_SIP
    logSent(QString("SIP packet to %1:%2\n%3").arg(
            d->serverAddress.toString(),
            QString::number(d->serverPort),
            QString::fromUtf8(message.toByteArray())));
#endif
    d->socket->writeDatagram(message.toByteArray(), d->serverAddress, d->serverPort);
}

/** Send a STUN binding request.
 */
void SipClient::sendStun()
{
    QXmppStunMessage request;
    request.setType(QXmppStunMessage::Binding | QXmppStunMessage::Request);

    d->stunCookie = request.cookie();
    d->stunId = request.id();

#ifdef QXMPP_DEBUG_STUN
    logSent(QString("STUN packet to %1 port %2\n%3").arg(d->stunServerAddress.toString(),
            QString::number(d->stunServerPort), request.toString()));
#endif
    d->socket->writeDatagram(request.encode(QString(), false), d->stunServerAddress, d->stunServerPort);
    d->stunTimer->start(STUN_RETRY_MS);
}

void SipClient::setSipServer(const QHostInfo &hostInfo)
{
    if (hostInfo.addresses().isEmpty()) {
        warning(QString("Could not lookup SIP server %1").arg(hostInfo.hostName()));
        return;
    }
    d->serverAddress = hostInfo.addresses().first();

    if (d->stunDone)
        registerWithServer();
}

void SipClient::setSipServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName = "sip." + d->domain;
    d->serverPort = 5060;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        d->serverPort = serviceInfo.records().first().port();
    }

    // lookup SIP host name
    QHostInfo::lookupHost(serverName, this, SLOT(setSipServer(QHostInfo)));
}

void SipClient::setStunServer(const QHostInfo &hostInfo)
{
    if (hostInfo.addresses().isEmpty()) {
        warning(QString("Could not lookup STUN server %1").arg(hostInfo.hostName()));
        return;
    }
    d->stunServerAddress = hostInfo.addresses().first();

    // send STUN binding request
    sendStun();
}

void SipClient::setStunServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName = "stun." + d->domain;
    d->stunServerPort = 5060;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        d->stunServerPort = serviceInfo.records().first().port();
    }

    // lookup STUN host name
    QHostInfo::lookupHost(serverName, this, SLOT(setStunServer(QHostInfo)));
}

SipClient::State SipClient::state() const
{
    return d->state;
}

void SipClient::transactionFinished()
{
    SipTransaction *transaction = qobject_cast<SipTransaction*>(sender());
    if (!transaction || !d->transactions.removeAll(transaction))
        return;
    transaction->deleteLater();

    // handle authentication
    const SipMessage reply = transaction->response();
    if (reply.statusCode() == 401 &&
        d->handleAuthentication(reply))
    {
        SipMessage request = d->buildRetry(transaction->request(), d);
        d->transactions << new SipTransaction(request, this, this);
        return;
    }

    const QByteArray method = transaction->request().method();
    if (method == "REGISTER") {
        if (reply.statusCode() == 200) {
            if (d->state == SipClient::DisconnectingState) {
                d->setState(SipClient::DisconnectedState);
            } else {
                d->connectTimer->stop();
                d->setState(SipClient::ConnectedState);

                QList<QByteArray> contacts = reply.headerFieldValues("Contact");
                const QByteArray expectedContact = transaction->request().headerField("Contact");
                foreach (const QByteArray &contact, contacts) {
                    if (contact.startsWith(expectedContact)) {
                        // schedule next register
                        QMap<QByteArray, QByteArray> params = SipMessage::valueParameters(contact);
                        const int marginSeconds = 10;
                        const int registerSeconds = params.value("expires").toInt();
                        if (registerSeconds > marginSeconds) {
                            debug(QString("Re-registering in %1 seconds").arg(registerSeconds - marginSeconds));
                            QTimer::singleShot((registerSeconds - marginSeconds) * 1000, this, SLOT(registerWithServer()));
                        }
                        break;
                    }
                }

#if 0
                // send subscribe
                const QByteArray uri = QString("sip:%1@%2").arg(username, domain).toUtf8();
                SipMessage request = buildRequest("SUBSCRIBE", uri, this, cseq++);
                request.setHeaderField("Expires", QByteArray::number(EXPIRE_SECONDS));
                d->transactions << new SipTransaction(request, this, this);
#endif
            }
        } else {
            warning("Register failed");
            if (d->state != SipClient::DisconnectingState)
                d->connectTimer->start();
            d->setState(SipClient::DisconnectedState);
        }
    }
    else if (method == "SUBSCRIBE") {
        if (reply.statusCode() != 200) {
            warning("Subscribe failed");
        }
    }
}

QString SipClient::displayName() const
{
    return d->displayName;
}

void SipClient::setDisplayName(const QString &displayName)
{
    d->displayName = displayName;
}

QString SipClient::domain() const
{
    return d->domain;
}

void SipClient::setDomain(const QString &domain)
{
    d->domain = domain;
}

QString SipClient::password() const
{
    return d->password;
}

void SipClient::setPassword(const QString &password)
{
    d->password = password;
}

QString SipClient::username() const
{
    return d->username;
}

void SipClient::setUsername(const QString &username)
{
    d->username = username;
}

SipMessage::SipMessage(const QByteArray &bytes)
    : m_statusCode(0)
{
    const QByteArrayMatcher crlf("\r\n");
    const QByteArrayMatcher colon(":");

    int j = crlf.indexIn(bytes);
    if (j < 0)
        return;

    // parse status
    const QByteArray status = bytes.left(j);
    if (status.size() >= 10 && status.startsWith("SIP/2.0 ")) {
        m_statusCode = status.mid(8, 3).toInt();
        m_reasonPhrase = QString::fromUtf8(status.mid(12));
    }
    else if (status.size() > 10 && status.endsWith(" SIP/2.0")) {
        int n = status.indexOf(' ');
        m_method = status.left(n);
        m_uri = status.mid(n + 1, j - n - 9);
    } else {
        j = -1;
    }

    // parse headers
    const QByteArray header = bytes.mid(j+1);
    int i = 0;
    while (i < header.count()) {
        int n = crlf.indexIn(header, i);

        if (n < 0) {
            // something is wrong
            qWarning("Missing end of line in SIP header");
            return;
        }
        else if (n == i) {
            // end of header
            i = n + 2;
            break;
        }

        // parse header field
        int j = colon.indexIn(header, i);
        if (j == -1)
            break;
        QByteArray field = header.mid(i, j - i).trimmed();
        j++;
        QByteArray value = header.mid(j, n - j).trimmed();

        // expand shortcuts
        if (field == "c")
            field = "Content-Type";
        else if (field == "f")
            field = "From";
        else if (field == "i")
            field = "Call-ID";
        else if (field == "k")
            field = "Supported";
        else if (field == "l")
            field = "Content-Length";
        else if (field == "m")
            field = "Contact";
        else if (field == "t")
            field = "To";
        else if (field == "v")
            field = "Via";
        m_fields.append(qMakePair(field, value));

        i = n + 2;
    }
    if (i >= 0)
        m_body = header.mid(i);
}

/** Returns the concatenated values for a header.
 *
 * @param name
 */
QByteArray SipMessage::headerField(const QByteArray &name) const
{
    QList<QByteArray> allValues = headerFieldValues(name);

    QByteArray result;
    bool first = true;
    foreach (const QByteArray &value, allValues) {
        if (!first)
            result += ", ";
        first = false;
        result += value;
    }
    return result;
}

/** Returns the values for a header.
 *
 * @param name
 */
QList<QByteArray> SipMessage::headerFieldValues(const QByteArray &name) const
{
    QList<QByteArray> result;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it) {
        if (qstricmp(name.constData(), it->first) == 0) {
            QList<QByteArray> bits = it->second.split(',');
            foreach (const QByteArray &bit, bits)
                result += bit.trimmed();
        }
    }

    return result;
}

/** Returns the parameters for a field value.
 *
 * @param value
 */
QMap<QByteArray, QByteArray> SipMessage::valueParameters(const QByteArray &value)
{
    QMap<QByteArray, QByteArray> params;
    // FIXME: this is a very, very naive implementation
    QList<QByteArray> bits = value.split(';');
    if (bits.size() > 1)
    {
        bits.removeFirst();
        foreach (const QByteArray &bit, bits)
        {
            int i = bit.indexOf('=');
            if (i >= 0)
                params[bit.left(i)] = bit.mid(i+1);
        }
    }
    return params;
}

void SipMessage::addHeaderField(const QByteArray &name, const QByteArray &data)
{
    m_fields.append(qMakePair(name, data));
}

void SipMessage::removeHeaderField(const QByteArray &name)
{
    QList<QPair<QByteArray, QByteArray> >::Iterator it = m_fields.begin();
    while (it != m_fields.end()) {
        if (qstricmp(name.constData(), it->first) == 0)
            it = m_fields.erase(it);
        else
            ++it;
    }
}

void SipMessage::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    removeHeaderField(name);
    m_fields.append(qMakePair(name, data));
}

bool SipMessage::isReply() const
{
    return m_statusCode != 0;
}

bool SipMessage::isRequest() const
{
    return !m_method.isEmpty() && !m_uri.isEmpty();
}

QByteArray SipMessage::body() const
{
    return m_body;
}

void SipMessage::setBody(const QByteArray &body)
{
    m_body = body;
}

QByteArray SipMessage::method() const
{
    return m_method;
}

void SipMessage::setMethod(const QByteArray &method)
{
    m_method = method;
}

QByteArray SipMessage::uri() const
{
    return m_uri;
}

void SipMessage::setUri(const QByteArray &uri)
{
    m_uri = uri;
}

QString SipMessage::reasonPhrase() const
{
    return m_reasonPhrase;
}

void SipMessage::setReasonPhrase(const QString &reasonPhrase)
{
    m_reasonPhrase = reasonPhrase;
}

quint32 SipMessage::sequenceNumber() const
{
    return headerField("CSeq").split(' ').first().toUInt();
}

int SipMessage::statusCode() const
{
    return m_statusCode;
}

void SipMessage::setStatusCode(int statusCode)
{
    m_statusCode = statusCode;
}

QByteArray SipMessage::toByteArray() const
{
    QByteArray ba;

    if (!m_method.isEmpty()) {
        ba += m_method;
        ba += ' ';
        ba += m_uri;
        ba += " SIP/2.0\r\n";
    } else {
        ba += "SIP/2.0 ";
        ba += QByteArray::number(m_statusCode);
        ba += ' ';
        ba += m_reasonPhrase.toUtf8();
        ba += "\r\n";
    }

    bool hasLength = false;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it) {
        if (qstricmp("Content-Length", it->first) == 0)
            hasLength = true;
        ba += it->first;
        ba += ": ";
        ba += it->second;
        ba += "\r\n";
    }
    if (!hasLength)
        ba += "Content-Length: " + QByteArray::number(m_body.size()) + "\r\n";

    ba += "\r\n";
    return ba + m_body;
}

SipTransaction::SipTransaction(const SipMessage &request, SipClient *client, QObject *parent)
    : QXmppLoggable(parent),
    m_request(request),
    m_state(Trying)
{
    bool check;

    check = connect(this, SIGNAL(sendMessage(SipMessage)),
                    client, SLOT(sendMessage(SipMessage)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(finished()),
                    parent, SLOT(transactionFinished()));
    Q_ASSERT(check);

    // Timer F
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    check = connect(m_timeoutTimer, SIGNAL(timeout()),
                    this, SLOT(timeout()));
    Q_ASSERT(check);
    m_timeoutTimer->start(64 * SIP_T1_TIMER);

    // Timer E
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    check = connect(m_retryTimer, SIGNAL(timeout()),
                    this, SLOT(retry()));
    Q_ASSERT(check);

    // Send packet immediately
    client->sendMessage(m_request);
    m_retryTimer->start(SIP_T1_TIMER);
}

QByteArray SipTransaction::branch() const
{
    const QMap<QByteArray, QByteArray> params = SipMessage::valueParameters(m_request.headerField("Via"));
    return params.value("branch");
}

void SipTransaction::messageReceived(const SipMessage &message)
{
    if (message.statusCode() < 200) {
        if (m_state == Trying) {
            m_retryTimer->start(SIP_T2_TIMER);
            m_state = Proceeding;
        }
    } else {
        if (m_state == Trying || m_state == Proceeding) {
            m_retryTimer->stop();
            m_timeoutTimer->stop();
            m_response = message;
            m_state = Completed;
            emit finished();
        }
    }
}

SipMessage SipTransaction::request() const
{
    return m_request;
}

SipMessage SipTransaction::response() const
{
    return m_response;
}

SipTransaction::State SipTransaction::state() const
{
    return m_state;
}

void SipTransaction::retry()
{
    emit sendMessage(m_request);

    // schedule next retry
    m_retryTimer->start(qMin(2 * m_retryTimer->interval(), SIP_T2_TIMER));
}

void SipTransaction::timeout()
{
    warning(QString("%1 transaction timed out").arg(QString::fromUtf8(m_request.method())));
    m_retryTimer->stop();
    m_state = Terminated;
    emit finished();
}
