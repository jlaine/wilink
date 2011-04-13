/*
 * wiLink
 * Copyright (C) 2009-2011 Bolloré telecom
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

class QAudioDeviceInfo;
class QHostInfo;
class QUdpSocket;
class QTimer;
class QXmppRtpAudioChannel;
class QXmppSrvInfo;

class SipCallContext;
class SipCallPrivate;
class SipClient;
class SipClientPrivate;

extern const char *sipAddressPattern;
QString sipAddressToName(const QString &address);

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

public:
    ~SipCall();

    QXmppCall::Direction direction() const;
    int duration() const;
    QString error() const;
    QByteArray id() const;
    QString recipient() const;
    QXmppCall::State state() const;

    QXmppRtpAudioChannel *audioChannel() const;

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

    // This signal is emitted when the input volume changes.
    void inputVolumeChanged(int volume);

    // This signal is emitted when the output volume changes.
    void outputVolumeChanged(int volume);

public slots:
    void accept();
    void hangup();

private slots:
    void audioStateChanged();
    void handleTimeout();
    void localCandidatesChanged();
    void transactionFinished();

private:
    SipCall(const QString &recipient, QXmppCall::Direction direction, SipClient *parent);

    SipCallPrivate *d;
    friend class SipCallPrivate;
    friend class SipClient;
};

/// The SipClient class represents a SIP client capable of making and
/// receiving Voice-Over-IP calls.

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

    /// This signal is emitted when a new outgoing call is dialled.
    void callDialled(SipCall *call);

    /// This signal is emitted when a new incoming call is received.
    ///
    /// To accept the call, invoke the call's SipCall::accept() method.
    /// To refuse the call, invoke the call's SipCall::hangup() method.
    void callReceived(SipCall *call);

    /// This signal is emitted when the client state changes.
    void stateChanged(SipClient::State state);

public slots:
    SipCall *call(const QString &recipient);
    void connectToServer();
    void disconnectFromServer();
    void sendMessage(const SipMessage &message);
    void setAudioInputDevice(const QAudioDeviceInfo &device);
    void setAudioOutputDevice(const QAudioDeviceInfo &device);

private slots:
    void callDestroyed(QObject *object);
    void datagramReceived();
    void registerWithServer();
    void sendStun();
    void setSipServer(const QHostInfo &info);
    void setSipServer(const QXmppSrvInfo &info);
    void setStunServer(const QHostInfo &info);
    void setStunServer(const QXmppSrvInfo &info);
    void transactionFinished();

private:
    SipClientPrivate *d;
    friend class SipCall;
    friend class SipCallPrivate;
    friend class SipClientPrivate;
};

#endif
