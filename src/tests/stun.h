#include <QHostAddress>
#include <QObject>

#include "QXmppStun.h"

class QUdpSocket;

class Turn : public QObject
{
    Q_OBJECT

public:
    Turn(QObject *parent = 0);
    void setTurnServer(const QHostAddress &host, quint16 port = 3478);
    void setUsername(const QString &username);
    void setPassword(const QString &password);

signals:
    void finished();

public slots:
    void connectToHost();

private slots:
    void readyRead();

private:
    qint64 writeStun(const QXmppStunMessage &message);

    QUdpSocket *socket;
    QString m_password;
    QString m_username;
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
};


