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

#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>
#include <Growl/Growl.h>

#include "notifications_growl.h"

static inline CFStringRef qstringToCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(string.unicode()),
                                        string.length());
}

@interface GrowlController : NSObject <GrowlApplicationBridgeDelegate>
- (void) growlNotificationWasClicked: (CFDataRef)clickContext;
- (NSDictionary *) registrationDictionaryForGrowl;
@end

@implementation GrowlController

/** Catch clicks on notifications.
 */
- (void) growlNotificationWasClicked:(CFDataRef)data
{
    Notification *handle = 0;
    CFDataGetBytes(data, CFRangeMake(0,CFDataGetLength(data)), (UInt8*) &handle);
    QMetaObject::invokeMethod(handle, "clicked");
}

/** Supply registration information.
 */
- (NSDictionary *) registrationDictionaryForGrowl
{
        NSArray *notifications = [NSArray arrayWithObject: @"Notification"];
        return [NSDictionary dictionaryWithObjectsAndKeys:
                        notifications, GROWL_NOTIFICATIONS_ALL,
                        notifications, GROWL_NOTIFICATIONS_DEFAULT, nil];
}

@end

NotifierBackendGrowl::NotifierBackendGrowl(Notifier *qq)
    : q(qq)
{
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    GrowlController *growlDelegate = [[GrowlController alloc] init];
    [GrowlApplicationBridge setGrowlDelegate:growlDelegate];
    [autoreleasepool release];
}

void NotifierBackendGrowl::alert()
{
    NSApplicationLoad();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
    [pool release];
}

Notification *NotifierBackendGrowl::showMessage(const QString &title, const QString &message, const QString &action)
{
    Notification *handle = new Notification(q);
    CFDataRef context = CFDataCreate(kCFAllocatorDefault, (UInt8*)&handle, sizeof(&handle));
    [GrowlApplicationBridge notifyWithTitle:(NSString*)qstringToCFStringRef(title)
        description:(NSString*)qstringToCFStringRef(message)
        notificationName:@"Notification"
        iconData:nil
        priority:0
        isSticky:NO
        clickContext:(NSData*)context];
    return handle;
}
