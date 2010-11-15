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
#include <QDebug>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QPair>
#include <QUdpSocket>

#include "QXmppRtpChannel.h"
#include "QXmppSaslAuth.h"
#include "QXmppSrvInfo.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"

#include "sip.h"

const int RTP_COMPONENT = 1;
const int RTCP_COMPONENT = 2;

#define QXMPP_DEBUG_SIP

QXmppLoggable::QXmppLoggable(QObject *parent)
    : QObject(parent)
{
}

void QXmppLoggable::debug(const QString &msg)
{
    emit logMessage(QXmppLogger::DebugMessage, msg);
}

void QXmppLoggable::warning(const QString &msg)
{
    emit logMessage(QXmppLogger::WarningMessage, msg);
}

struct AuthInfo
{
    QMap<QByteArray, QByteArray> challenge;
    bool proxy;
};

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

class SipCallPrivate
{
public:
    QString recipient;
    QByteArray id;
    QXmppRtpChannel *channel;
    QAudioInput *audioInput;
    QAudioOutput *audioOutput;
    QHostAddress remoteHost;
    quint16 remotePort;
    QUdpSocket *socket;

    SipCall *q;
    SipClient *client;
};

class SipClientPrivate
{
public:
    SipRequest buildRequest(const QByteArray &method, const QByteArray &uri, const QByteArray &id);
    void sendRequest(SipRequest &request);

    QUdpSocket *socket;
    QByteArray baseId;
    int cseq;
    QByteArray tag;

    // configuration
    QString displayName;
    QString username;
    QString password;
    QString domain;

    bool disconnecting;
    QString rinstance;
    QMap<QByteArray, AuthInfo> authInfos;
    QHostAddress serverAddress;
    QString serverName;
    quint16 serverPort;
    SipRequest lastRequest;
    QList<SipCall*> calls;

    SipClient *q;
};

SipCall::SipCall(const QString &recipient, QUdpSocket *socket, SipClient *parent)
    : QXmppLoggable(parent),
    d(new SipCallPrivate)
{
    d->client = parent;
    d->q = this;
    d->recipient = recipient;
    d->id = generateStanzaHash().toLatin1();
    d->channel = new QXmppRtpChannel(this);
    d->audioInput = 0;
    d->audioOutput = 0;
    d->remotePort = 0;
    d->socket = socket;
    d->socket->setParent(this);

    connect(d->channel, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));
    connect(d->channel, SIGNAL(sendDatagram(QByteArray)),
            this, SLOT(writeToSocket(QByteArray)));
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(readFromSocket()));
}

SipCall::~SipCall()
{
    delete d;
}

QByteArray SipCall::id() const
{
    return d->id;
}

