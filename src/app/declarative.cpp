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

#include <QCoreApplication>
#include <QDeclarativeItem>
#include <QDeclarativeEngine>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QProcess>
#include <QSslError>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "QXmppCallManager.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMucManager.h"
#include "QXmppRtpChannel.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "QSoundPlayer.h"
#include "QSoundTester.h"

#include "wallet.h"

#include "accounts.h"
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
#include "icons.h"
#include "news.h"
#include "phone.h"
#include "phone/sip.h"
#include "photos.h"
#include "places.h"
#include "player.h"
#include "rooms.h"
#include "roster.h"
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
    if (m_model) {
        QModelIndex index = m_model->index(row, 0);
        if (index.isValid()) {
            QVariantMap result;
            const QHash<int, QByteArray> roleNames = m_model->roleNames();
            foreach (int role, roleNames.keys())
                result.insert(QString::fromAscii(roleNames[role]), index.data(role));
            result.insert("index", row);
            return result;
        }
    }
    return QVariant();
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
    Q_UNUSED(check);

    QNetworkDiskCache *cache = new QNetworkDiskCache(this);
    cache->setCacheDirectory(wApp->cacheDirectory());
    setCache(cache);

    check = connect(this, SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),
                    QNetIO::Wallet::instance(), SLOT(onAuthenticationRequired(QNetworkReply*,QAuthenticator*)));
    Q_ASSERT(check);

    check = connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                    this, SLOT(onSslErrors(QNetworkReply*,QList<QSslError>)));
    Q_ASSERT(check);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request(req);
    request.setRawHeader("Accept-Language", QLocale::system().name().toAscii());
    request.setRawHeader("User-Agent", userAgent().toAscii());
    return QNetworkAccessManager::createRequest(op, request, outgoingData);
}

void NetworkAccessManager::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply);

    qWarning("SSL errors:");
    foreach (const QSslError &error, errors)
        qWarning("* %s", qPrintable(error.errorString()));

    // uncomment for debugging purposes only
    //reply->ignoreSslErrors();
}

QString NetworkAccessManager::userAgent()
{
    static QString globalUserAgent;

    if (globalUserAgent.isEmpty()) {
        QString osDetails;
#if defined(Q_OS_LINUX)
        osDetails = QLatin1String("X11; Linux");
        QProcess process;
        process.start(QString("uname"), QStringList(QString("-m")), QIODevice::ReadOnly);
        process.waitForFinished();
        const QString arch = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        if (!arch.isEmpty())
            osDetails += " " + arch;
#elif defined(Q_OS_MAC)
        osDetails = QLatin1String("Macintosh");
        switch (QSysInfo::MacintoshVersion)
        {
        case QSysInfo::MV_10_4:
            osDetails += QLatin1String("; Mac OS X 10.4");
            break;
        case QSysInfo::MV_10_5:
            osDetails += QLatin1String("; Mac OS X 10.5");
            break;
        case QSysInfo::MV_10_6:
            osDetails += QLatin1String("; Mac OS X 10.6");
            break;
        }
#elif defined(Q_OS_WIN)
        DWORD dwVersion = GetVersion();
        DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
        DWORD dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));
        osDetails = QString("Windows NT %1.%2").arg(QString::number(dwMajorVersion), QString::number(dwMinorVersion));
#endif
        globalUserAgent = QString("Mozilla/5.0 (%1) %2/%3").arg(osDetails, qApp->applicationName(), qApp->applicationVersion());
    }
    return globalUserAgent;
}

static QStringList droppedFiles(QGraphicsSceneDragDropEvent *event)
{
    QStringList files;
    if (event->mimeData()->hasUrls()) {
        foreach (const QUrl &url, event->mimeData()->urls()) {
            if (url.scheme() == "file") {
                QFileInfo info(url.toLocalFile());
                if (info.exists() && !info.isDir())
                    files << info.filePath();
            }
        }
    }
    return files;
}

DropArea::DropArea(QDeclarativeItem *parent)
    : QDeclarativeItem(parent)
{
    setAcceptDrops(true);
}

void DropArea::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    event->setAccepted(!droppedFiles(event).isEmpty());
}

void DropArea::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QStringList files = droppedFiles(event);
    if (!files.isEmpty()) {
        emit filesDropped(files);
        event->accept();
    } else {
        event->ignore();
    }
}

