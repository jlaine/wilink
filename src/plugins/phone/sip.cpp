/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
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

const int RTP_COMPONENT = 1;
const int RTCP_COMPONENT = 2;

#define QXMPP_DEBUG_SIP
#define QXMPP_DEBUG_STUN

#define EXPIRE_SECONDS 1800
#define TIMEOUT_SECONDS 30
#define MARGIN_SECONDS 10

static const char *addressPattern = "(.*)<([^>]+)>(;.+)?";

enum StunStep {
    StunConnectivity = 0,
    StunChangeServer,
    StunChangePort,
    StunChangeSocket,
};

QString sipAddressToName(const QString &address)
{
    QRegExp rx(addressPattern);
    if (!rx.exactMatch(address)) {
        qWarning("Bad address %s", qPrintable(address));
        return QString();
    }
    QString name = rx.cap(1).trimmed();
    if (name.startsWith('"') && name.endsWith('"'))
        name = name.mid(1, name.size() - 2);
    return name.isEmpty() ? rx.cap(2).split('@').first() : name;
}

static QString sipAddressToUri(const QString &address)
{
    QRegExp rx(addressPattern);
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

SipCallPrivate::SipCallPrivate(SipCall *qq)
    : state(QXmppCall::OfferState),
    invitePending(false),
    activeTime("0 0"),
    q(qq)
{
}

void SipCallPrivate::handleReply(const SipMessage &reply)
{
    const QByteArray command = reply.headerField("CSeq").split(' ').last();

    // store information
    if (!reply.headerField("To").isEmpty())
        remoteRecipient = reply.headerField("To");
    if (!reply.headerField("Contact").isEmpty()) {
        const QString contact = QString::fromUtf8(reply.headerField("Contact"));
        remoteUri = sipAddressToUri(contact).toUtf8();
    }
    if (reply.hasHeaderField("Record-Route")) {
        remoteRoute.clear();
        foreach (const QByteArray &route, reply.headerFieldValues("Record-Route"))
            remoteRoute << route;
    }

    // send ack
    if  (command == "INVITE" && reply.statusCode() >= 200) {
        invitePending = false;

        SipMessage request = client->d->buildRequest("ACK", remoteUri, id, inviteRequest.sequenceNumber());
        for (int i = remoteRoute.size() - 1; i >= 0; --i)
            request.addHeaderField("Route", remoteRoute[i]);
        request.setHeaderField("To", remoteRecipient);
        request.setHeaderField("Via", inviteRequest.headerField("Via"));
        request.removeHeaderField("Contact");
        client->d->sendRequest(request, this);
    }

    // handle authentication
    if (reply.statusCode() == 407) {
        if (client->d->handleAuthentication(reply, this)) {
            if (lastRequest.method() == "INVITE") {
                invitePending = true;
                inviteRequest = lastRequest;
            }
        } else {
            setState(QXmppCall::FinishedState);
        }
        return;
    }

    if (command == "BYE") {

        if (invitePending) {
            SipMessage request = client->d->buildRequest("CANCEL", inviteRequest.uri(), id, inviteRequest.sequenceNumber());
            request.setHeaderField("To", inviteRequest.headerField("To"));
            request.setHeaderField("Via", inviteRequest.headerField("Via"));
            request.removeHeaderField("Contact");
            client->d->sendRequest(request, this);
        } else {
            setState(QXmppCall::FinishedState);
        }

    } else if (command == "CANCEL") {

        setState(QXmppCall::FinishedState);

    } else if (command == "INVITE") {

        if (reply.statusCode() == 180)
        {
            emit q->ringing();
        }
        else if (reply.statusCode() == 200)
        {
            q->debug(QString("SIP call %1 established").arg(QString::fromUtf8(id)));
            timer->stop();

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
            timer->stop();
            setState(QXmppCall::FinishedState);
        }
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
    if (request.hasHeaderField("Record-Route")) {
        remoteRoute.clear();
        foreach (const QByteArray &route, request.headerFieldValues("Record-Route"))
            remoteRoute << route;
    }

    // respond
    SipMessage response = client->d->buildResponse(request);
    if (request.method() == "ACK") {
        setState(QXmppCall::ActiveState);
        return;
    } else if (request.method() == "BYE") {
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
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
    } else {
        response.setStatusCode(405);
        response.setReasonPhrase("Method not allowed");
    }
    client->d->sendRequest(response, this);
}

SdpMessage SipCallPrivate::buildSdp(const QList<QXmppJinglePayloadType> &payloadTypes) const
{
    const QString localAddress = QString("IN %1 %2").arg(
        socket->localAddress().protocol() == QAbstractSocket::IPv6Protocol ? "IP6" : "IP4",
        socket->localAddress().toString());

    static const QDateTime ntpEpoch(QDate(1900, 1, 1));
    quint32 ntpSeconds = ntpEpoch.secsTo(QDateTime::currentDateTime());

    SdpMessage sdp;
    sdp.addField('v', "0");
    sdp.addField('o', QString("- %1 %2 %3").arg(
        QString::number(ntpSeconds),
        QString::number(ntpSeconds),
        localAddress).toUtf8());
    sdp.addField('s', "-");
    sdp.addField('c', localAddress.toUtf8());
    sdp.addField('t', activeTime);
#ifdef USE_ICE
    // ICE credentials
    sdp.addField('a', "ice-ufrag:" + iceConnection->localUser().toUtf8());
    sdp.addField('a', "ice-pwd:" + iceConnection->localPassword().toUtf8());
#endif

    // RTP profile
    QByteArray profiles = "RTP/AVP";
    QList<QByteArray> attrs;
    foreach (const QXmppJinglePayloadType &payload, payloadTypes) {
        profiles += " " + QByteArray::number(payload.id());
        QByteArray rtpmap = QByteArray::number(payload.id()) + " " + payload.name().toUtf8() + "/" + QByteArray::number(payload.clockrate());
        if (payload.channels() != 1)
            rtpmap += "/" + QByteArray::number(payload.channels());
        attrs << "rtpmap:" + rtpmap;
    }
    profiles += " 101";
    attrs << "rtpmap:101 telephone-event/8000";
    attrs << "fmtp:101 0-15";
    attrs << "sendrecv";

    sdp.addField('m', "audio " + QByteArray::number(socket->localPort()) + " "  + profiles);
    foreach (const QByteArray &attr, attrs)
        sdp.addField('a', attr);

#ifdef USE_ICE
    // ICE candidates
    foreach (const QXmppJingleCandidate &candidate, d->iceConnection->localCandidates()) {
        QByteArray ba = "candidate:" + QByteArray::number(candidate.foundation()) + " ";
        ba += QByteArray::number(candidate.component()) + " ";
        ba += "UDP ";
        ba += QByteArray::number(candidate.priority()) + " ";
        ba += candidate.host().toString().toUtf8() + " ";
        ba += QByteArray::number(candidate.port()) + " ";
        ba += "typ host";
        sdp.addField('a', ba);
    }
#endif
    return sdp;
}

bool SipCallPrivate::handleSdp(const SdpMessage &sdp)
{
    // parse descriptor
    QPair<char, QByteArray> field;
    foreach (field, sdp.fields()) {
        if (field.first == 'c') {
            // determine remote host
            if (field.second.startsWith("IN IP4 ") || field.second.startsWith("IN IP6 ")) {
                remoteHost = QHostAddress(QString::fromUtf8(field.second.mid(7)));
            }
        } else if (field.first == 'm') {
            QList<QByteArray> bits = field.second.split(' ');
            if (bits.size() < 3 || bits[0] != "audio" || bits[2] != "RTP/AVP")
                continue;

            // determine remote port
            remotePort = bits[1].toUInt();

            // determine codec
            foreach (const QXmppJinglePayloadType &payload, channel->supportedPayloadTypes()) {
                for (int i = 3; i < bits.size(); ++i) {
                    bool ok = false;
                    if (payload.id() == bits[i].toInt(&ok) && ok) {
                        q->debug(QString("Accepting RTP profile %1 (%2)").arg(
                            payload.name(),
                            QString::number(payload.id())));
                        commonPayloadTypes << payload;
                        break;
                    }
                }
            }
        } else if (field.first == 'a') {
            // determine packetization time
            if (field.second.startsWith("ptime:")) {
                bool ok = false;
                int ptime = field.second.mid(6).toInt(&ok);
                if (ok && ptime > 0) {
                    q->debug(QString("Setting RTP payload time to %1 ms").arg(QString::number(ptime)));
                    for (int i = 0; i < commonPayloadTypes.size(); ++i)
                        commonPayloadTypes[i].setPtime(ptime);
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

    // set codec
    if (!commonPayloadTypes.size()) {
        q->warning("Could not find a common codec");
        return false;
    }
    QXmppJinglePayloadType commonPayload = commonPayloadTypes[0];
    q->debug(QString("Selecting RTP profile %1 (%2), %3 ms").arg(
        commonPayload.name(),
        QString::number(commonPayload.id()),
        QString::number(commonPayload.ptime())));
    channel->setPayloadType(commonPayload);
    if (!channel->isOpen()) {
        q->warning("Could not assign codec to RTP channel");
        return false;
    }

    return true;
}

void SipCallPrivate::sendInvite()
{
    const SdpMessage sdp = buildSdp(channel->supportedPayloadTypes());

    SipMessage request = client->d->buildRequest("INVITE", remoteUri, id, cseq++);
    request.setHeaderField("To", remoteRecipient);
    request.setHeaderField("Content-Type", "application/sdp");
    request.setBody(sdp.toByteArray());
    client->d->sendRequest(request, this);
    invitePending = true;
    inviteRequest = request;

    timer->start(TIMEOUT_SECONDS * 1000);
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
        // prepare audio format
        QAudioFormat format;
        format.setFrequency(channel->payloadType().clockrate());
        format.setChannels(channel->payloadType().channels());
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        int packetSize = (format.frequency() * format.channels() * (format.sampleSize() / 8)) * channel->payloadType().ptime() / 1000;

        // initialise audio output
        if (!audioOutput) {
            audioOutput = new QAudioOutput(format, q);
            audioOutput->setBufferSize(2 * packetSize);
            audioOutput->start(channel);
        }

        // initialise audio input
        if (!audioInput) {
            audioInput = new QAudioInput(format, q);
            audioInput->setBufferSize(2 * packetSize);
            audioInput->start(channel);
        }
    } else {
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
    d = new SipCallPrivate(this);
    d->client = parent;
    d->direction = direction;
    d->remoteRecipient = recipient.toUtf8();
    d->remoteUri = sipAddressToUri(recipient).toUtf8();

    d->id = generateStanzaHash().toLatin1();
    d->channel = new QXmppRtpChannel(this);
    d->audioInput = 0;
    d->audioOutput = 0;
    d->remotePort = 0;
    d->socket = new QUdpSocket(this);
    if (!d->socket->bind(parent->d->socket->localAddress(), 0))
        warning("Could not start listening for RTP");
    d->timer = new QTimer(this);
    d->timer->setSingleShot(true);

    connect(d->channel, SIGNAL(sendDatagram(QByteArray)),
            this, SLOT(writeToSocket(QByteArray)));
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(readFromSocket()));
    connect(d->timer, SIGNAL(timeout()),
            this, SLOT(handleTimeout()));
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
        const SdpMessage sdp = d->buildSdp(d->commonPayloadTypes);

        SipMessage response = d->client->d->buildResponse(d->inviteRequest);
        response.setStatusCode(200);
        response.setReasonPhrase("OK");
        response.setHeaderField("Content-Type", "application/sdp");
        response.setBody(sdp.toByteArray());

        d->client->d->sendRequest(response, d);

        d->setState(QXmppCall::ConnectingState);
    }
}

/// Returns the call's direction.
///

QXmppCall::Direction SipCall::direction() const
{
    return d->direction;
}

QByteArray SipCall::id() const
{
    return d->id;
}

QString SipCall::recipient() const
{
    return QString::fromUtf8(d->remoteRecipient);
}

QXmppCall::State SipCall::state() const
{
    return d->state;
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
    d->socket->close();

    SipMessage request = d->client->d->buildRequest("BYE", d->remoteUri, d->id, d->cseq++);
    request.setHeaderField("To", d->remoteRecipient);
    for (int i = d->remoteRoute.size() - 1; i >= 0; --i)
        request.addHeaderField("Route", d->remoteRoute[i]);
    d->client->d->sendRequest(request, d);
}

void SipCall::readFromSocket()
{
    while (d->socket && d->socket->hasPendingDatagrams())
    {
        QByteArray ba(d->socket->pendingDatagramSize(), '\0');
        QHostAddress remoteHost;
        quint16 remotePort = 0;
        d->socket->readDatagram(ba.data(), ba.size(), &remoteHost, &remotePort);
        d->channel->datagramReceived(ba);
    }
}

/** Writes an RTP packet to the socket.
 *
 * @param ba
 */
void SipCall::writeToSocket(const QByteArray &ba)
{
    if (!d->socket || d->remoteHost.isNull() || !d->remotePort)
        return;

    d->socket->writeDatagram(ba, d->remoteHost, d->remotePort);
}

SipClientPrivate::SipClientPrivate(SipClient *qq)
    : state(SipClient::DisconnectedState),
    reflexivePort(0),
    socketsBound(false),
    stunDone(false),
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

SipMessage SipClientPrivate::buildRequest(const QByteArray &method, const QByteArray &uri, const QByteArray &callId, int seqNum)
{
    QString addr;
    if (!displayName.isEmpty())
        addr += QString("\"%1\"").arg(displayName);
    addr += QString("<sip:%1@%2>").arg(username, domain);

    const QString branch = "z9hG4bK-" + generateStanzaHash();
    const QString host = QString("%1:%2").arg(
        socket->localAddress().toString(),
        QString::number(socket->localPort()));
    const QString via = QString("SIP/2.0/UDP %1;branch=%2;rport").arg(
        host, branch);

    SipMessage packet;
    packet.setMethod(method);
    packet.setUri(uri);
    packet.setHeaderField("Via", via.toUtf8());
    packet.setHeaderField("Max-Forwards", "70");
    packet.setHeaderField("Call-ID", callId);
    packet.setHeaderField("CSeq", QByteArray::number(seqNum) + ' ' + method);
    packet.setHeaderField("Contact", QString("<sip:%1@%2:%3>").arg(
        username,
        reflexiveAddress.toString(),
        QString::number(reflexivePort)).toUtf8());
    packet.setHeaderField("To", addr.toUtf8());
    packet.setHeaderField("From", addr.toUtf8() + ";tag=" + tag);
    if (method != "ACK" && method != "CANCEL")
        packet.setHeaderField("Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
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
    response.setHeaderField("Contact", QString("<sip:%1@%2:%3>").arg(
        username,
        reflexiveAddress.toString(),
        QString::number(reflexivePort)).toUtf8());
    return response;
}

bool SipClientPrivate::handleAuthentication(const SipMessage &reply, SipCallContext *ctx)
{
    bool isProxy = reply.statusCode() == 407;
    QMap<QByteArray, QByteArray> *lastChallenge = isProxy ? &ctx->proxyChallenge : &ctx->challenge;

    const QByteArray auth = reply.headerField(isProxy ? "Proxy-Authenticate" : "WWW-Authenticate");
    int spacePos = auth.indexOf(' ');
    if (spacePos < 0 || auth.left(spacePos) != "Digest") {
        q->warning("Unsupported authentication method");
        return false;
    }
    QMap<QByteArray, QByteArray> challenge = QXmppSaslDigestMd5::parseMessage(auth.mid(spacePos + 1));
    if (lastChallenge->value("realm") == challenge.value("realm") &&
        lastChallenge->value("nonce") == challenge.value("nonce")) {
        q->warning("Authentication failed");
        return false;
    }
    *lastChallenge = challenge;

    SipMessage request = ctx->lastRequest;
    request.setHeaderField("CSeq", QByteArray::number(ctx->cseq++) + ' ' + request.method());
    sendRequest(request, ctx);
    return true;
}

void SipClientPrivate::handleReply(const SipMessage &reply)
{
    const QByteArray command = reply.headerField("CSeq").split(' ').last();

    // store information
    QMap<QByteArray, QByteArray> params = reply.headerFieldParameters("Via");
    if (params.contains("received"))
        reflexiveAddress = QHostAddress(QString::fromLatin1(params.value("received")));
    if (params.contains("rport"))
        reflexivePort = params.value("rport").toUInt();

    // handle authentication
    if (reply.statusCode() == 401) {
        if (!handleAuthentication(reply, this))
            setState(SipClient::DisconnectedState);
        return;
    }

    if (command == "REGISTER") {
        if (reply.statusCode() == 200) {
            if (state == SipClient::DisconnectingState) {
                setState(SipClient::DisconnectedState);
            } else {
                setState(SipClient::ConnectedState);

                // schedule next register
                QMap<QByteArray, QByteArray> params = reply.headerFieldParameters("Contact");
                int registerSeconds = params.value("expires").toInt();
                if (registerSeconds > MARGIN_SECONDS) {
                    q->debug(QString("Re-registering in %1 seconds").arg(registerSeconds - MARGIN_SECONDS));
                    registerTimer->start((registerSeconds - MARGIN_SECONDS) * 1000);
                }

                // send subscribe
                const QByteArray uri = QString("sip:%1@%2").arg(username, domain).toUtf8();
                SipMessage request = buildRequest("SUBSCRIBE", uri, id, cseq++);
                request.setHeaderField("Expires", QByteArray::number(EXPIRE_SECONDS));
                sendRequest(request, this);
            }
        } else if (reply.statusCode() >= 300) {
            q->warning("Register failed");
            setState(SipClient::DisconnectedState);
        }
    }
    else if (command == "SUBSCRIBE") {
        if (reply.statusCode() >= 300) {
            q->warning("Subscribe failed");
        }
    }
}

void SipClientPrivate::sendRequest(SipMessage &request, SipCallContext *ctx)
{
    if (request.isRequest()) {
        if (!ctx->challenge.isEmpty())
            request.setHeaderField("Authorization", authorization(request, ctx->challenge));
        if (!ctx->proxyChallenge.isEmpty())
            request.setHeaderField("Proxy-Authorization", authorization(request, ctx->proxyChallenge));
    }
    request.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());

#ifdef QXMPP_DEBUG_SIP
    q->logSent(QString("SIP packet to %1:%2\n%3").arg(
            serverAddress.toString(),
            QString::number(serverPort),
            QString::fromUtf8(request.toByteArray())));
#endif

    socket->writeDatagram(request.toByteArray(), serverAddress, serverPort);
    if (request.isRequest() && request.method() != "ACK")
        ctx->lastRequest = request;
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
    d = new SipClientPrivate(this);
    d->socket = new QUdpSocket(this);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(datagramReceived()));

    d->stunTester = new StunTester(this);

    d->registerTimer = new QTimer(this);
    connect(d->registerTimer, SIGNAL(timeout()),
            this, SLOT(connectToServer()));
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

    SipCall *call = new SipCall(QString("<%1>").arg(recipient), QXmppCall::OutgoingDirection, this);
    connect(call, SIGNAL(destroyed(QObject*)),
            this, SLOT(callDestroyed(QObject*)));
    d->calls << call;
    call->d->sendInvite();

    return call;
}

void SipClient::callDestroyed(QObject *object)
{
    d->calls.removeAll(static_cast<SipCall*>(object));
}

void SipClient::connectToServer()
{
    // bind sockets
    if (!d->socketsBound) {
        QHostAddress bindAddress;
        foreach (const QHostAddress &ip, QNetworkInterface::allAddresses()) {
            if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
                    ip != QHostAddress::Null &&
                    ip != QHostAddress::LocalHost &&
                    ip != QHostAddress::LocalHostIPv6 &&
                    ip != QHostAddress::Broadcast &&
                    ip != QHostAddress::Any &&
                    ip != QHostAddress::AnyIPv6) {
                bindAddress = ip;
                break;
            }
        }

        // listen for SIP
        if (!d->socket->bind(bindAddress, 0))
        {
            warning("Could not start listening for SIP");
            return;
        }
        debug(QString("Listening for SIP on %1:%2").arg(
            d->socket->localAddress().toString(),
            QString::number(d->socket->localPort())));

        d->reflexiveAddress = d->socket->localAddress();
        d->reflexivePort = d->socket->localPort();
        d->socketsBound = true;
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
    QByteArray messageId;
    quint16 messageType = QXmppStunMessage::peekType(buffer, messageId);
    if (messageType)
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

        // store reflexive address
        if (!message.xorMappedHost.isNull() && message.xorMappedPort != 0) {
            d->reflexiveAddress = message.xorMappedHost;
            d->reflexivePort = message.xorMappedPort;
        } else if (!message.mappedHost.isNull() && message.mappedPort != 0) {
            d->reflexiveAddress = message.mappedHost;
            d->reflexivePort = message.mappedPort;
        }
        d->stunDone = true;

        // register with server
        if (!d->serverAddress.isNull())
            registerWithServer();
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
            QString recipient = reply.headerField("From");
            info(QString("SIP call from %1").arg(recipient));
            currentCall = new SipCall(recipient, QXmppCall::IncomingDirection, this);
            currentCall->d->id = reply.headerField("Call-ID");
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
    d->registerTimer->stop();

    // terminate calls
    foreach (SipCall *call, d->calls)
        call->hangup();

    // unregister
    if (d->state == SipClient::ConnectedState) {
        debug(QString("Disconnecting from SIP server %1:%2").arg(d->serverAddress.toString(), QString::number(d->serverPort)));
        const QByteArray uri = QString("sip:%1").arg(d->domain).toUtf8();
        SipMessage request = d->buildRequest("REGISTER", uri, d->id, d->cseq++);
        request.setHeaderField("Contact", request.headerField("Contact") + ";expires=0");
        d->sendRequest(request, d);

        d->setState(DisconnectingState);
    }
}

void SipClient::registerWithServer()
{
    // register
    d->id = generateStanzaHash().toLatin1();
    d->tag = generateStanzaHash(8).toLatin1();
    debug(QString("Connecting to SIP server %1:%2").arg(d->serverAddress.toString(), QString::number(d->serverPort)));

    const QByteArray uri = QString("sip:%1").arg(d->domain).toUtf8();
    SipMessage request = d->buildRequest("REGISTER", uri, d->id, d->cseq++);
    request.setHeaderField("Expires", QByteArray::number(EXPIRE_SECONDS));
    d->sendRequest(request, d);
    d->registerTimer->start(TIMEOUT_SECONDS * 1000);

    d->setState(ConnectingState);
}

void SipClient::setSipServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        d->serverPort = serviceInfo.records().first().port();
    } else {
        serverName = d->domain;
        d->serverPort = 5060;
    }

    QHostInfo info = QHostInfo::fromName(serverName);
    if (info.addresses().isEmpty()) {
        warning(QString("Could not lookup SIP server %1").arg(serverName));
        return;
    }
    d->serverAddress = info.addresses().first();

    if (d->stunDone)
        registerWithServer();
}

void SipClient::setStunServer(const QXmppSrvInfo &serviceInfo)
{
    QString serverName = "stun." + d->domain;
    d->stunServerPort = 5060;
    if (!serviceInfo.records().isEmpty()) {
        serverName = serviceInfo.records().first().target();
        d->stunServerPort = serviceInfo.records().first().port();
    }

    QHostInfo info = QHostInfo::fromName(serverName);
    if (info.addresses().isEmpty()) {
        warning(QString("Could not lookup STUN server %1").arg(serverName));
        return;
    }
    d->stunServerAddress = info.addresses().first();

    // start STUN tests
    d->stunTester->setServer(d->stunServerAddress, d->stunServerPort);
    d->stunTester->start();
}

SipClient::State SipClient::state() const
{
    return d->state;
}

void SipClient::stunFinished()
{
    // send STUN binding request
    QXmppStunMessage request;
    request.setId(generateRandomBytes(12));
    request.setType(QXmppStunMessage::Binding | QXmppStunMessage::Request);

#ifdef QXMPP_DEBUG_STUN
    logSent(QString("STUN packet to %1 port %2\n%3").arg(d->stunServerAddress.toString(),
            QString::number(d->stunServerPort), request.toString()));
#endif
    d->socket->writeDatagram(request.encode(), d->stunServerAddress, d->stunServerPort);
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

bool SipMessage::hasHeaderField(const QByteArray &name) const
{
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it)
        if (qstricmp(name.constData(), it->first) == 0)
            return true;
    return false;
}

QByteArray SipMessage::headerField(const QByteArray &name, const QByteArray &defaultValue) const
{
    QList<QByteArray> allValues = headerFieldValues(name);
    if (allValues.isEmpty())
        return defaultValue;

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

QList<QByteArray> SipMessage::headerFieldValues(const QByteArray &name) const
{
    QList<QByteArray> result;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it)
        if (qstricmp(name.constData(), it->first) == 0)
            result += it->second;

    return result;
}

QMap<QByteArray, QByteArray> SipMessage::headerFieldParameters(const QByteArray &name) const
{
    const QByteArray value = headerField(name);
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

StunTester::StunTester(QObject *parent)
    : QXmppLoggable(parent),
    retries(0),
    serverPort(0),
    step(StunConnectivity)
{
    socket = new QUdpSocket(this);
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(readyRead()));

    timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, SIGNAL(timeout()),
            this, SLOT(timeout()));
}