void SipCall::handleReply(const SipReply &reply)
{
    if (reply.statusCode() == 180)
    {
        emit ringing();
    }
    else if (reply.statusCode() < 200)
    {
        debug(QString("Call %1 progress %2 %3").arg(
            QString::fromUtf8(d->id),
            QString::number(reply.statusCode()), reply.reasonPhrase()));
    }
    else if (reply.statusCode() == 200)
    {
        debug(QString("Call %1 established").arg(QString::fromUtf8(d->id)));

        if (reply.headerField("Content-Type") == "application/sdp" && !d->audioOutput)
        {
            QXmppRtpChannel *channel = d->channel;

            // parse descriptor
            SdpMessage sdp(reply.body());
            QPair<char, QByteArray> field;
            foreach (field, sdp.fields()) {
                //qDebug() << "field" << field.first << field.second;
                if (field.first == 'c') {
                    // determine remote host
                    if (field.second.startsWith("IN IP4 ")) {
                        d->remoteHost = QHostAddress(QString::fromUtf8(field.second.mid(7)));
                    }
                } else if (field.first == 'm') {
                    QList<QByteArray> bits = field.second.split(' ');
                    if (bits.size() < 3 || bits[0] != "audio" || bits[2] != "RTP/AVP")
                        continue;

                    // determine remote port
                    d->remotePort = bits[1].toUInt();

                    // determine codec
                    for (int i = 3; i < bits.size() && !d->channel->isOpen(); ++i)
                    {
                        foreach (const QXmppJinglePayloadType &payload, d->channel->supportedPayloadTypes()) {
                            bool ok = false;
                            if (payload.id() == bits[i].toInt(&ok) && ok) {
                                qDebug() << "found codec" << payload.name();
                                d->channel->setPayloadType(payload);
                                break;
                            }
                        }
                    }
                }
            }
            if (!d->channel->isOpen()) {
                warning("Could not negociate a common codec");
                return;
            }

            // prepare audio format
            QAudioFormat format;
            format.setFrequency(d->channel->payloadType().clockrate());
            format.setChannels(d->channel->payloadType().channels());
            format.setSampleSize(16);
            format.setCodec("audio/pcm");
            format.setByteOrder(QAudioFormat::LittleEndian);
            format.setSampleType(QAudioFormat::SignedInt);

            int packetSize = (format.frequency() * format.channels() * (format.sampleSize() / 8)) * d->channel->payloadType().ptime() / 1000;

            // initialise audio output
            d->audioOutput = new QAudioOutput(format, this);
            d->audioOutput->setBufferSize(2 * packetSize);
            d->audioOutput->start(d->channel);

            // initialise audio input
            d->audioInput = new QAudioInput(format, this);
            d->audioInput->setBufferSize(2 * packetSize);
            d->audioInput->start(d->channel);

            emit connected();
        }
    } else if (reply.statusCode() >= 300) {
        warning(QString("Call %1 failed").arg(
            QString::fromUtf8(d->id)));
        emit finished();
    }
}

void SipCall::hangup()
{
    debug(QString("Call %1 hangup").arg(
            QString::fromUtf8(d->id)));

    // stop audio
    if (d->audioInput)
    {
        d->audioInput->stop();
        delete d->audioInput;
        d->audioInput = 0;
    }
    if (d->audioOutput)
    {
        d->audioOutput->stop();
        delete d->audioOutput;
        d->audioOutput = 0;
    }
    d->socket->close();

    SipRequest request = d->client->d->buildRequest("BYE", d->recipient.toUtf8(), d->id);
    request.setHeaderField("To", "<" + d->recipient.toUtf8() + ">");
    d->client->d->sendRequest(request);
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

void SipCall::writeToSocket(const QByteArray &ba)
{
    if (!d->socket || d->remoteHost.isNull() || !d->remotePort)
        return;

    d->socket->writeDatagram(ba, d->remoteHost, d->remotePort);
}

SipRequest SipClientPrivate::buildRequest(const QByteArray &method, const QByteArray &uri, const QByteArray &callId)
{
    const QString addr = QString("\"%1\"<sip:%2@%3>").arg(displayName,
        username, domain);
    const QByteArray branch = "z9hG4bK-" + generateStanzaHash().toLatin1();
    const QString via = QString("SIP/2.0/UDP %1:%2;branch=%3;rport").arg(
        socket->localAddress().toString(),
        QString::number(socket->localPort()),
        branch);

    SipRequest packet;
    packet.setMethod(method);
    packet.setUri(uri);
    packet.setHeaderField("Via", via.toLatin1());
    packet.setHeaderField("Max-Forwards", "70");
    packet.setHeaderField("Call-ID", callId);
    packet.setHeaderField("CSeq", QString::number(cseq++).toLatin1() + ' ' + method);
    packet.setHeaderField("Contact", QString("<sip:%1@%2:%3;rinstance=%4>").arg(
        username, socket->localAddress().toString(),
        QString::number(socket->localPort()), rinstance).toUtf8());
    packet.setHeaderField("To", addr.toUtf8());
    packet.setHeaderField("From", addr.toUtf8() + ";tag=" + tag);
    packet.setHeaderField("Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
    packet.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());
    return packet;
}

void SipClientPrivate::sendRequest(SipRequest &request)
{
    if (authInfos.contains(request.uri()))
    {
        AuthInfo info = authInfos.value(request.uri());

        QXmppSaslDigestMd5 digest;
        digest.setCnonce(digest.generateNonce());
        digest.setNc("00000001");
        digest.setNonce(info.challenge.value("nonce"));
        digest.setQop(info.challenge.value("qop"));
        digest.setRealm(info.challenge.value("realm"));
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
        response["cnonce"] = digest.cnonce();
        response["qop"] = digest.qop();
        response["nc"] = digest.nc();
        response["algorithm"] = "MD5";

        request.setHeaderField(info.proxy ? "Proxy-Authorization" : "Authorization",
                               "Digest " + QXmppSaslDigestMd5::serializeMessage(response));
    }

    if (request.method() != "ACK")
        lastRequest = request;

#ifdef QXMPP_DEBUG_SIP
    q->logMessage(QXmppLogger::SentMessage,
        QString("SIP packet to %1:%2\n%3").arg(
            serverAddress.toString(),
            QString::number(serverPort),
            QString::fromUtf8(request.toByteArray())));
#endif

    socket->writeDatagram(request.toByteArray(), serverAddress, serverPort);
}

SipClient::SipClient(QObject *parent)
    : QXmppLoggable(parent),
    d(new SipClientPrivate)
{
    d->q = this;
    d->cseq = 1;
    d->disconnecting = false;
    d->rinstance = generateStanzaHash(16);
    d->socket = new QUdpSocket(this);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(datagramReceived()));
}

