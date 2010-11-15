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

#include "QXmppLogger.h"

class QUdpSocket;
class QXmppSrvInfo;

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

    SipCall *call(const QString &recipient);
    void connectToServer();
    void disconnectFromServer();
    QString serverName() const;

    void setDisplayName(const QString &displayName);
    void setDomain(const QString &domain);
    void setPassword(const QString &password);
    void setUsername(const QString &user);

signals:
    void connected();
    void disconnected();

    /// This signal is emitted to send logging messages.
    void logMessage(QXmppLogger::MessageType type, const QString &msg);

private slots:
    void connectToServer(const QXmppSrvInfo &info);
    void datagramReceived();

private:
    void debug(const QString &msg);
    void warning(const QString &msg);
    SipClientPrivate *const d;
    friend class SipClientPrivate;
};

#endif
