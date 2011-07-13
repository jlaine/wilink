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

#ifndef __WILINK_PHONE_H__
#define __WILINK_PHONE_H__

#include <QAbstractListModel>
#include <QList>
#include <QUrl>

#include "QXmppCallManager.h"

class QAuthenticator;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QTimer;
class PhoneContactItem;
class PhoneHistoryItem;
class SipCall;
class SipClient;

/** The PhoneContactModel class represents the user's phone contacts.
 */
class PhoneContactModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

public:
    enum Role {
        IdRole = Qt::UserRole,
        NameRole,
        PhoneRole,
    };

    PhoneContactModel(QObject *parent = 0);

    QUrl url() const;
    void setUrl(const QUrl &url);

    // QAbstractListModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void urlChanged(const QUrl &url);

public slots:
    void addContact(const QString &name, const QString &phone);

private slots:
    void _q_handleCreate();
    void _q_handleList();

private:
    QList<PhoneContactItem*> m_items;
    QNetworkAccessManager *m_network;
    QUrl m_url;
};

/** The PhoneHistoryModel class represents the user's phone call history.
 */
class PhoneHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(SipClient* client READ client CONSTANT)
    Q_PROPERTY(QUrl contactsUrl READ contactsUrl NOTIFY contactsUrlChanged)
    Q_PROPERTY(int currentCalls READ currentCalls NOTIFY currentCallsChanged)
    Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)
    Q_PROPERTY(QString phoneNumber READ phoneNumber NOTIFY phoneNumberChanged)
    Q_PROPERTY(QUrl selfcareUrl READ selfcareUrl NOTIFY selfcareUrlChanged)
    Q_PROPERTY(QString voicemailNumber READ voicemailNumber NOTIFY voicemailNumberChanged)

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

    PhoneHistoryModel(QObject *parent = 0);
    ~PhoneHistoryModel();

    SipClient *client() const;
    QUrl contactsUrl() const;
    int currentCalls() const;
    bool enabled() const;
    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;
    QString phoneNumber() const;
    QUrl selfcareUrl() const;
    QUrl url() const;
    void setUrl(const QUrl &url);
    QString voicemailNumber() const;

    // QAbstractListModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Q_INVOKABLE bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

signals:
    void contactsUrlChanged(const QUrl &contactsUrl);
    void currentCallsChanged();
    void enabledChanged(bool enabled);
    void error(const QString &error);
    void inputVolumeChanged(int inputVolume);
    void outputVolumeChanged(int outputVolume);
    void phoneNumberChanged(const QString &phoneNumber);
    void selfcareUrlChanged(const QUrl &selfcareUrl);
    void urlChanged(const QUrl &url);
    void voicemailNumberChanged(const QString &voicemailNumber);

public slots:
    void addCall(SipCall *call);
    bool call(const QString &address);
    void hangup();
    void startTone(int tone);
    void stopTone(int tone);

private slots:
    void callRinging();
    void callStateChanged(QXmppCall::State state);
    void callTick();
    void _q_getSettings();
    void _q_handleCreate();
    void _q_handleList();
    void _q_handleSettings();
    void _q_openUrl(const QUrl &url);

private:
    QList<SipCall*> activeCalls() const;

    SipClient *m_client;
    QUrl m_contactsUrl;
    bool m_enabled;
    QList<PhoneHistoryItem*> m_items;
    QNetworkAccessManager *m_network;
    QString m_phoneNumber;
    bool m_registeredHandler;
    QUrl m_selfcareUrl;
    QTimer *m_ticker;
    QTimer *m_timer;
    QUrl m_url;
    QString m_voicemailNumber;
};

#endif
