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

#include <QStringList>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QProcess>

#include "wireless.h"

#define FALLBACK_NOISE_FLOOR_DBM  (-90)
#define FALLBACK_SIGNAL_MAX_DBM   (-20)
#define FALLBACK_MAX_CINR (FALLBACK_SIGNAL_MAX_DBM - FALLBACK_NOISE_FLOOR_DBM)

class WirelessInterfacePrivate
{
public:
    WirelessInterfacePrivate() {};
    QString interfaceName;
    QString objectName;
    static int percentToRssi(int percent);
    static WirelessNetwork getFromDbusObject(QString path);
};

/* FIXME: unaccurate formula! */
int WirelessInterfacePrivate::percentToRssi(int percent)
{
   return FALLBACK_NOISE_FLOOR_DBM + percent * FALLBACK_MAX_CINR / 100;
}

WirelessNetwork WirelessInterfacePrivate::getFromDbusObject(QString path)
{
    QDBusInterface apInterface("org.freedesktop.NetworkManager", path,
        "org.freedesktop.NetworkManager.AccessPoint", QDBusConnection::systemBus());
    if(!apInterface.isValid())
        return WirelessNetwork();
    WirelessNetwork myNetwork;
    myNetwork.setSsid(apInterface.property("Ssid").toString());
    /* FIXME: unaccurate calculations! */
    int rssi = WirelessInterfacePrivate::percentToRssi(apInterface.property("Strength").toInt());
    myNetwork.setRssi(rssi);
    myNetwork.setCinr(rssi - FALLBACK_NOISE_FLOOR_DBM);
    return myNetwork;
}

WirelessInterface::WirelessInterface(const QNetworkInterface &iface)
{
    d = new WirelessInterfacePrivate();
    d->interfaceName = iface.name();
    QDBusInterface listInterface("org.freedesktop.NetworkManager", "/org/freedesktop/NetworkManager",
        "org.freedesktop.NetworkManager", QDBusConnection::systemBus());
    if (!listInterface.isValid())
        return;
    QDBusReply< QList<QDBusObjectPath> > reply = listInterface.call("GetDevices");
    foreach (const QDBusObjectPath &objectPath, reply.value())
    {
        QDBusInterface interface("org.freedesktop.NetworkManager", objectPath.path(),
            "org.freedesktop.NetworkManager.Device", QDBusConnection::systemBus());
        if (interface.property("Interface") == iface.name())
        {
            d->objectName = objectPath.path();
            break;
        }
    }
}

WirelessInterface::~WirelessInterface()
{
    delete d;
}

bool WirelessInterface::isValid() const
{
    QDBusInterface interface("org.freedesktop.NetworkManager", d->objectName,
        "org.freedesktop.NetworkManager.Device", QDBusConnection::systemBus());
    return ( interface.isValid() && interface.property("DeviceType").toInt() == 2 );
}

QList<WirelessNetwork> WirelessInterface::availableNetworks()
{
    QDBusInterface interface("org.freedesktop.NetworkManager", d->objectName,
        "org.freedesktop.NetworkManager.Device.Wireless", QDBusConnection::systemBus());
    QDBusReply< QList<QDBusObjectPath> > reply = interface.call("GetAccessPoints");
    if (! reply.isValid())
        return QList<WirelessNetwork>();

    QList<WirelessNetwork> result;
    foreach(QDBusObjectPath path, reply.value()) {
        WirelessNetwork myNetwork = WirelessInterfacePrivate::getFromDbusObject(path.path());
        if (myNetwork != WirelessNetwork())
            result.append(myNetwork);
    }
    return result;
}

WirelessNetwork WirelessInterface::currentNetwork()
{
    QDBusInterface interface("org.freedesktop.NetworkManager", d->objectName,
        "org.freedesktop.NetworkManager.Device.Wireless", QDBusConnection::systemBus());
    if (!interface.isValid())
        return WirelessNetwork();
    QDBusObjectPath activeAP = interface.property("ActiveAccessPoint").value<QDBusObjectPath>();
    return WirelessInterfacePrivate::getFromDbusObject(activeAP.path());
}

WirelessStandards WirelessInterface::supportedStandards()
{
    WirelessStandards standards;
    QProcess proc;
    proc.start("/sbin/iwconfig", QStringList() << d->interfaceName, QIODevice::ReadOnly);
    if (proc.waitForFinished())
    {
        QString output = QString::fromLocal8Bit(proc.readAll());
        QRegExp rx("IEEE 802.11([abgn]+)");
        if (rx.indexIn(output) != -1)
        {
            QString modes = rx.cap(1);
            if (modes.contains('a'))
                standards |= Wireless_80211A;
            if (modes.contains('b'))
                standards |= Wireless_80211B;
            if (modes.contains('g'))
                standards |= Wireless_80211G;
            if (modes.contains('n'))
                standards |= Wireless_80211N;
        }
    }
    return standards;
}

