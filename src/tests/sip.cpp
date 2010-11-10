#include <QByteArrayMatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppSaslAuth.h"
#include "QXmppUtils.h"
#include "sip.h"

class SipClientPrivate
{
public:
    void sendRegister();

    QUdpSocket *socket;
    QByteArray callId;
    int cseq;
    QByteArray tag;

    // configuration
    QString displayName;
    QString username;
    QString password;
    QString domain;

    QHostAddress serverAddress;
    QString serverName;
    quint16 serverPort;
    QMap<QByteArray, QByteArray> saslChallenge;
};

void SipClientPrivate::sendRegister()
{
    const QByteArray branch = "z9hG4bK-" + generateStanzaHash().toLatin1();
    const QString addr = QString("\"%1\"<sip:%2@%3>").arg(displayName,
        username, domain);
    const QString via = QString("SIP/2.0/UDP %1:%2;branch=%3;rport").arg(
        socket->localAddress().toString(),
        QString::number(socket->localPort()),
        branch);

    const QByteArray uri = QString("sip:%1").arg(serverName).toUtf8();
    SipRequest packet("REGISTER", uri);
    packet.setHeaderField("Via", via.toLatin1());
    packet.setHeaderField("Max-Forwards", "70");
    packet.setHeaderField("Contact", QString("<sip:%1@%2:%3;rinstance=%4>").arg(
        username, socket->localAddress().toString(),
        QString::number(socket->localPort()), "changeme").toUtf8());
    packet.setHeaderField("To", addr.toUtf8());
    packet.setHeaderField("From", addr.toUtf8() + ";tag=" + tag);
    packet.setHeaderField("Call-ID", callId);
    packet.setHeaderField("CSeq", QString("%1 REGISTER").arg(QString::number(cseq++)).toLatin1());
    packet.setHeaderField("Expires", "3600");
    packet.setHeaderField("User-Agent", QString("%1/%2").arg(qApp->applicationName(), qApp->applicationVersion()).toUtf8());

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
    socket->writeDatagram(packet.toByteArray(), serverAddress, serverPort);
}

SipClient::SipClient(QObject *parent)
    : QObject(parent),
    d(new SipClientPrivate)
{
    d->cseq = 1;
    d->socket = new QUdpSocket(this);
    connect(d->socket, SIGNAL(readyRead()),
            this, SLOT(datagramReceived()));
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
    d->domain = "wifirst.net";
    d->displayName = "Foo Bar";
    d->username = "foo";
    d->password = "bar";

    d->socket->bind();
    d->sendRegister();
}

void SipClient::datagramReceived()
{
    if (!d->socket->hasPendingDatagrams())
        return;

    const qint64 size = d->socket->pendingDatagramSize();
    QByteArray buffer(size, 0);
    QHostAddress remoteHost;
    quint16 remotePort;
    d->socket->readDatagram(buffer.data(), buffer.size(), &remoteHost, &remotePort);

    SipReply reply(buffer);
    if (reply.statusCode() == 401 && reply.headerField("WWW-Authenticate").startsWith("Digest "))
    {
        const QByteArray saslData = reply.headerField("WWW-Authenticate").mid(7);
        d->saslChallenge = QXmppSaslDigestMd5::parseMessage(saslData);

        if (!d->saslChallenge.contains("nonce"))
        {
            qWarning("Did not receive a nonce");
            //disconnectFromHost();
            return;
        }
        d->sendRegister();
    }
}

SipRequest::SipRequest(const QByteArray &method, const QByteArray &uri)
    : m_method(method),
    m_uri(uri)
{
}

QByteArray SipRequest::method() const
{
    return m_method;
}

QByteArray SipRequest::uri() const
{
    return m_uri;
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
    ba += "Content-Length: 0\r\n";
    ba += "\r\n";
    return ba;
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

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("wiLink");
    app.setApplicationVersion("1.0.0");
    SipClient client;
    client.connectToServer("sip.wifirst.net", 5060);

    return app.exec();
}

