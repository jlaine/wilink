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

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "QXmppCallManager.h"
#include "QXmppClient.h"
#include "QXmppDiscoveryManager.h"
#include "QXmppMucManager.h"
#include "QXmppRtpChannel.h"
#include "QXmppRosterManager.h"
#include "QXmppTransferManager.h"
#include "QXmppUtils.h"

#include "plugin.h"

void QXmppPlugin::registerTypes(const char *uri)
{
    qDebug("register types");

    // QXmpp
    qmlRegisterUncreatableType<QXmppClient>(uri, 0, 4, "QXmppClient", "");
    qmlRegisterUncreatableType<QXmppCall>(uri, 0, 4, "QXmppCall", "");
    qmlRegisterUncreatableType<QXmppCallManager>(uri, 0, 4, "QXmppCallManager", "");
    qmlRegisterType<QXmppDeclarativeDataForm>(uri, 0, 4, "QXmppDataForm");
    qmlRegisterUncreatableType<QXmppDiscoveryManager>(uri, 0, 4, "QXmppDiscoveryManager", "");
    qmlRegisterType<QXmppLogger>(uri, 0, 4, "QXmppLogger");
    qmlRegisterType<QXmppDeclarativeMessage>(uri, 0, 4, "QXmppMessage");
    qmlRegisterType<QXmppDeclarativeMucItem>(uri, 0, 4, "QXmppMucItem");
    qmlRegisterUncreatableType<QXmppMucManager>(uri, 0, 4, "QXmppMucManager", "");
    qmlRegisterUncreatableType<QXmppMucRoom>(uri, 0, 4, "QXmppMucRoom", "");
    qmlRegisterType<QXmppDeclarativePresence>(uri, 0, 4, "QXmppPresence");
    qmlRegisterUncreatableType<QXmppRosterManager>(uri, 0, 4, "QXmppRosterManager", "");
    qmlRegisterUncreatableType<QXmppRtpAudioChannel>(uri, 0, 4, "QXmppRtpAudioChannel", "");
    qmlRegisterUncreatableType<QXmppTransferJob>(uri, 0, 4, "QXmppTransferJob", "");
    qmlRegisterUncreatableType<QXmppTransferManager>(uri, 0, 4, "QXmppTransferManager", "");
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");
}

Q_EXPORT_PLUGIN2(qmlqxmpplugin, QXmppPlugin);
