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

#include <QCoreApplication>
#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QNetworkDiskCache>
#include <QNetworkRequest>

#include "QXmppCallManager.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMucManager.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "QSoundPlayer.h"
#include "QSoundTester.h"

#include "qnetio/wallet.h"

#include "application.h"
#include "idle/idle.h"
#include "calls.h"
#include "client.h"
#include "console.h"
#include "conversations.h"
#include "declarative.h"
#include "diagnostics.h"
#include "discovery.h"
#include "history.h"
#include "phone.h"
#include "phone/sip.h"
#include "photos.h"
#include "player.h"
#include "rooms.h"
#include "roster.h"
#include "shares.h"
#include "updater.h"
#include "window.h"

QString QDeclarativeFileDialog::directory() const
{
    return QFileDialog::directory().path();
}

void QDeclarativeFileDialog::setDirectory(const QString &directory)
{
    QFileDialog::setDirectory(QDir(directory));
}

QDeclarativeSortFilterProxyModel::QDeclarativeSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void QDeclarativeSortFilterProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (model != sourceModel()) {
        QSortFilterProxyModel::setSourceModel(model);
        emit sourceModelChanged(sourceModel());
    }
}

void QDeclarativeSortFilterProxyModel::sort(int column)
{
    QSortFilterProxyModel::sort(column);
}

ListHelper::ListHelper(QObject *parent)
    : QObject(parent),
    m_model(0)
{
}

int ListHelper::count() const
{
    if (m_model)
        return m_model->rowCount();
    return 0;
}

QVariant ListHelper::get(int row) const
{
    QVariantMap result;
    if (m_model) {
        QModelIndex index = m_model->index(row, 0);
        const QHash<int, QByteArray> roleNames = m_model->roleNames();
        foreach (int role, roleNames.keys())
            result.insert(QString::fromAscii(roleNames[role]), index.data(role));
    }
    return result;
}

QVariant ListHelper::getProperty(int row, const QString &name) const
{
    if (m_model) {
        QModelIndex index = m_model->index(row, 0);
        const int role = m_model->roleNames().key(name.toAscii());
        return index.data(role);
    }
    return QVariant();
}

QAbstractItemModel *ListHelper::model() const
{
    return m_model;
}

void ListHelper::setModel(QAbstractItemModel *model)
{
    if (model != m_model) {
        m_model = model;
        emit modelChanged(m_model);
    }
}

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
    bool check;

    QNetworkDiskCache *cache = new QNetworkDiskCache(this);
    cache->setCacheDirectory(wApp->cacheDirectory());
    setCache(cache);

    check = QObject::connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                             QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    Q_ASSERT(check);
    Q_UNUSED(check);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request(req);
    request.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + "/" + qApp->applicationVersion()).toAscii());
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

DeclarativeWallet::DeclarativeWallet(QObject *parent)
    : QObject(parent)
{
}

QString DeclarativeWallet::get(const QString &jid) const
{
    const QString key = realm(jid);

    if (!key.isEmpty()) {
        QString tmpJid(jid);
        QString tmpPassword;

        if (QNetIO::Wallet::instance()->getCredentials(key, tmpJid, tmpPassword))
            return tmpPassword;
    }
    return QString();
}

/** Returns the authentication realm for the given JID.
 */
QString DeclarativeWallet::realm(const QString &jid)
{
    const QString domain = jidToDomain(jid);
    if (domain == QLatin1String("wifirst.net"))
        return QLatin1String("www.wifirst.net");
    else if (domain == QLatin1String("gmail.com"))
        return QLatin1String("www.google.com");
    return domain;
}

void DeclarativeWallet::remove(const QString &jid)
{
    const QString key = realm(jid);
    if (!key.isEmpty()) {
        qDebug("Removing password for %s (%s)", qPrintable(jid), qPrintable(key));
        QNetIO::Wallet::instance()->deleteCredentials(key);
    }
}

void DeclarativeWallet::set(const QString &jid, const QString &password)
{
    const QString key = realm(jid);
    if (!key.isEmpty()) {
        qDebug("Setting password for %s (%s)", qPrintable(jid), qPrintable(key));
        QNetIO::Wallet::instance()->setCredentials(key, jid, password);
    }
}

void Plugin::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    engine->addImageProvider("photo", new PhotoImageProvider);
    engine->addImageProvider("roster", new RosterImageProvider);
    engine->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory);
}

