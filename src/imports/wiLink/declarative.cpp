/*
 * wiLink
 * Copyright (C) 2009-2013 Wifirst
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
#include <QDeclarativeEngine>
#include <QDesktopServices>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>

#include "QXmppArchiveManager.h"
#include "QXmppCallManager.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMucManager.h"
#include "QXmppRtpChannel.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "QSoundLoader.h"
#include "QSoundPlayer.h"
#include "QSoundTester.h"

#include "wallet.h"

#include "accounts.h"
#include "idle/idle.h"
#include "calls.h"
#include "client.h"
#include "console.h"
#include "conversations.h"
#include "declarative.h"
#include "declarative_qxmpp.h"
#include "diagnostics.h"
#include "discovery.h"
#include "history.h"
#include "icons.h"
#include "menubar.h"
#include "notifications.h"
#include "phone.h"
#include "phone/sip.h"
#include "places.h"
#include "rooms.h"
#include "roster.h"
#include "settings.h"
#include "translations.h"
#include "updater.h"

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
                result.insert(QString::fromLatin1(roleNames[role]), index.data(role));
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
        const int role = m_model->roleNames().key(name.toLatin1());
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
    engine->addImageProvider("roster", new RosterImageProvider);

    // initialise wallet
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    const QString dataPath = QStandardPaths::standardLocations(QStandardPaths::DataLocation)[0];
#else
    const QString dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#endif
    QDir().mkpath(dataPath);
    QNetIO::Wallet::setDataPath(QDir(dataPath).filePath("wallet"));
}

void Plugin::registerTypes(const char *uri)
{
    // wiLink
    qmlRegisterType<AccountModel>(uri, 2, 4, "AccountModel");
    qmlRegisterType<ApplicationSettings>(uri, 2, 4, "ApplicationSettings");
    qmlRegisterType<CallAudioHelper>(uri, 2, 4, "CallAudioHelper");
    qmlRegisterType<CallVideoHelper>(uri, 2, 4, "CallVideoHelper");
    qmlRegisterType<CallVideoItem>(uri, 2, 4, "CallVideoItem");
    qmlRegisterUncreatableType<DeclarativePen>(uri, 2, 4, "Pen", "");
    qmlRegisterType<ChatClient>(uri, 2, 4, "Client");
    qmlRegisterType<Conversation>(uri, 2, 4, "Conversation");
    qmlRegisterUncreatableType<DiagnosticManager>(uri, 2, 4, "DiagnosticManager", "");
    qmlRegisterType<DiscoveryModel>(uri, 2, 4, "DiscoveryModel");
    qmlRegisterType<DropArea>(uri, 2, 4, "DropArea");
    qmlRegisterUncreatableType<HistoryModel>(uri, 2, 4, "HistoryModel", "");
    qmlRegisterType<Idle>(uri, 2, 4, "Idle");
    qmlRegisterType<ListHelper>(uri, 2, 4, "ListHelper");
    qmlRegisterType<LogModel>(uri, 2, 4, "LogModel");
    qmlRegisterType<MenuBar>(uri, 2, 4, "MenuBar");
    qmlRegisterType<Notifier>(uri, 2, 4, "Notifier");
    qmlRegisterUncreatableType<Notification>(uri, 2, 4, "Notification", "");
    qmlRegisterType<PhoneAudioHelper>(uri, 2, 4, "PhoneAudioHelper");
    qmlRegisterType<SipClient>(uri, 2, 4, "SipClient");
    qmlRegisterUncreatableType<SipCall>(uri, 2, 4, "SipCall", "");
    qmlRegisterType<PlaceModel>(uri, 2, 4, "PlaceModel");
    qmlRegisterType<RoomConfigurationModel>(uri, 2, 4, "RoomConfigurationModel");
    qmlRegisterType<RoomModel>(uri, 2, 4, "RoomModel");
    qmlRegisterType<RoomPermissionModel>(uri, 2, 4, "RoomPermissionModel");
    qmlRegisterType<RosterModel>(uri, 2, 4, "RosterModel");
    qmlRegisterType<QSoundLoader>(uri, 2, 4, "SoundLoader");
    qmlRegisterType<QSoundPlayer>(uri, 2, 4, "SoundPlayer");
    qmlRegisterType<QSoundTester>(uri, 2, 4, "SoundTester");
    qmlRegisterType<TranslationLoader>(uri, 2, 4, "TranslationLoader");
    qmlRegisterType<Updater>(uri, 2, 4, "Updater");
    qmlRegisterType<VCard>(uri, 2, 4, "VCard");
    qmlRegisterType<WheelArea>(uri, 2, 4, "WheelArea");

    // QXmpp
    qmlRegisterUncreatableType<QXmppArchiveManager>(uri, 2, 4, "QXmppArchiveManager", "");
    qmlRegisterUncreatableType<QXmppClient>(uri, 2, 4, "QXmppClient", "");
    qmlRegisterUncreatableType<QXmppCall>(uri, 2, 4, "QXmppCall", "");
    qmlRegisterUncreatableType<QXmppCallManager>(uri, 2, 4, "QXmppCallManager", "");
    qmlRegisterType<QXmppDeclarativeDataForm>(uri, 2, 4, "QXmppDataForm");
    qmlRegisterUncreatableType<QXmppDiscoveryManager>(uri, 2, 4, "QXmppDiscoveryManager", "");
    qmlRegisterType<QXmppLogger>(uri, 2, 4, "QXmppLogger");
    qmlRegisterType<QXmppDeclarativeMessage>(uri, 2, 4, "QXmppMessage");
    qmlRegisterType<QXmppDeclarativeMucItem>(uri, 2, 4, "QXmppMucItem");
    qmlRegisterUncreatableType<QXmppMucManager>(uri, 2, 4, "QXmppMucManager", "");
    qmlRegisterUncreatableType<QXmppMucRoom>(uri, 2, 4, "QXmppMucRoom", "");
    qmlRegisterUncreatableType<QXmppRosterManager>(uri, 2, 4, "QXmppRosterManager", "");
    qmlRegisterUncreatableType<QXmppRtpAudioChannel>(uri, 2, 4, "QXmppRtpAudioChannel", "");
    qmlRegisterUncreatableType<QXmppTransferJob>(uri, 2, 4, "QXmppTransferJob", "");
    qmlRegisterUncreatableType<QXmppTransferManager>(uri, 2, 4, "QXmppTransferManager", "");
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");

    // crutches for Qt..
    qRegisterMetaType<QIODevice::OpenMode>("QIODevice::OpenMode");
    qmlRegisterUncreatableType<QAbstractItemModel>(uri, 2, 4, "QAbstractItemModel", "");
    qmlRegisterUncreatableType<QFileDialog>(uri, 2, 4, "QFileDialog", "");
    qmlRegisterType<QDeclarativeSortFilterProxyModel>(uri, 2, 4, "SortFilterProxyModel");
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(qmlwilinkplugin, Plugin);
#endif