void StunTester::readyRead()
{
    if (!socket->hasPendingDatagrams())
        return;

    // receive datagram
    const qint64 size = socket->pendingDatagramSize();
    QByteArray buffer(size, 0);
    QHostAddress remoteHost;
    quint16 remotePort;
    socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    // decode STUN
    QXmppStunMessage response;
    if (!response.decode(buffer))
        return;

#ifdef QXMPP_DEBUG_STUN
    logReceived(QString("STUN packet from %1 port %2\n%3").arg(
        remoteHost.toString(),
        QString::number(remotePort),
        response.toString()));
#endif

    // determine next step
    timer->stop();
    switch (step)
    {
    case StunConnectivity:
        if (response.changedHost.isNull() || !response.changedPort) {
            warning("STUN response is missing changed address or port");
            emit finished();
            return;
        }
        changedAddress = response.changedHost;
        changedPort = response.changedPort;
        retries = 0;
        step = StunChangeServer;
        sendRequest();
        break;
    case StunChangeServer:
        retries = 0;
        step = StunChangePort;
        sendRequest();
        break;
    }
}

void StunTester::sendRequest()
{
    // send STUN binding request
    QXmppStunMessage request;
    request.setId(generateRandomBytes(12));
    request.setType(QXmppStunMessage::Binding | QXmppStunMessage::Request);

    QHostAddress destAddress = serverAddress;
    quint16 destPort = serverPort;

    switch (step)
    {
    case StunConnectivity:
        break;
    case StunChangeServer:
        destAddress = changedAddress;
        destPort = changedPort;
        break;
    case StunChangePort:
        request.setChangeRequest(1);
        break;
    default:
        warning("Unknown STUN step");
        return;
    }

#ifdef QXMPP_DEBUG_STUN
    logSent(QString("STUN packet to %1 port %2\n%3").arg(destAddress.toString(),
            QString::number(destPort), request.toString()));
#endif
    socket->writeDatagram(request.encode(), destAddress, destPort);
    timer->start();
}

void StunTester::setServer(const QHostAddress &server, quint16 port)
{
    serverAddress = server;
    serverPort = port;
}

void StunTester::start()
{
    if (serverAddress.isNull() || !serverPort) {
        warning("STUN tester is missing server address or port");
        emit finished();
        return;
    }
    if (!socket->bind()) {
        warning("Could not start listening for STUN");
        emit finished();
        return;
    }
    retries = 0;
    step = StunConnectivity;
    sendRequest();
}

void StunTester::timeout()
{
    timer->stop();
    if (retries > 4) {
        warning("STUN timeout");
        emit finished();
        return;
    }

    retries++;
    sendRequest();
}
