/*
 * wiLink
 * Copyright (C) 2009-2010 Bolloré telecom
 * See AUTHORS file for a full list of contributors.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WILINK_PHONE_SIP_H__
#define __WILINK_PHONE_SIP_H__

#include <QHostAddress>
#include <QObject>
#include <QPair>

#include "QXmppCallManager.h"
#include "QXmppLogger.h"

class QUdpSocket;
class QTimer;
class QXmppSrvInfo;

class SipCallContext;
class SipCallPrivate;
class SipClient;
class SipClientPrivate;

QString sipAddressToName(const QString &address);

class StunTester : public QXmppLoggable
{
    Q_OBJECT

public:
    enum ConnectionType
    {
        NoConnection = 0,
        DirectConnection,
        NattedConnection,
    };

    StunTester(QObject *parent = 0);
    bool bind(const QHostAddress &address);
    void setServer(const QHostAddress &server, quint16 port);

signals:
    void finished(StunTester::ConnectionType result);

public slots:
    void start();

private slots:
    void readyRead();
    void timeout();

private:
    void sendRequest();

    QHostAddress serverAddress;
    quint16 serverPort;
    QByteArray requestId;
    int retries;
    QUdpSocket *socket;
    QTimer *timer;
};

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

class SipMessage
{
public:
    SipMessage(const QByteArray &ba = QByteArray());

    QByteArray body() const;
    void setBody(const QByteArray &body);

    quint32 sequenceNumber() const;

    bool hasHeaderField(const QByteArray &name) const;
    QByteArray headerField(const QByteArray &name, const QByteArray &defaultValue = QByteArray()) const;
    QMap<QByteArray, QByteArray> headerFieldParameters(const QByteArray &name) const;
    QList<QByteArray> headerFieldValues(const QByteArray &name) const;
    void addHeaderField(const QByteArray &name, const QByteArray &data);
    void removeHeaderField(const QByteArray &name);
    void setHeaderField(const QByteArray &name, const QByteArray &data);

    bool isReply() const;
    bool isRequest() const;

    // request
    QByteArray method() const;
    void setMethod(const QByteArray &method);

    QByteArray uri() const;
    void setUri(const QByteArray &method);

    // response
    QString reasonPhrase() const;
    void setReasonPhrase(const QString &reasonPhrase);

    int statusCode() const;
    void setStatusCode(int statusCode);

    QByteArray toByteArray() const;

protected:
    QList<QPair<QByteArray, QByteArray> > m_fields;

private:
    QByteArray m_body;
    QByteArray m_method;
    QByteArray m_uri;
    int m_statusCode;
    QString m_reasonPhrase;
};

class SipCall : public QXmppLoggable
{
    Q_OBJECT

public:
    ~SipCall();

    QXmppCall::Direction direction() const;
    QByteArray id() const;
    QString recipient() const;
    QXmppCall::State state() const;

signals:
    /// This signal is emitted when a call is connected.
    void connected();

    /// This signal is emitted when a call is finished.
    ///
    /// Note: Do not delete the call in the slot connected to this signal,
    /// instead use deleteLater().
    void finished();

    /// This signal is emitted when the remote party is ringing.
    void ringing();

    /// This signal is emitted when the call state changes.
    void stateChanged(QXmppCall::State state);

public slots:
    void accept();
    void hangup();

private slots:
    void audioStateChanged();
    void handleTimeout();
    void readFromSocket();
    void writeToSocket(const QByteArray &ba);

private:
    SipCall(const QString &recipient, QXmppCall::Direction direction, SipClient *parent);

    SipCallPrivate *d;
    friend class SipCallPrivate;
    friend class SipClient;
};

class SipClient : public QXmppLoggable
{
    Q_OBJECT

public:
    /// This enum is used to describe the state of a client.
    enum State
    {
        DisconnectedState = 0,
        ConnectingState = 1,
        ConnectedState = 2,
        DisconnectingState = 3,
    };

    SipClient(QObject *parent = 0);
    ~SipClient();

    SipCall *call(const QString &recipient);
    SipClient::State state() const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString domain() const;
    void setDomain(const QString &domain);

    QString password() const;
    void setPassword(const QString &password);

    QString username() const;
    void setUsername(const QString &user);

signals:
    void connected();
    void disconnected();

    /// This signal is emitted when a new incoming call is received.
    ///
    /// To accept the call, invoke the call's SipCall::accept() method.
    /// To refuse the call, invoke the call's SipCall::hangup() method.
    void callReceived(SipCall *call);

    /// This signal is emitted when the call state changes.
    void stateChanged(SipClient::State state);

public slots:
    void connectToServer();
    void disconnectFromServer();

private slots:
    void callDestroyed(QObject *object);
    void datagramReceived();
    void registerWithServer();
    void setSipServer(const QXmppSrvInfo &info);
    void setStunServer(const QXmppSrvInfo &info);
    void stunFinished(StunTester::ConnectionType type);

private:
    SipClientPrivate *d;
    friend class SipCall;
    friend class SipCallPrivate;
    friend class SipClientPrivate;
};

#endif
