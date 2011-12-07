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

#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>

#include "application.h"

extern bool qt_mac_execute_apple_script(const QString &script, AEDesc *ret);

@interface AppController : NSObject <NSApplicationDelegate>
- (BOOL)applicationShouldHandleReopen: (NSApplication *)app hasVisibleWindows: (BOOL) flag;
- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender;
@end

@implementation AppController

/** Catch clicks on the dock icon and show chat windows.
 */
- (BOOL)applicationShouldHandleReopen: (NSApplication *)app hasVisibleWindows: (BOOL) flag
{
    Q_UNUSED(app);
    Q_UNUSED(flag);

    wApp->showWindows();
    return NO;
}

/** Make sure we don't prevent logout.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    Q_UNUSED(sender);

    return NSTerminateNow;
}

@end

void Application::alert(QWidget *widget)
{
    Q_UNUSED(widget);

    NSApplicationLoad();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
    [pool release];
}

void Application::platformInit()
{
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    NSApplication *cocoaApp = [NSApplication sharedApplication];
    AppController *delegate = [[AppController alloc] init];
    [cocoaApp setDelegate:delegate];
    [autoreleasepool release];
}

Notification *Application::showMessage(const QString &title, const QString &message, const QString &action)
{
    // Make sure that we have Growl installed on the machine we are running on.
    CFURLRef cfurl = 0;
    OSStatus status = LSGetApplicationForInfo(kLSUnknownType, kLSUnknownCreator,
                                              CFSTR("growlTicket"), kLSRolesAll, 0, &cfurl);
    if (status == kLSApplicationNotFoundErr)
        return 0;

    CFBundleRef bundle = CFBundleCreate(0, cfurl);
    if (CFStringCompare(CFBundleGetIdentifier(bundle), CFSTR("com.Growl.GrowlHelperApp"),
                kCFCompareCaseInsensitive |  kCFCompareBackwards) != kCFCompareEqualTo) {
        CFRelease(cfurl);
        return 0;
    }

    QString notificationType(QLatin1String("Notification"));
    const QString script(QLatin1String(
        "tell application \"GrowlHelperApp\"\n"
        "-- Make a list of all the notification types (all)\n"
        "set the allNotificationsList to {\"") + notificationType + QLatin1String("\"}\n"

        "-- Make a list of the notifications (enabled)\n"
        "set the enabledNotificationsList to {\"") + notificationType + QLatin1String("\"}\n"

        "-- Register our script with growl.\n"
        "register as application \"") + applicationName() + QLatin1String("\" all notifications allNotificationsList default notifications enabledNotificationsList\n"

        "--	Send a Notification...\n") +
        QLatin1String("notify with name \"") + notificationType +
        QLatin1String("\" title \"") + title +
        QLatin1String("\" description \"") + message +
        QLatin1String("\" application name \"") + applicationName() +
        QLatin1String("\"\nend tell"));
    qt_mac_execute_apple_script(script, 0);

    CFRelease(cfurl);
    CFRelease(bundle);
    return 0;
}