SipClient::~SipClient()
{
    delete d;
}

SipCall *SipClient::call(const QString &recipient)
{
    QUdpSocket *socket = new QUdpSocket;
    if (!socket->bind(d->socket->localAddress(), 0))
    {
        warning("Could not start listening for RTP");
        delete socket;
        return 0;
    }

    SipCall *call = new SipCall(recipient, socket, this);
    connect(call, SIGNAL(destroyed(QObject*)),
            this, SLOT(callDestroyed(QObject*)));
    connect(call, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            this, SIGNAL(logMessage(QXmppLogger::MessageType,QString)));

    SdpMessage sdp;
    sdp.addField('v', "0");
    sdp.addField('o', "- 1289387706660194 1 IN IP4 " + d->socket->localAddress().toString().toUtf8());
    sdp.addField('s', qApp->applicationName().toUtf8());
    sdp.addField('c', "IN IP4 " + socket->localAddress().toString().toUtf8());
    sdp.addField('t', "0 0");
#ifdef USE_ICE
    sdp.addField('a', "ice-ufrag:" + call->iceConnection->localUser().toUtf8());
    sdp.addField('a', "ice-pwd:" + call->iceConnection->localPassword().toUtf8());
#endif
    QByteArray profiles = "RTP/AVP";
    foreach (const QXmppJinglePayloadType &payload, call->d->channel->supportedPayloadTypes())
        profiles += " " + QByteArray::number(payload.id());
    profiles += " 101";
    sdp.addField('m', "audio " + QByteArray::number(socket->localPort()) + " "  + profiles);
    sdp.addField('a', "rtpmap:101 telephone-event/8000");
    sdp.addField('a', "fmtp:101 0-15");
    sdp.addField('a', "sendrecv");
#ifdef USE_ICE
    foreach (const QXmppJingleCandidate &candidate, call->iceConnection->localCandidates()) {
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
    d->calls << call;

    SipRequest request = d->buildRequest("INVITE", recipient.toUtf8(), call->id());
    request.setHeaderField("To", "<" + recipient.toUtf8() + ">");
    request.setHeaderField("Content-Type", "application/sdp");
    request.setBody(sdp.toByteArray());
    d->sendRequest(request);

    return call;
}

void SipClient::callDestroyed(QObject *object)
{
    d->calls.removeAll(static_cast<SipCall*>(object));
}

void SipClient::connectToServer()
{
    debug(QString("Looking up server for domain %1").arg(d->domain));
    QXmppSrvInfo::lookupService("_sip._udp." + d->domain, this,
                                SLOT(connectToServer(QXmppSrvInfo)));
}

void SipClient::connectToServer(const QXmppSrvInfo &serviceInfo)
{
    if (!serviceInfo.records().isEmpty()) {
        d->serverName = serviceInfo.records().first().target();
        d->serverPort = serviceInfo.records().first().port();
    } else {
        d->serverName = d->domain;
        d->serverPort = 5060;
    }

    debug(QString("Connecting to SIP server %1:%2").arg(d->serverName, QString::number(d->serverPort)));
    QHostInfo info = QHostInfo::fromName(d->serverName);
    if (info.addresses().isEmpty()) {
        warning(QString("Could not lookup SIP server %1").arg(d->serverName));
        return;
    }
    d->serverAddress = info.addresses().first();
    d->baseId = generateStanzaHash().toLatin1();
    d->tag = generateStanzaHash(8).toLatin1();

    QHostAddress bindAddress;
    foreach (const QHostAddress &ip, QNetworkInterface::allAddresses())
    {
        if (ip.protocol() == QAbstractSocket::IPv4Protocol &&
            ip != QHostAddress::Null &&
            ip != QHostAddress::LocalHost &&
            ip != QHostAddress::LocalHostIPv6 &&
            ip != QHostAddress::Broadcast &&
            ip != QHostAddress::Any &&
            ip != QHostAddress::AnyIPv6)
        {
            bindAddress = ip;
            break;
        }
    }
    if (!d->socket->bind(bindAddress, 0))
    {
        warning("Could not start listening for SIP");
        return;
    }

    // register
    const QByteArray uri = QString("sip:%1").arg(d->serverName).toUtf8();
    SipRequest request = d->buildRequest("REGISTER", uri, d->baseId);
    request.setHeaderField("Expires", "3600");
    d->sendRequest(request);
}

void SipClient::disconnectFromServer()
{
    d->disconnecting = true;

    // unregister
    const QByteArray uri = QString("sip:%1").arg(d->serverName).toUtf8();
    SipRequest request = d->buildRequest("REGISTER", uri, d->baseId);
    request.setHeaderField("Contact", request.headerField("Contact") + ";expires=0");
    d->sendRequest(request);
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

#ifdef QXMPP_DEBUG_SIP
    emit logMessage(QXmppLogger::ReceivedMessage,
        QString("SIP packet from %1\n%2").arg(remoteHost.toString(), QString::fromUtf8(buffer)));
#endif

    // parse packet
    SipReply reply(buffer);
    QByteArray cseq = reply.headerField("CSeq");
    int i = cseq.indexOf(' ');
    if (i < 0) {
        warning("No CSeq found in reply");
        return;
    }
    int commandSeq = cseq.left(i).toInt();
    QByteArray command = cseq.mid(i+1);

    // find corresponding call
    SipCall *currentCall = 0;
    const QByteArray callId = reply.headerField("Call-ID");
    if (callId != d->baseId)
    {
        foreach (SipCall *potentialCall, d->calls)
        {
            if (potentialCall->id() == callId)
            {
                currentCall = potentialCall;
                break;
            }
        }
        if (!currentCall)
        {
            warning("Received a reply for unknown call " + callId);
            return;
        }
    }

    // send ack
    if (command == "INVITE" && reply.statusCode() >= 200) {
        SipRequest request;
        request.setMethod("ACK");
        request.setUri(reply.headerField("contact").replace("<", "").replace(">", ""));
        request.setHeaderField("Via", d->lastRequest.headerField("Via"));
        request.setHeaderField("Max-Forwards", d->lastRequest.headerField("Max-Forwards"));
        request.setHeaderField("Contact", d->lastRequest.headerField("Contact"));
        request.setHeaderField("To", reply.headerField("To"));
        request.setHeaderField("From", reply.headerField("From"));
        request.setHeaderField("Call-Id", reply.headerField("Call-Id"));
        request.setHeaderField("CSeq", QByteArray::number(commandSeq) + " ACK");
        if (!reply.headerField("Record-Route").isEmpty())
            request.setHeaderField("Route", reply.headerField("Record-Route"));
        d->sendRequest(request);
    }

    // handle authentication
    if (reply.statusCode() == 401 || reply.statusCode() == 407) {

        AuthInfo info;
        info.proxy = reply.statusCode() == 407;
        const QByteArray auth = reply.headerField(info.proxy ? "Proxy-Authenticate" : "WWW-Authenticate");
        if (!auth.startsWith("Digest ")) {
            warning("Unsupported authentication method");
            d->socket->close();
            emit disconnected();
        }

        SipRequest request = d->lastRequest;
        if (!request.headerField(info.proxy ? "Proxy-Authorization" : "Authorization").isEmpty()) {
            warning("Authentication failed");
            d->socket->close();
            emit disconnected();
        }
        request.setHeaderField("CSeq", QString::number(d->cseq++).toLatin1() + ' ' + request.method());
        info.challenge = QXmppSaslDigestMd5::parseMessage(auth.mid(7));
        d->authInfos[request.uri()] = info;

        d->sendRequest(request);
        return;
    }

    if (currentCall)
    {
        // actual VoIP call
        currentCall->handleReply(reply);
    } else {
        // "base" call
        if (command == "REGISTER" && reply.statusCode() == 200) {
            if (d->disconnecting) {
                d->socket->close();
                emit disconnected();
            } else {
                const QByteArray uri = QString("sip:%1@%2").arg(d->username, d->domain).toUtf8();
                SipRequest request = d->buildRequest("SUBSCRIBE", uri, d->baseId);
                request.setHeaderField("Expires", "3600");
                d->sendRequest(request);
            }
        }
        else if (command == "SUBSCRIBE" && reply.statusCode() == 200) {
            emit connected();
        }
    }

}

QString SipClient::serverName() const
{
    return d->serverName;
}

void SipClient::setDisplayName(const QString &displayName)
{
    d->displayName = displayName;
}

void SipClient::setDomain(const QString &domain)
{
    d->domain = domain;
}

void SipClient::setPassword(const QString &password)
{
    d->password = password;
}

void SipClient::setUsername(const QString &username)
{
    d->username = username;
}

SipRequest::SipRequest()
{
    setHeaderField("Content-Length", "0");
}

QByteArray SipRequest::body() const
{
    return m_body;
}

void SipRequest::setBody(const QByteArray &body)
{
    m_body = body;
    setHeaderField("Content-Length", QByteArray::number(m_body.size()));
}

QByteArray SipRequest::method() const
{
    return m_method;
}

void SipRequest::setMethod(const QByteArray &method)
{
    m_method = method;
}

QByteArray SipRequest::uri() const
{
    return m_uri;
}

void SipRequest::setUri(const QByteArray &uri)
{
    m_uri = uri;
}

QByteArray SipRequest::toByteArray() const
{
    QByteArray ba;
    ba += m_method;
    ba += ' ';
    ba += m_uri;
    ba += " SIP/2.0\r\n";

    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it) {
        ba += it->first;
        ba += ": ";
        ba += it->second;
        ba += "\r\n";
    }
    ba += "\r\n";
    return ba + m_body;
}

