#include <QObject>
#include <QPair>

class QUdpSocket;

class SipCallPrivate;
class SipClientPrivate;

class SdpMessage
{
public:
    SdpMessage(const QByteArray &ba = QByteArray());
    void addField(char name, const QByteArray &data);
    QList<QPair<char, QByteArray> > fields() const;
    QByteArray toByteArray() const;

private:
    QList<QPair<char, QByteArray> > m_fields;
};

class SipPacket
{
public:
    QByteArray headerField(const QByteArray &name, const QByteArray &defaultValue = QByteArray()) const;
    QList<QByteArray> headerFieldValues(const QByteArray &name) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);

protected:
    QList<QPair<QByteArray, QByteArray> > m_fields;
};

class SipRequest : public SipPacket
{
public:
    SipRequest();

    QByteArray body() const;
    void setBody(const QByteArray &body);

    QByteArray method() const;
    void setMethod(const QByteArray &method);

    QByteArray uri() const;
    void setUri(const QByteArray &method);

    QByteArray toByteArray() const;

private:
    QByteArray m_body;
    QByteArray m_method;
    QByteArray m_uri;
};

class SipReply : public SipPacket
{
public:
    SipReply(const QByteArray &bytes);

    QByteArray body() const;
    QString reasonPhrase() const;
    int statusCode() const;

private:
    QByteArray m_body;
    int m_statusCode;
    QString m_reasonPhrase;
};

class SipCall : public QObject
{
    Q_OBJECT

public:
    ~SipCall();

    QByteArray id() const;

private slots:
    void readFromSocket();
    void writeToSocket(const QByteArray &ba);

private:
    SipCall(QUdpSocket *socket, QObject *parent = 0);

    SipCallPrivate * const d;
    friend class SipClient;
};

class SipClient : public QObject
{
    Q_OBJECT

public:
    SipClient(QObject *parent = 0);
    ~SipClient();

    void call(const QString &recipient);
    void connectToServer(const QString &server, quint16 port);
    void disconnectFromServer();

private slots:
    void datagramReceived();

private:
    SipClientPrivate *const d;
};

