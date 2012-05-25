/*
 * wiLink
 * Copyright (C) 2009-2012 Wifirst
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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include <QAbstractListModel>
#include <QList>
#include <QUrl>

#include "phone/sip.h"

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QSoundPlayer;
class QSoundStream;
class QTimer;
class PhoneHistoryItem;
class SipCall;
class SipClient;

/** The PhoneHistoryModel class represents the user's phone call history.
 */
class PhoneHistoryModel : public QObject
{
    Q_OBJECT
    Q_ENUMS(Role)
    Q_PROPERTY(SipClient* client READ client CONSTANT)
    Q_PROPERTY(int currentCalls READ currentCalls NOTIFY currentCallsChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setUsername NOTIFY passwordChanged)

public:
    enum Role {
        AddressRole = Qt::UserRole,
        ActiveRole,
        DateRole,
        FlagsRole,
        DurationRole,
        IdRole,
        StateRole,
    };

    PhoneHistoryModel(QObject *parent = 0);
    ~PhoneHistoryModel();

    SipClient *client() const;
    int currentCalls() const;
    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString username() const;
    void setUsername(const QString &username);

    QString password() const;
    void setPassword(const QString &password);

    // QAbstractListModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void currentCallsChanged();
    void enabledChanged(bool enabled);
    void error(const QString &error);
    void inputVolumeChanged(int inputVolume);
    void outputVolumeChanged(int outputVolume);
    void urlChanged();
    void usernameChanged();
    void passwordChanged();

public slots:
    bool call(const QString &address);
    void startTone(int tone);
    void stopTone(int tone);

private slots:
    void _q_callStarted(SipCall *call);
    void _q_callStateChanged(SipCall::State state);
    void _q_openUrl(const QUrl &url);

private:
    QModelIndex createIndex(PhoneHistoryItem *item);

    SipClient *m_client;
    QList<PhoneHistoryItem*> m_items;
    QNetworkAccessManager *m_network;
    QSoundPlayer *m_player;
    bool m_registeredHandler;
    QUrl m_url;
    QString m_username;
    QString m_password;

    QMap<SipCall*, QSoundStream*> m_activeCalls;
};

#endif
