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

#include <QDebug>

#ifdef USE_COREWLAN
#include <CoreWLAN/CoreWLAN.h>
#include <CoreWLAN/CWInterface.h>
#include <CoreWLAN/CWNetwork.h>
#endif

#include <Foundation/NSEnumerator.h>
#include <Foundation/NSKeyValueObserving.h>
#include <Foundation/NSAutoreleasePool.h>

#include <SystemConfiguration/SCNetworkConfiguration.h>

#include "wireless.h"

inline QString cfstringRefToQstring(CFStringRef cfStringRef) {
    QString retVal;
    CFIndex maxLength = 2 * CFStringGetLength(cfStringRef) + 1/*zero term*/; // max UTF8
    char *cstring = new char[maxLength];
    if (CFStringGetCString(CFStringRef(cfStringRef), cstring, maxLength, kCFStringEncodingUTF8)) {
        retVal = QString::fromUtf8(cstring);
    }
    delete cstring;
    return retVal;
}

inline CFStringRef qstringToCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(string.unicode()),
                                        string.length());
}

inline NSString *qstringToNSString(const QString &qstr)
{
    return [reinterpret_cast<const NSString *>(qstringToCFStringRef(qstr)) autorelease];
}

inline int nsnumberToInt(const NSNumber *nsnum)
{
    return nsnum ? [nsnum intValue] : 0;
}

inline QString nsstringToQString(const NSString *nsstr)
{
    return cfstringRefToQstring(reinterpret_cast<const CFStringRef>(nsstr));
}

class WirelessInterfacePrivate
{
public:
    QString interfaceName;
};

WirelessInterface::WirelessInterface(const QNetworkInterface &networkInterface)
    : d(NULL)
{
    d = new WirelessInterfacePrivate();
    d->interfaceName = networkInterface.name();
}

WirelessInterface::~WirelessInterface()
{
    delete d;
}

QList<WirelessNetwork> WirelessInterface::availableNetworks()
{
    QList<WirelessNetwork> results;
#ifdef USE_COREWLAN
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    CWInterface *currentInterface = [CWInterface interfaceWithName:qstringToNSString(d->interfaceName)];
    NSError *err = nil;
    NSDictionary *parametersDict = nil;
    NSArray* apArray = [NSMutableArray arrayWithArray:[currentInterface scanForNetworksWithParameters:parametersDict error:&err]];

    if(!err) {
        for(uint row=0; row < [apArray count]; row++ ) {
            CWNetwork *apNetwork = [apArray objectAtIndex:row];

            WirelessNetwork info;
            info.setCinr(nsnumberToInt([apNetwork noise]));
            info.setRssi(nsnumberToInt([apNetwork rssi]));
            info.setSsid(nsstringToQString([apNetwork ssid]));
            results.append(info);
        }
    } else {
        qWarning() << "ERROR scanning for ssids" << nsstringToQString([err localizedDescription])
                <<nsstringToQString([err domain]);
    }
    [autoreleasepool release];
#endif
    return results;
}

WirelessNetwork WirelessInterface::currentNetwork()
{
    WirelessNetwork network;
#ifdef USE_COREWLAN
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    CWInterface *currentInterface = [CWInterface interfaceWithName:qstringToNSString(d->interfaceName)];
    network.setCinr(nsnumberToInt([currentInterface noise]));
    network.setRssi(nsnumberToInt([currentInterface rssi]));
    network.setSsid(nsstringToQString([currentInterface ssid]));

    [autoreleasepool release];
#endif
    return network;
}

bool WirelessInterface::isValid() const
{
    bool valid = false;
#ifdef USE_COREWLAN
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    CWInterface *defaultInterface = [CWInterface interfaceWithName: qstringToNSString(d->interfaceName)];
    if([defaultInterface power])
        valid = true;

    [autoreleasepool release];
#endif
    return valid;
}

