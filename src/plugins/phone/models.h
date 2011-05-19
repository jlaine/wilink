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
#include <QTableView>

#include "QXmppCallManager.h"

class QNetworkAccessManager;
class QNetworkRequest;
class QSortFilterProxyModel;
class QTimer;
class PhoneCallsItem;
class SipCall;
class SipClient;

class PhoneCallsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QList<SipCall*> activeCalls READ activeCalls NOTIFY activeCallsChanged)
    Q_PROPERTY(int currentCalls READ currentCalls NOTIFY activeCallsChanged)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int maximumVolume READ maximumVolume CONSTANT)
    Q_PROPERTY(int outputVolume READ outputVolume NOTIFY outputVolumeChanged)

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

    PhoneCallsModel(SipClient *client, QNetworkAccessManager *network, QObject *parent = 0);
    ~PhoneCallsModel();

    QList<SipCall*> activeCalls() const;
    int currentCalls() const;
    int inputVolume() const;
    int maximumVolume() const;
    int outputVolume() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    void setUrl(const QUrl &url);

signals:
    void activeCallsChanged();
    void error(const QString &error);
    void inputVolumeChanged(int);
    void outputVolumeChanged(int);
    void stateChanged(bool haveCalls);

public slots:
    void addCall(SipCall *call);
    bool call(const QString &address);
    void hangup();

private slots:
    void callRinging();
    void callStateChanged(QXmppCall::State state);
    void callTick();
    void handleCreate();
    void handleList();

private:
    QNetworkRequest buildRequest(const QUrl &url) const;

    SipClient *m_client;
    QList<PhoneCallsItem*> m_items;
    QNetworkAccessManager *m_network;
    QTimer *m_ticker;
    QUrl m_url;
};

class PhoneCallsView : public QTableView
{
    Q_OBJECT

public:
    PhoneCallsView(PhoneCallsModel *model, QWidget *parent = 0);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void resizeEvent(QResizeEvent *e);

protected slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private slots:
    void callSelected();
    void removeSelected();

private:
    PhoneCallsModel *m_callsModel;
    QSortFilterProxyModel *m_sortedModel;
};

#endif