SipReply::SipReply(const QByteArray &bytes)
    : m_statusCode(0)
{
    const QByteArrayMatcher crlf("\r\n");
    const QByteArrayMatcher colon(":");

    int j = crlf.indexIn(bytes);
    if (j < 0)
        return;

    // parse status
    const QByteArray status = bytes.left(j);
    if (status.size() < 10 ||
        !status.startsWith("SIP/2.0"))
        return;

    m_statusCode = status.mid(8, 3).toInt();
    m_reasonPhrase = QString::fromUtf8(status.mid(12));

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

QByteArray SipReply::body() const
{
    return m_body;
}

QString SipReply::reasonPhrase() const
{
    return m_reasonPhrase;
}

int SipReply::statusCode() const
{
    return m_statusCode;
}

QByteArray SipPacket::headerField(const QByteArray &name, const QByteArray &defaultValue) const
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

QList<QByteArray> SipPacket::headerFieldValues(const QByteArray &name) const
{
    QList<QByteArray> result;
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    for ( ; it != end; ++it)
        if (qstricmp(name.constData(), it->first) == 0)
            result += it->second;

    return result;
}

void SipPacket::setHeaderField(const QByteArray &name, const QByteArray &data)
{
    QList<QPair<QByteArray, QByteArray> >::Iterator it = m_fields.begin();
    while (it != m_fields.end()) {
        if (qstricmp(name.constData(), it->first) == 0)
            it = m_fields.erase(it);
        else
            ++it;
    }
    m_fields.append(qMakePair(name, data));
}

