/*
 * Copyright (C) 2010-2012 Jeremy Lainé
 * Contact: http://code.google.com/p/qsip/
 *
 * This file is a part of the QSip library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef __SIP_H__
#define __SIP_H__

#include <QHostAddress>
#include <QObject>
#include <QPair>

#include "QXmppLogger.h"

class QHostInfo;
class QUdpSocket;
class QTimer;
class QXmppIceConnection;
class QXmppRtpAudioChannel;

class SipCallContext;
class SipCallPrivate;
class SipClient;
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

/** The SipMessage class represents a SIP request or response.
 */
class SipMessage
{
public:
    SipMessage(const QByteArray &ba = QByteArray());

    QByteArray body() const;
    void setBody(const QByteArray &body);

    quint32 sequenceNumber() const;

    QByteArray headerField(const QByteArray &name) const;
    QList<QByteArray> headerFieldValues(const QByteArray &name) const;
    void addHeaderField(const QByteArray &name, const QByteArray &data);
    void removeHeaderField(const QByteArray &name);
    void setHeaderField(const QByteArray &name, const QByteArray &data);

    static QMap<QByteArray, QByteArray> valueParameters(const QByteArray &value);

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

/** The SipTransaction class represents a non-INVITE SIP transaction.
 */
class SipTransaction : public QXmppLoggable
{
    Q_OBJECT
    Q_ENUMS(State)

public:
    enum State
    {
        Trying,
        Proceeding,
        Completed,
        Terminated,
    };

    SipTransaction(const SipMessage &request, SipClient *client, QObject *parent);
    QByteArray branch() const;
    SipMessage request() const;
    SipMessage response() const;
    State state() const;

signals:
    void finished();
    void sendMessage(const SipMessage &message);

public slots:
    void messageReceived(const SipMessage &message);

private slots:
    void retry();
    void timeout();

private:
    SipMessage m_request;
    SipMessage m_response;
    State m_state;
    QTimer *m_retryTimer;
    QTimer *m_timeoutTimer;
};

/// The SipCall class represents a SIP Voice-Over-IP call.

class SipCall : public QXmppLoggable
{
    Q_OBJECT
    Q_ENUMS(Direction State)
    Q_PROPERTY(QXmppRtpAudioChannel* audioChannel READ audioChannel CONSTANT)
    Q_PROPERTY(Direction direction READ direction CONSTANT)
    Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)
    Q_PROPERTY(QString recipient READ recipient CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    /// This enum is used to describe the direction of a call.
    enum Direction
    {
        IncomingDirection, ///< The call is incoming.
        OutgoingDirection, ///< The call is outgoing.
    };

    /// This enum is used to describe the state of a call.
    enum State
    {
        ConnectingState = 0,    ///< The call is being connected.
        ActiveState = 1,        ///< The call is active.
        DisconnectingState = 2, ///< The call is being disconnected.
        FinishedState = 3,      ///< The call is finished.
    };

    ~SipCall();

    SipCall::Direction direction() const;
    int duration() const;
    QString errorString() const;
    QByteArray id() const;
    QString recipient() const;
    SipCall::State state() const;

    QHostAddress localRtpAddress() const;
    void setLocalRtpAddress(const QHostAddress &address);

    quint16 localRtpPort() const;
    void setLocalRtpPort(quint16 port);

    QHostAddress remoteRtpAddress() const;
    quint16 remoteRtpPort() const;

    QXmppRtpAudioChannel *audioChannel() const;
    QXmppIceConnection *audioConnection() const;

signals:
    /// This signal is emitted when a call is connected.
    void connected();

    /// This signal is emitted when the call duration changes.
    void durationChanged();

    /// This signal is emitted when a call is finished.
    ///
    /// Note: Do not delete the call in the slot connected to this signal,
    /// instead use deleteLater().
    void finished();

    /// This signal is emitted when the remote party is ringing.
    void ringing();

    /// This signal is emitted when the call state changes.
    void stateChanged(SipCall::State state);

public slots:
    void accept();
    void hangup();
    void destroyLater();

private slots:
    void handleTimeout();
    void localCandidatesChanged();
    void transactionFinished();

private:
    SipCall(const QString &recipient, SipCall::Direction direction, SipClient *parent);

    SipCallPrivate *d;
    friend class SipCallPrivate;
    friend class SipClient;
};

/// The SipClient class represents a SIP client capable of making and
/// receiving Voice-Over-IP calls.

class SipClient : public QXmppLoggable
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(int activeCalls READ activeCalls NOTIFY activeCallsChanged)
    Q_PROPERTY(QXmppLogger* logger READ logger WRITE setLogger NOTIFY loggerChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword)
    Q_PROPERTY(QString username READ username WRITE setUsername)

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

    int activeCalls() const;

    QString displayName() const;
    void setDisplayName(const QString &displayName);

    QString domain() const;
    void setDomain(const QString &domain);

    QHostAddress localAddress() const;

    QXmppLogger *logger() const;
    void setLogger(QXmppLogger *logger);

    QString password() const;
    void setPassword(const QString &password);

    SipClient::State state() const;

    QString username() const;
    void setUsername(const QString &user);

signals:
    void connected();
    void disconnected();

    /// This signal is emitted when the number of active calls changes.
    void activeCallsChanged(int calls);

    /// This signal is emitted when a new incoming call is received.
    ///
    /// To accept the call, invoke the call's SipCall::accept() method.
    /// To refuse the call, invoke the call's SipCall::hangup() method.
    void callReceived(SipCall *call);

    /// This signal is emitted when a call (incoming or outgoing) is started.
    void callStarted(SipCall *call);

    /// This signal is emitted when the domain changes.
    void domainChanged(const QString domain);

    /// This signal is emitted when the logger changes.
    void loggerChanged(QXmppLogger *logger);

    /// This signal is emitted when the client state changes.
    void stateChanged(SipClient::State state);

public slots:
    SipCall *call(const QString &recipient);
    void connectToServer();
    void disconnectFromServer();
    void sendMessage(const SipMessage &message);

private slots:
    void callDestroyed(QObject *object);
    void datagramReceived();
    void registerWithServer();
    void sendStun();
    void _q_sipDnsLookupFinished();
    void _q_sipHostInfoFinished(const QHostInfo &info);
    void _q_stunDnsLookupFinished();
    void _q_stunHostInfoFinished(const QHostInfo &info);
    void transactionFinished();

private:
    SipClientPrivate *d;
    friend class SipCall;
    friend class SipCallPrivate;
    friend class SipClientPrivate;
};

#endif
