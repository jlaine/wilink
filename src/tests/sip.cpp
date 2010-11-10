#include <QByteArrayMatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QSettings>
#include <QUdpSocket>

#include "QXmppSaslAuth.h"
#include "QXmppUtils.h"
#include "sip.h"

class SipClientPrivate
{
public:
    SipRequest buildRequest(const QByteArray &method, const QByteArray &uri);
    void sendRegister();
    void sendRequest(const SipRequest &request);

    QUdpSocket *socket;
    QByteArray callId;
    int cseq;
    QByteArray tag;

    // configuration
    QString displayName;
    QString username;
    QString password;
    QString domain;
    QString phoneNumber;

    QHostAddress serverAddress;
    QString serverName;
    quint16 serverPort;
    SipRequest lastRequest;
    QMap<QByteArray, QByteArray> saslChallenge;
};

SipRequest SipClientPrivate::buildRequest(const QByteArray &method, const QByteArray &uri)
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
        QString::number(socket->localPort()), "changeme").toUtf8());
    packet.setHeaderField("To", addr.toUtf8());
    packet.setHeaderField("From", addr.toUtf8() + ";tag=" + tag);
    packet.setHeaderField("Allow", "INVITE, ACK, CANCEL, OPTIONS, BYE, REFER, NOTIFY, MESSAGE, SUBSCRIBE, INFO");
    packet.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());
    return packet;
}

void SipClientPrivate::sendRegister()
{
    const QByteArray uri = QString("sip:%1").arg(serverName).toUtf8();
    SipRequest packet = buildRequest("REGISTER", uri);
    packet.setHeaderField("Expires", "3600");

    if (!saslChallenge.isEmpty())
    {
        QXmppSaslDigestMd5 digest;
        digest.setCnonce(digest.generateNonce());
        digest.setNc("00000001");
        digest.setNonce(saslChallenge.value("nonce"));
        digest.setQop(saslChallenge.value("qop"));
        digest.setRealm(saslChallenge.value("realm"));
        digest.setUsername(username.toUtf8());
        digest.setPassword(password.toUtf8());

        const QByteArray A1 = digest.username() + ':' + digest.realm() + ':' + password.toUtf8();
        const QByteArray A2 = packet.method() + ':' + packet.uri();

        QMap<QByteArray, QByteArray> response;
        response["username"] = digest.username();
        response["realm"] = digest.realm();
        response["nonce"] = digest.nonce();
        response["uri"] = uri;
        response["response"] = digest.calculateDigest(A1, A2);
        response["cnonce"] = digest.cnonce();
        response["qop"] = digest.qop();
        response["nc"] = digest.nc();
        response["algorithm"] = "MD5";
        packet.setHeaderField("Authorization", "Digest " + QXmppSaslDigestMd5::serializeMessage(response));
    }
    sendRequest(packet);
}

void SipClientPrivate::sendRequest(const SipRequest &request)
{
    lastRequest = request;
    socket->writeDatagram(request.toByteArray(), serverAddress, serverPort);
}

SipClient::SipClient(QObject *parent)
    : QObject(parent),
    d(new SipClientPrivate)
{
    d->cseq = 1;
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

void SipClient::connectToServer(const QString &server, quint16 port)
{
    QHostInfo info = QHostInfo::fromName(server);
    if (info.addresses().isEmpty())
        return;
    d->callId = generateStanzaHash().toLatin1();
    d->tag = generateStanzaHash(8).toLatin1();
    d->serverAddress = info.addresses().first();
    d->serverName = server;
    d->serverPort = port;

    d->socket->bind();
    d->sendRegister();
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
    QByteArray command = cseq.mid(i+1);
    qDebug() << "command" << command;

    // handle reply
    if (command == "REGISTER") {
        if (reply.statusCode() == 200) {
            const QByteArray uri = QString("sip:%1@%2").arg(d->username, d->domain).toUtf8();
            SipRequest request = d->buildRequest("SUBSCRIBE", uri);
            request.setHeaderField("Expires", "3600");
            d->sendRequest(request);
        }
        else if (reply.statusCode() == 401) {
            const QByteArray auth = reply.headerField("WWW-Authenticate");
            if (!auth.startsWith("Digest ")) {
                qWarning("Unsupported authentication method");
                return;
            }

            d->saslChallenge = QXmppSaslDigestMd5::parseMessage(auth.mid(7));
            if (!d->saslChallenge.contains("nonce")) {
                qWarning("Did not receive a nonce");
                return;
            }
            d->sendRegister();
        }
    }
    else if (command == "SUBSCRIBE") {
        const QByteArray recipient = QString("sip:%1@%2").arg(d->phoneNumber, d->serverName).toUtf8();
        SipRequest request = d->buildRequest("INVITE", recipient);
        request.setHeaderField("To", "<" + recipient + ">");
        request.setHeaderField("Content-Type", "application/sdp");
        request.setBody("v=0\r\n");
        d->sendRequest(request);
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
    ba += SipPacket::toByteArray();
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
    m_reasonPhrase = status.mid(12);

    const QByteArray header = bytes.mid(j+1);

    // see rfc2616, sec 4 for information about HTTP/1.1 headers.
    // allows relaxed parsing here, accepts both CRLF & LF line endings
    int i = 0;
    while (i < header.count()) {
        int j = colon.indexIn(header, i); // field-name
        if (j == -1)
            break;
        const QByteArray field = header.mid(i, j - i).trimmed();
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

        m_fields.append(qMakePair(field, value));
    }
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

QByteArray SipPacket::toByteArray() const
{
    QList<QPair<QByteArray, QByteArray> >::ConstIterator it = m_fields.constBegin(),
                                                        end = m_fields.constEnd();
    QByteArray ba;
    for ( ; it != end; ++it) {
        ba += it->first;
        ba += ": ";
        ba += it->second;
        ba += "\r\n";
    }
    return ba;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion("1.0.0");

    SipClient client;
    client.connectToServer("sip.wifirst.net", 5060);

    return app.exec();
}

