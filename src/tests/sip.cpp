#include <signal.h>

#include <QAudioInput>
#include <QAudioOutput>
#include <QByteArrayMatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QSettings>
#include <QUdpSocket>

#include "QXmppSaslAuth.h"
#include "QXmppStun.h"
#include "QXmppUtils.h"
#include "rtp.h"
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

class SipCall
{
public:
    SipCall();

    QByteArray id;
    RtpChannel *channel;
    QAudioInput *audioInput;
    QAudioOutput *audioOutput;
};

SipCall::SipCall()
    : channel(0),
    audioInput(0),
    audioOutput(0)
{
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
    QString phoneNumber;

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
    d->rinstance = generateStanzaHash(16);
    d->socket = new QUdpSocket(this);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(datagramReceived()));

    QSettings settings("sip.conf", QSettings::IniFormat);
    d->domain = settings.value("domain").toString();
    d->displayName = settings.value("displayName").toString();
    d->username = settings.value("username").toString();
    d->password = settings.value("password").toString();
    d->phoneNumber = settings.value("phoneNumber").toString();
}

SipClient::~SipClient()
{
    delete d;
}

void SipClient::call(const QString &recipient)
{
    QUdpSocket *socket = new QUdpSocket(this);
    if (!socket->bind(d->socket->localAddress(), 0))
    {
        qWarning("Could not start listening for RTP");
        delete socket;
        return;
    }

    SipCall *call = new SipCall;
    call->id = generateStanzaHash().toLatin1();
    call->channel = new RtpChannel(this);
    connect(call->channel, SIGNAL(logMessage(QXmppLogger::MessageType,QString)),
            QXmppLogger::getLogger(), SLOT(log(QXmppLogger::MessageType,QString)));
    call->channel->setSocket(socket);
    socket->setParent(call->channel);

    // FIXME : negociate codec!
    QXmppJinglePayloadType payload;
    payload.setId(RtpChannel::G711u);
    payload.setChannels(1);
    payload.setName("PCMU");
    payload.setClockrate(8000);
    call->channel->setPayloadType(payload);

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
    profiles += " " + QByteArray::number(payload.id());
    // profiles += " " + QByteArray::number(RtpChannel::G711a);
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

    SipRequest request = d->buildRequest("INVITE", recipient.toUtf8(), call->id);
    request.setHeaderField("To", "<" + recipient.toUtf8() + ">");
    request.setHeaderField("Content-Type", "application/sdp");
    request.setBody(sdp.toByteArray());
    d->sendRequest(request);
}

void SipClient::connectToServer(const QString &server, quint16 port)
{
    QHostInfo info = QHostInfo::fromName(server);
    if (info.addresses().isEmpty())
        return;
    d->baseId = generateStanzaHash().toLatin1();
    d->tag = generateStanzaHash(8).toLatin1();
    d->serverAddress = info.addresses().first();
    d->serverName = server;
    d->serverPort = port;

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
    // register
    const QByteArray uri = QString("sip:%1").arg(d->serverName).toUtf8();
    SipRequest request = d->buildRequest("REGISTER", uri, d->baseId);
    request.setHeaderField("Contact", request.headerField("Contact") + ";expires=0");
    d->sendRequest(request);
    d->socket->flush();
    d->socket->close();
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
            if (potentialCall->id == reply.headerField("Call-ID"))
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
        SipRequest request = d->lastRequest;
        request.setHeaderField("CSeq", QString::number(d->cseq++).toLatin1() + ' ' + request.method());

        AuthInfo info;
        info.proxy = reply.statusCode() == 407;
        const QByteArray auth = reply.headerField(info.proxy ? "Proxy-Authenticate" : "WWW-Authenticate");
        if (!auth.startsWith("Digest ")) {
            qWarning("Unsupported authentication method");
            return;
        }
        info.challenge = QXmppSaslDigestMd5::parseMessage(auth.mid(7));
        d->authInfos[request.uri()] = info;

        d->sendRequest(request);
        return;
    }

    // "base" call
    if (currentCall)
    {
        qDebug() << "Call" << currentCall->id << "progress" << reply.statusCode() << reply.reasonPhrase();
        if (reply.statusCode() == 200)
        {
            qDebug() << "Call" << currentCall->id << "established";

            if (!currentCall->audioOutput)
            {
                // prepare audio format
                QAudioFormat format;
                format.setFrequency(currentCall->channel->payloadType().clockrate());
                format.setChannels(currentCall->channel->payloadType().channels());
                format.setSampleSize(16);
                format.setCodec("audio/pcm");
                format.setByteOrder(QAudioFormat::LittleEndian);
                format.setSampleType(QAudioFormat::SignedInt);

                // initialise audio output
                currentCall->audioOutput = new QAudioOutput(format, this);
                currentCall->audioOutput->start(currentCall->channel);
            }

        } else if (reply.statusCode() >= 400) {
            qDebug() << "Call" << currentCall->id << "failed";
            d->calls.removeAll(currentCall);
            delete currentCall;
        }
    } else {
        if (command == "REGISTER" && reply.statusCode() == 200) {
            const QByteArray uri = QString("sip:%1@%2").arg(d->username, d->domain).toUtf8();
            SipRequest request = d->buildRequest("SUBSCRIBE", uri, d->baseId);
            request.setHeaderField("Expires", "3600");
            d->sendRequest(request);
        }
        else if (command == "SUBSCRIBE" && reply.statusCode() == 200) {
            const QString recipient = QString("sip:%1@%2").arg(d->phoneNumber, d->serverName);
            call(recipient);
        }
    }

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
    const QByteArrayMatcher lf("\n");
    const QByteArrayMatcher colon(":");

    int j = lf.indexIn(bytes);
    if (j < 0)
        return;

    // parse status
    const QByteArray status = bytes.left(j - 1);
    if (status.size() < 10 ||
        !status.startsWith("SIP/2.0"))
        return;

    m_statusCode = status.mid(8, 3).toInt();
    m_reasonPhrase = QString::fromUtf8(status.mid(12));

    const QByteArray header = bytes.mid(j+1);

    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    int i = 0;
    while (i < header.count()) {
        int j = colon.indexIn(header, i); // field-name
        if (j == -1)
            break;
        QByteArray field = header.mid(i, j - i).trimmed();
        j++;
        // any number of LWS is allowed before and after the value
        QByteArray value;
        do {
            i = lf.indexIn(header, j);
            if (i == -1)
                break;
            if (!value.isEmpty())
                value += ' ';
            // check if we have CRLF or only LF
            bool hasCR = (i && header[i-1] == '\r');
            int length = i -(hasCR ? 1: 0) - j;
            value += header.mid(j, length).trimmed();
            j = ++i;
        } while (i < header.count() && (header.at(i) == ' ' || header.at(i) == '\t'));
        if (i == -1)
            break; // something is wrong

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
    }
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

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion("1.0.0");

    /* Install signal handler */
    signal(SIGINT, signal_handler);
#ifdef SIGKILL
    signal(SIGKILL, signal_handler);
#endif
    signal(SIGTERM, signal_handler);

    QXmppLogger::getLogger()->setLoggingType(QXmppLogger::StdoutLogging);
    SipClient client;
    client.connectToServer("sip.wifirst.net", 5060);

    int ret = app.exec();
    client.disconnectFromServer();

    return ret;
}

