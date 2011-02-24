#include <QHostAddress>
#include <QObject>

#include "QXmppLogger.h"
#include "QXmppStun.h"

class QUdpSocket;

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

signals:
    void connected();
    void disconnected();

public slots:
    void connectToHost();
    void disconnectFromHost();

private slots:
    void readyRead();

private:
    qint64 writeStun(const QXmppStunMessage &message);

    QUdpSocket *socket;
    QString m_password;
    QString m_username;
    QHostAddress m_relayedHost;
    quint16 m_relayedPort;
    QHostAddress m_turnHost;
    quint16 m_turnPort;

    // permissions
    QHostAddress m_peerHost;
    quint16 m_peerPort;

    // state
    QByteArray m_key;
    QString m_realm;
    QByteArray m_nonce;
    QXmppStunMessage m_request;
    AllocationState m_state;
};


