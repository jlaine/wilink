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

#ifndef __SIP_P_H__
#define __SIP_P_H__

#include <QDnsLookup>
#include <QHostAddress>
#include <QMap>
#include <QObject>

#include "sip.h"

class QUdpSocket;
class QTimer;

/** The SipCallContext class represents a SIP dialog.
 */
class SipCallContext
{
public:
    SipCallContext();
    bool handleAuthentication(const SipMessage &reply);

    quint32 cseq;
    QByteArray id;
    QByteArray tag;

    QMap<QByteArray, QByteArray> challenge;
    QMap<QByteArray, QByteArray> proxyChallenge;
    QList<SipTransaction*> transactions;
};

class SipCallPrivate : public SipCallContext
{
public:
    SipCallPrivate(SipCall *qq);
    SdpMessage buildSdp(const QList<QXmppJinglePayloadType> &payloadTypes) const;
    void handleReply(const SipMessage &reply);
    void handleRequest(const SipMessage &request);
    bool handleSdp(const SdpMessage &sdp);
    void onStateChanged();
    void sendInvite();
    void setState(SipCall::State state);

    SipCall::Direction direction;
    QString errorString;
    QDateTime startStamp;
    QDateTime finishStamp;
    SipCall::State state;
    QByteArray activeTime;
    QXmppRtpAudioChannel *audioChannel;
    QXmppIceConnection *iceConnection;
    bool invitePending;
    bool inviteQueued;
    SipMessage inviteRequest;
    QHostAddress localRtpAddress;
    quint16 localRtpPort;
    QByteArray remoteRecipient;
    QList<QByteArray> remoteRoute;
    QHostAddress remoteRtpAddress;
    quint16 remoteRtpPort;
    QByteArray remoteUri;

    SipClient *client;
    QTimer *durationTimer;
    QTimer *timeoutTimer;

private:
    SipCall *q;
};

class SipClientPrivate : public SipCallContext
{
public:
    SipClientPrivate(SipClient *qq);
    SipMessage buildRequest(const QByteArray &method, const QByteArray &uri, SipCallContext *ctx, int seq);
    SipMessage buildResponse(const SipMessage &request);
    SipMessage buildRetry(const SipMessage &original, SipCallContext *ctx);
    void handleReply(const SipMessage &reply);
    void setState(SipClient::State state);

    // timers
    QTimer *connectTimer;
    QTimer *stunTimer;

    // configuration
    QString displayName;
    QString username;
    QString password;
    QString domain;

    QXmppLogger *logger;
    SipClient::State state;
    QHostAddress serverAddress;
    quint16 serverPort;
    QList<SipCall*> calls;

    // sockets
    QDnsLookup sipDns;
    QHostAddress localAddress;
    QUdpSocket *socket;

    // STUN
    quint32 stunCookie;
    QDnsLookup stunDns;
    bool stunDone;
    QByteArray stunId;
    QHostAddress stunReflexiveAddress;
    quint16 stunReflexivePort;
    QHostAddress stunServerAddress;
    quint16 stunServerPort;

private:
    QByteArray authorization(const SipMessage &request, const QMap<QByteArray, QByteArray> &challenge) const;
    void setContact(SipMessage &request);
    SipClient *q;
};

#endif
