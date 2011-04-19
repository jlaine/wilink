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

#include <Foundation/NSAutoreleasePool.h>
#include <QTKit/QTCaptureDevice.h>
#include <QTKit/QTMedia.h>

#include <QString>

#include "QVideoGrabber.h"
#include "QVideoGrabber_p.h"

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

class QVideoGrabberPrivate
{
public:
    QVideoGrabberPrivate(QVideoGrabber *qq);

private:
    QVideoGrabber *q;
};

QVideoGrabberPrivate::QVideoGrabberPrivate(QVideoGrabber *qq)
    : q(qq)
{
}

QVideoGrabber::QVideoGrabber()
{
    d = new QVideoGrabberPrivate(this);
}

QVideoGrabber::~QVideoGrabber()
{
    delete d;
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    return QXmppVideoFrame();
}

QXmppVideoFormat QVideoGrabber::format() const
{
    return QXmppVideoFormat();
}

bool QVideoGrabber::start()
{
    return false;
}

QVideoGrabber::State QVideoGrabber::state() const
{
    return StoppedState;
}

void QVideoGrabber::stop()
{
}

QList<QVideoGrabberInfo> QVideoGrabberInfo::availableGrabbers()
{
    QList<QVideoGrabberInfo> grabbers;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSArray* devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    for (QTCaptureDevice *device in devices) {
        QVideoGrabberInfo grabber;
        grabber.d->deviceName = nsstringToQString([device uniqueID]);
        grabbers << grabber;
    }

    [pool release];
    return grabbers;
}

