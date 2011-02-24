#include <QHostAddress>
#include <QObject>

#include "QXmppLogger.h"
#include "QXmppStun.h"

class QUdpSocket;
class QTimer;

class TurnAllocation : public QXmppLoggable
{
    Q_OBJECT

    enum AllocationState
    {
        UnconnectedState,
        ConnectingState,
        ConnectedState,
        ClosingState,
    };

public:
    TurnAllocation(QObject *parent = 0);

    QHostAddress relayedHost() const;
    quint16 relayedPort() const;

    void setServer(const QHostAddress &host, quint16 port = 3478);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

    void writeDatagram(const QByteArray &data, const QHostAddress &host, quint16 port);

signals:
    void connected();
    void datagramReceived(const QByteArray &data, const QHostAddress &host, quint16 port);
    void disconnected();

public slots:
    void connectToHost();
    void disconnectFromHost();

private slots:
    void readyRead();
    void refresh();

private:
    void setState(AllocationState state);
    qint64 writeStun(const QXmppStunMessage &message);

    QUdpSocket *socket;
    QTimer *timer;
    QString m_password;
    QString m_username;
    QHostAddress m_relayedHost;
    quint16 m_relayedPort;
    QHostAddress m_turnHost;
    quint16 m_turnPort;

    // permissions
    typedef QPair<QHostAddress, quint16> Address;
    QMap<Address, QByteArray> m_permissions;

    // state
    quint32 m_lifetime;
    QByteArray m_key;
    QString m_realm;
    QByteArray m_nonce;
    QXmppStunMessage m_request;
    AllocationState m_state;
};

class TurnTester : public QObject
{
    Q_OBJECT

public:
    TurnTester(TurnAllocation *allocation);

signals:
    void finished();

private slots:
    void connected();
    void datagramReceived(const QByteArray &data, const QHostAddress &host, quint16 port);

private:
    TurnAllocation *m_allocation;
};