WheelArea::WheelArea(QDeclarativeItem *parent)
    : QDeclarativeItem(parent)
{
}

void WheelArea::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    switch(event->orientation()) {
        case Qt::Horizontal:
            emit horizontalWheel(event->delta());
            break;
        case Qt::Vertical:
            emit verticalWheel(event->delta());
            break;
        default:
            event->ignore();
            break;
    }
}

void Plugin::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
    engine->addImageProvider("icon", new IconImageProvider);
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
    qmlRegisterType<AccountModel>(uri, 2, 0, "AccountModel");
    qmlRegisterUncreatableType<ApplicationSettings>(uri, 2, 0, "ApplicationSettings", "");
    qmlRegisterType<CallAudioHelper>(uri, 2, 0, "CallAudioHelper");
    qmlRegisterType<CallVideoHelper>(uri, 2, 0, "CallVideoHelper");
    qmlRegisterType<CallVideoItem>(uri, 2, 0, "CallVideoItem");
    qmlRegisterUncreatableType<DeclarativePen>(uri, 2, 0, "Pen", "");
    qmlRegisterType<ChatClient>(uri, 2, 0, "Client");
    qmlRegisterType<Conversation>(uri, 2, 0, "Conversation");
    qmlRegisterType<DiscoveryModel>(uri, 2, 0, "DiscoveryModel");
    qmlRegisterType<DropArea>(uri, 2, 0, "DropArea");
    qmlRegisterType<FolderModel>(uri, 2, 0, "FolderModel");
    qmlRegisterUncreatableType<HistoryModel>(uri, 2, 0, "HistoryModel", "");
    qmlRegisterType<Idle>(uri, 2, 0, "Idle");
    qmlRegisterType<ListHelper>(uri, 2, 0, "ListHelper");
    qmlRegisterType<LogModel>(uri, 2, 0, "LogModel");
    qmlRegisterType<NewsListModel>(uri, 2, 0, "NewsListModel");
    qmlRegisterUncreatableType<Notification>(uri, 2, 0, "Notification", "");
    qmlRegisterType<PhoneContactModel>(uri, 2, 0, "PhoneContactModel");
    qmlRegisterType<PhoneHistoryModel>(uri, 2, 0, "PhoneHistoryModel");
    qmlRegisterUncreatableType<SipClient>(uri, 2, 0, "SipClient", "");
    qmlRegisterUncreatableType<SipCall>(uri, 2, 0, "SipCall", "");
    qmlRegisterUncreatableType<FolderQueueModel>(uri, 2, 0, "FolderQueueModel", "");
    qmlRegisterType<PlaceModel>(uri, 2, 0, "PlaceModel");
    qmlRegisterType<PlayerModel>(uri, 2, 0, "PlayerModel");
    qmlRegisterType<RoomConfigurationModel>(uri, 2, 0, "RoomConfigurationModel");
    qmlRegisterType<RoomListModel>(uri, 2, 0, "RoomListModel");
    qmlRegisterType<RoomModel>(uri, 2, 0, "RoomModel");
    qmlRegisterType<RoomPermissionModel>(uri, 2, 0, "RoomPermissionModel");
    qmlRegisterType<RosterModel>(uri, 2, 0, "RosterModel");
    qmlRegisterUncreatableType<QSoundPlayer>(uri, 2, 0, "SoundPlayer", "");
    qmlRegisterUncreatableType<QSoundPlayerJob>(uri, 2, 0, "SoundPlayerJob", "");
    qmlRegisterType<QSoundTester>(uri, 2, 0, "SoundTester");
    qmlRegisterType<Updater>(uri, 2, 0, "Updater");
    qmlRegisterType<VCard>(uri, 2, 0, "VCard");
    qmlRegisterType<WheelArea>(uri, 2, 0, "WheelArea");
    qmlRegisterType<Window>(uri, 2, 0, "Window");

    // crutches for Qt..
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qmlRegisterUncreatableType<QAbstractItemModel>(uri, 2, 0, "QAbstractItemModel", "");
    qmlRegisterUncreatableType<QFileDialog>(uri, 2, 0, "QFileDialog", "");
    qmlRegisterType<QDeclarativeSortFilterProxyModel>(uri, 2, 0, "SortFilterProxyModel");
}
