#include <QObject>
#include <QPair>

class SipClientPrivate;

class SipPacket
{
public:
    SipPacket(const QByteArray &method, const QByteArray &uri);
    QByteArray toByteArray() const;

    QByteArray headerField(const QByteArray &name, const QByteArray &defaultValue = QByteArray()) const;
    QList<QByteArray> headerFieldValues(const QByteArray &name) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);

private:
    QByteArray m_method;
    QByteArray m_uri;
    QList<QPair<QByteArray, QByteArray> > m_fields;
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

