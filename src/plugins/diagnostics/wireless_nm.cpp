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
#include <QDebug>

#include "wireless.h"

#define FALLBACK_NOISE_FLOOR_DBM  (-90)
#define FALLBACK_SIGNAL_MAX_DBM   (-20)
#define FALLBACK_MAX_CINR (FALLBACK_SIGNAL_MAX_DBM - FALLBACK_NOISE_FLOOR_DBM)

/* FIXME: this is only for the debug phase (listing properties can be useful) */
#include <QMetaProperty>
void printProps(QObject &obj)
{
    const QMetaObject* metaObject = obj.metaObject();
    QStringList properties;
    for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i)
        properties << QString::fromLatin1(metaObject->property(i).name());
    qDebug() << properties;
}

class WirelessInterfacePrivate
{
public:
    WirelessInterfacePrivate() {};
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

WirelessInterface::WirelessInterface(const QNetworkInterface &name)
{
    d = new WirelessInterfacePrivate();
    d->objectName = "/org/freedesktop/Hal/devices/net_" + name.hardwareAddress().toLower().replace(':','_');
}

WirelessInterface::~WirelessInterface()
{
    delete d;
}

bool WirelessInterface::isValid() const
{
    QDBusInterface interface("org.freedesktop.NetworkManager", d->objectName,
        "org.freedesktop.NetworkManager.Device", QDBusConnection::systemBus());
    return ( interface.isValid() && interface.property("DeviceType").toInt()==2 );
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
    if (! interface.isValid()) {
        qDebug() << d->objectName << "error line" << __LINE__;
        return WirelessNetwork();
    }
    QDBusObjectPath activeAP = interface.property("ActiveAccessPoint").value<QDBusObjectPath>();
    return WirelessInterfacePrivate::getFromDbusObject(activeAP.path());
}