void Plugin::registerTypes(const char *uri)
{
    // QXmpp
    qmlRegisterUncreatableType<QXmppClient>("QXmpp", 0, 4, "QXmppClient", "");
    qmlRegisterUncreatableType<QXmppCall>("QXmpp", 0, 4, "QXmppCall", "");
    qmlRegisterUncreatableType<QXmppCallManager>("QXmpp", 0, 4, "QXmppCallManager", "");
    qmlRegisterType<QXmppDeclarativeDataForm>("QXmpp", 0, 4, "QXmppDataForm");
    qmlRegisterUncreatableType<DiagnosticManager>("QXmpp", 0, 4, "DiagnosticManager", "");
    qmlRegisterUncreatableType<QXmppDiscoveryManager>("QXmpp", 0, 4, "QXmppDiscoveryManager", "");
    qmlRegisterType<QXmppLogger>("QXmpp", 0, 4, "QXmppLogger");
    qmlRegisterType<QXmppDeclarativeMessage>("QXmpp", 0, 4, "QXmppMessage");
    qmlRegisterType<QXmppDeclarativeMucItem>("QXmpp", 0, 4, "QXmppMucItem");
    qmlRegisterUncreatableType<QXmppMucManager>("QXmpp", 0, 4, "QXmppMucManager", "");
    qmlRegisterUncreatableType<QXmppMucRoom>("QXmpp", 0, 4, "QXmppMucRoom", "");
    qmlRegisterType<QXmppDeclarativePresence>("QXmpp", 0, 4, "QXmppPresence");
    qmlRegisterUncreatableType<QXmppRosterManager>("QXmpp", 0, 4, "QXmppRosterManager", "");
    qmlRegisterUncreatableType<QXmppRtpAudioChannel>("QXmpp", 0, 4, "QXmppRtpAudioChannel", "");
    qmlRegisterUncreatableType<QXmppTransferJob>("QXmpp", 0, 4, "QXmppTransferJob", "");
    qmlRegisterUncreatableType<QXmppTransferManager>("QXmpp", 0, 4, "QXmppTransferManager", "");
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");

    // wiLink
    qmlRegisterType<CallAudioHelper>(uri, 1, 2, "CallAudioHelper");
    qmlRegisterType<CallVideoHelper>(uri, 1, 2, "CallVideoHelper");
    qmlRegisterType<CallVideoItem>(uri, 1, 2, "CallVideoItem");
    qmlRegisterUncreatableType<DeclarativePen>(uri, 1, 2, "Pen", "");
    qmlRegisterType<ChatClient>(uri, 1, 2, "Client");
    qmlRegisterType<Conversation>(uri, 1, 2, "Conversation");
    qmlRegisterType<DiscoveryModel>(uri, 1, 2, "DiscoveryModel");
    qmlRegisterUncreatableType<HistoryModel>(uri, 1, 2, "HistoryModel", "");
    qmlRegisterType<Idle>(uri, 1, 2, "Idle");
    qmlRegisterType<ListHelper>(uri, 1, 2, "ListHelper");
    qmlRegisterType<LogModel>(uri, 1, 2, "LogModel");
    qmlRegisterType<PhoneCallsModel>(uri, 1, 2, "PhoneCallsModel");
    qmlRegisterUncreatableType<SipClient>(uri, 1, 2, "SipClient", "");
    qmlRegisterUncreatableType<SipCall>(uri, 1, 2, "SipCall", "");
    qmlRegisterUncreatableType<PhotoUploadModel>(uri, 1, 2, "PhotoUploadModel", "");
    qmlRegisterType<PhotoModel>(uri, 1, 2, "PhotoModel");
    qmlRegisterType<PlayerModel>(uri, 1, 2, "PlayerModel");
    qmlRegisterType<RoomConfigurationModel>(uri, 1, 2, "RoomConfigurationModel");
    qmlRegisterType<RoomListModel>(uri, 1, 2, "RoomListModel");
    qmlRegisterType<RoomModel>(uri, 1, 2, "RoomModel");
    qmlRegisterType<RoomPermissionModel>(uri, 1, 2, "RoomPermissionModel");
    qmlRegisterType<RosterModel>(uri, 1, 2, "RosterModel");
    qmlRegisterType<ShareModel>(uri, 1, 2, "ShareModel");
    qmlRegisterType<ShareFolderModel>(uri, 1, 2, "ShareFolderModel");
    qmlRegisterType<SharePlaceModel>(uri, 1, 2, "SharePlaceModel");
    qmlRegisterType<ShareQueueModel>(uri, 1, 2, "ShareQueueModel");
    qmlRegisterUncreatableType<QSoundPlayer>(uri, 1, 2, "SoundPlayer", "");
    qmlRegisterType<QSoundTester>(uri, 1, 2, "SoundTester");
    qmlRegisterUncreatableType<Updater>(uri, 1, 2, "Updater", "");
    qmlRegisterType<VCard>(uri, 1, 2, "VCard");
    qmlRegisterType<DeclarativeWallet>(uri, 1, 2, "Wallet");
    qmlRegisterUncreatableType<Window>(uri, 1, 2, "Window", "");

    // crutches for Qt..
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qmlRegisterUncreatableType<QAbstractItemModel>(uri, 1, 2, "QAbstractItemModel", "");
    qmlRegisterUncreatableType<QFileDialog>(uri, 1, 2, "QFileDialog", "");
    qmlRegisterType<QDeclarativeSortFilterProxyModel>(uri, 1, 2, "SortFilterProxyModel");
}
