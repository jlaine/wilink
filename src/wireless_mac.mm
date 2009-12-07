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

WirelessInterface::WirelessInterface(const QString &name)
    : interfaceName(name)
{
}

bool WirelessInterface::isValid() const
{
    return true;
}

QList<WirelessNetwork> WirelessInterface::networks()
{
    QList<WirelessNetwork> results;
#ifdef USE_COREWLAN
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];

    CWInterface *currentInterface = [CWInterface interfaceWithName:qstringToNSString(interfaceName)];
    NSError *err = nil;
    NSDictionary *parametersDict = nil;
    NSArray* apArray = [NSMutableArray arrayWithArray:[currentInterface scanForNetworksWithParameters:parametersDict error:&err]];

    if(!err) {
        for(uint row=0; row < [apArray count]; row++ ) {
            CWNetwork *apNetwork = [apArray objectAtIndex:row];
            NSNumber *num;

            WirelessNetwork info;
            info.setSsid(nsstringToQString([apNetwork ssid]));
            info.setRssi(nsnumberToInt([apNetwork rssi]));
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

