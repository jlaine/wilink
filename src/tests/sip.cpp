#include <QByteArrayMatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QHostInfo>
#include <QUdpSocket>

#include "QXmppUtils.h"
#include "sip.h"

class SipClientPrivate
{
public:
    QUdpSocket *socket;
    QByteArray callId;
    QHostAddress serverAddress;
    quint16 serverPort;
};

SipClient::SipClient(QObject *parent)
    : QObject(parent),
    d(new SipClientPrivate)
{
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
    d->serverAddress = info.addresses().first();
    d->serverPort = port;

    d->socket->bind();
    //d->socket->connectToHost(d->serverAddress, d->serverPort);
    const QByteArray branch = "z9hG4bK-" + generateStanzaHash().toLatin1();
    const QString addr = QString("\"%1\"<%2>").arg(
        QLatin1String("Jeremy Laine"), QLatin1String("jeremy.laine@wifirst.net"));
    const QString via = QString("SIP/2.0/UDP %1:%2;branch=%3;rport").arg(
        d->socket->localAddress().toString(),
        QString::number(d->socket->localPort()),
        branch);

    SipPacket packet("REGISTER", "sip:sip.wifirst.net");
    packet.setHeaderField("Via", via.toLatin1());
    packet.setHeaderField("Max-Forwards", "70");
    packet.setHeaderField("To", addr.toLatin1());
    packet.setHeaderField("From", addr.toLatin1() + ";tag=123456");
    packet.setHeaderField("Call-ID", d->callId);
    packet.setHeaderField("CSeq", "1 REGISTER");
    packet.setHeaderField("Expires", "3600");
    packet.setHeaderField("User-Agent", "wiLink/1.0.0");
    d->socket->writeDatagram(packet.toByteArray(), d->serverAddress, d->serverPort);
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

    SipPacket reply(buffer);
    qDebug() << "repl" << reply.headerField("Via");
}

SipPacket::SipPacket(const QByteArray &method, const QByteArray &uri)
    : m_method(method),
    m_uri(uri)
{
}

SipPacket::SipPacket(const QByteArray &bytes)
{
    // parse status
    const QByteArrayMatcher lf("\n");
    const QByteArrayMatcher colon(":");
    int j = lf.indexIn(bytes);
    if (j < 0)
        return;

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
    QByteArray ba;
    ba += m_method;
    ba += ' ';
    ba += m_uri;
    ba += " SIP/2.0\r\n";

    for (int i = 0; i < m_fields.size(); ++i) {
        ba += m_fields[i].first;
        ba += ": ";
        ba += m_fields[i].second;
        ba += "\r\n";
    }
    ba += "Content-Length: 0\r\n";
    ba += "\r\n";
    return ba;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    SipClient client;
    client.connectToServer("sip.wifirst.net", 5060);

    return app.exec();
}

