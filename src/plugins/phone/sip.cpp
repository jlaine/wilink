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
    QByteArray id;
    QXmppRtpChannel *channel;
    QAudioInput *audioInput;
    QAudioOutput *audioOutput;
    QHostAddress remoteHost;
    quint16 remotePort;
    QUdpSocket *socket;
};

SipCall::SipCall(QUdpSocket *socket, QObject *parent)
    : QObject(parent),
    d(new SipCallPrivate)
{
    d->id = generateStanzaHash().toLatin1();
    d->channel = new QXmppRtpChannel(this);
    d->audioInput = 0;
    d->audioOutput = 0;
    d->remotePort = 0;
    d->socket = socket;
    d->socket->setParent(this);

    connect(d->channel, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            QXmppLogger::getLogger(), SLOT(log(QXmppLogger::MessageType,QString)));
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
};

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

    lastRequest = request;
    socket->writeDatagram(request.toByteArray(), serverAddress, serverPort);
}

SipClient::SipClient(QObject *parent)
    : QObject(parent),
    d(new SipClientPrivate)
{
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

void SipClient::call(const QString &recipient)
{
    QUdpSocket *socket = new QUdpSocket;
    if (!socket->bind(d->socket->localAddress(), 0))
    {
        qWarning("Could not start listening for RTP");
        delete socket;
        return;
    }

    SipCall *call = new SipCall(socket, this);

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
}

void SipClient::connectToServer()
{
    qDebug() << "Looking up server for domain" << d->domain;
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

    qDebug("Connecting to %s:%i", qPrintable(d->serverName), d->serverPort);
    QHostInfo info = QHostInfo::fromName(d->serverName);
    if (info.addresses().isEmpty()) {
        qWarning("Could not lookup %s", qPrintable(d->serverName));
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
        qWarning("Could not start listening for SIP");
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

    // parse packet
    SipReply reply(buffer);
    QByteArray cseq = reply.headerField("CSeq");
    int i = cseq.indexOf(' ');
    if (i < 0) {
        qWarning("No CSeq found in reply");
        return;
    }
    int commandSeq = cseq.left(i).toInt();
    QByteArray command = cseq.mid(i+1);
    qDebug() << "Received reply for command" << command << commandSeq;

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
            qWarning("Received a reply for unknown call " + callId);
            return;
        }
    }

    // send ack
    if (command == "INVITE" && reply.statusCode() >= 200) {
        SipRequest request;
        request.setMethod("ACK");
        request.setUri(d->lastRequest.uri());
        request.setHeaderField("Via", d->lastRequest.headerField("Via"));
        request.setHeaderField("Max-Forwards", d->lastRequest.headerField("Max-Forwards"));
        request.setHeaderField("To", reply.headerField("To"));
        request.setHeaderField("From", reply.headerField("From"));
        request.setHeaderField("Call-Id", reply.headerField("Call-Id"));
        request.setHeaderField("CSeq", QByteArray::number(commandSeq) + " ACK");
        if (!reply.headerField("Record-Route").isEmpty())
            request.setHeaderField("Route", reply.headerField("Record-Route"));
        if (!d->lastRequest.headerField("Proxy-Authorization").isEmpty())
            request.setHeaderField("Proxy-Authorization", d->lastRequest.headerField("Proxy-Authorization"));
        if (!d->lastRequest.headerField("Authorization").isEmpty())
            request.setHeaderField("Authorization", d->lastRequest.headerField("Authorization"));
        d->socket->writeDatagram(request.toByteArray(), d->serverAddress, d->serverPort);
    }

    // handle authentication
    if (reply.statusCode() == 401 || reply.statusCode() == 407) {

        AuthInfo info;
        info.proxy = reply.statusCode() == 407;
        const QByteArray auth = reply.headerField(info.proxy ? "Proxy-Authenticate" : "WWW-Authenticate");
        if (!auth.startsWith("Digest ")) {
            qWarning("Unsupported authentication method");
            d->socket->close();
            emit disconnected();
        }

        SipRequest request = d->lastRequest;
        if (!request.headerField(info.proxy ? "Proxy-Authorization" : "Authorization").isEmpty()) {
            qWarning("Authentication failed");
            d->socket->close();
            emit disconnected();
        }
        request.setHeaderField("CSeq", QString::number(d->cseq++).toLatin1() + ' ' + request.method());
        info.challenge = QXmppSaslDigestMd5::parseMessage(auth.mid(7));
        d->authInfos[request.uri()] = info;

        d->sendRequest(request);
        return;
    }

    // actual VoIP call
    if (currentCall)
    {
        if (reply.statusCode() < 200)
        {
            qDebug() << "Call" << currentCall->id() << "progress" << reply.statusCode() << reply.reasonPhrase();
        }
        else if (reply.statusCode() == 200)
        {
            qDebug() << "Call" << currentCall->id() << "established";

            if (reply.headerField("Content-Type") == "application/sdp" && !currentCall->d->audioOutput)
            {
                QXmppRtpChannel *channel = currentCall->d->channel;

                // parse descriptor
                SdpMessage sdp(reply.body());
                QPair<char, QByteArray> field;
                foreach (field, sdp.fields()) {
                    //qDebug() << "field" << field.first << field.second;
                    if (field.first == 'c') {
                        // determine remote host
                        if (field.second.startsWith("IN IP4 ")) {
                            currentCall->d->remoteHost = QHostAddress(QString::fromUtf8(field.second.mid(7)));
                            qDebug() << "found host" << currentCall->d->remoteHost;
                        }
                    } else if (field.first == 'm') {
                        QList<QByteArray> bits = field.second.split(' ');
                        if (bits.size() < 3 || bits[0] != "audio" || bits[2] != "RTP/AVP")
                            continue;

                        // determine remote port
                        currentCall->d->remotePort = bits[1].toUInt();
                        qDebug() << "found port" << currentCall->d->remotePort;

                        // determine codec
                        for (int i = 3; i < bits.size() && !channel->isOpen(); ++i)
                        {
                            foreach (const QXmppJinglePayloadType &payload, channel->supportedPayloadTypes()) {
                                bool ok = false;
                                if (payload.id() == bits[i].toInt(&ok) && ok) {
                                    qDebug() << "found codec" << payload.name();
                                    channel->setPayloadType(payload);
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!channel->isOpen()) {
                    qWarning("Could not negociate a common codec");
                    return;
                }

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
                currentCall->d->audioOutput = new QAudioOutput(format, this);
                currentCall->d->audioOutput->setBufferSize(2 * packetSize);
                currentCall->d->audioOutput->start(channel);

                // initialise audio input
                currentCall->d->audioInput = new QAudioInput(format, this);
                currentCall->d->audioInput->setBufferSize(2 * packetSize);
                currentCall->d->audioInput->start(channel);
            }

        } else if (reply.statusCode() >= 400) {
            qDebug() << "Call" << currentCall->id() << "failed";
            d->calls.removeAll(currentCall);
            delete currentCall;
        }

    // "base" call
    } else {
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

