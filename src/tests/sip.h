#include <QObject>
#include <QPair>

class SipClientPrivate;

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
    SipRequest(const QByteArray &method, const QByteArray &uri);
    QByteArray method() const;
    QByteArray uri() const;
    QByteArray toByteArray() const;

private:
    QByteArray m_method;
    QByteArray m_uri;
};

class SipReply : public SipPacket
{
public:
    SipReply(const QByteArray &bytes);
    int statusCode() const;

private:
    int m_statusCode;
    QString m_reasonPhrase;
};

class SipClient : public QObject
{
    Q_OBJECT

public:
    SipClient(QObject *parent = 0);
    ~SipClient();

    void connectToServer(const QString &server, quint16 port);

private slots:
    void datagramReceived();

private:
    SipClientPrivate *const d;
};

