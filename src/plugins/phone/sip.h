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

#include <QObject>
#include <QPair>

#include "QXmppCallManager.h"
#include "QXmppLogger.h"

class QUdpSocket;
class QXmppSrvInfo;

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

class SipPacket
{
public:
    SipPacket(const QByteArray &ba = QByteArray());

    QByteArray body() const;
    void setBody(const QByteArray &body);

    quint32 sequenceNumber() const;

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
    void hangup();

private slots:
    void handleTimeout();
    void readFromSocket();
    void writeToSocket(const QByteArray &ba);

private:
    SipCall(const QString &recipient, QUdpSocket *socket, SipClient *parent);
    void handleReply(const SipPacket &reply);
    void handleRequest(const SipPacket &request);
    void sendInvite();
    void setState(QXmppCall::State state);

    SipCallPrivate * const d;
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
    QString serverName() const;
    SipClient::State state() const;

    void setDisplayName(const QString &displayName);
    void setDomain(const QString &domain);
    void setPassword(const QString &password);
    void setUsername(const QString &user);

signals:
    void connected();
    void disconnected();

    /// This signal is emitted when the call state changes.
    void stateChanged(SipClient::State state);

public slots:
    void connectToServer();
    void disconnectFromServer();

private slots:
    void callDestroyed(QObject *object);
    void connectToServer(const QXmppSrvInfo &info);
    void datagramReceived();
    void handleReply(const SipPacket &reply);

private:
    SipClientPrivate *const d;
    void setState(SipClient::State state);

    friend class SipCall;
    friend class SipClientPrivate;
};

#endif
