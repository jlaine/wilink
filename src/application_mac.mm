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

#include <AppKit/AppKit.h>
#include <Foundation/NSAutoreleasePool.h>

#include <QTimer>

#include "application.h"

#if (MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5)
@interface AppController : NSObject
#else
@interface AppController : NSObject <NSApplicationDelegate>
#endif
- (BOOL)applicationShouldHandleReopen: (NSApplication *)app hasVisibleWindows: (BOOL) flag;
- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender;
@end

@implementation AppController

/** Catch clicks on the dock icon and show chat windows.
 */
- (BOOL)applicationShouldHandleReopen: (NSApplication *)app hasVisibleWindows: (BOOL) flag
{
    if (qApp)
        QTimer::singleShot(0, qApp, SLOT(showChats()));
    return NO;
}

/** Make sure we don't prevent logout.
 */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    return NSTerminateNow;
}

@end

void Application::alert(QWidget *widget)
{
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

