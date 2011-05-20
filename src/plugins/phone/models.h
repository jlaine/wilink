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

#ifndef __WILINK_PHONE_MODELS_H__
#define __WILINK_PHONE_MODELS_H__

#include <QAbstractListModel>
#include <QList>
#include <QUrl>

#include "QXmppCallManager.h"
#include "QXmppRtpChannel.h"

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QTimer;
class PhoneCallsItem;
class SipCall;
class SipClient;

class PhoneCallsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(SipClient* client READ client CONSTANT)
    Q_PROPERTY(int currentCalls READ currentCalls NOTIFY currentCallsChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)
    Q_PROPERTY(QString phoneNumber READ phoneNumber NOTIFY phoneNumberChanged)
    Q_PROPERTY(QUrl selfcareUrl READ selfcareUrl NOTIFY selfcareUrlChanged)

public:
    enum Role {
        AddressRole = Qt::UserRole,
        ActiveRole,
        DirectionRole,
        DateRole,
        DurationRole,
        IdRole,
        NameRole,
        StateRole,
    };

    PhoneCallsModel(QObject *parent = 0);
    ~PhoneCallsModel();

    SipClient *client() const;
    int currentCalls() const;
    bool enabled() const;
    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;
    QString phoneNumber() const;
    QUrl selfcareUrl() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void currentCallsChanged();
    void enabledChanged(bool enabled);
    void error(const QString &error);
    void inputVolumeChanged(int inputVolume);
    void outputVolumeChanged(int outputVolume);
    void phoneNumberChanged(const QString &phoneNumber);
    void selfcareUrlChanged(const QUrl& selfcareUrl);

public slots:
    void addCall(SipCall *call);
    bool call(const QString &address);
    void hangup();
    void startTone(QXmppRtpAudioChannel::Tone tone);
    void stopTone(QXmppRtpAudioChannel::Tone tone);

private slots:
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void callRinging();
    void callStateChanged(QXmppCall::State state);
    void callTick();
    void getSettings();
    void handleCreate();
    void handleList();
    void handleSettings();

private:
    QList<SipCall*> activeCalls() const;
    QNetworkRequest buildRequest(const QUrl &url) const;

    SipClient *m_client;
    bool m_enabled;
    QList<PhoneCallsItem*> m_items;
    QNetworkAccessManager *m_network;
    QString m_phoneNumber;
    QUrl m_selfcareUrl;
    QTimer *m_ticker;
    QUrl m_url;
};

#endif
