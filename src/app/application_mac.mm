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

#include <QApplication>
#include "window.h"

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

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        CustomWindow *window = qobject_cast<CustomWindow*>(widget);
        if (window)
            window->showAndRaise();
    }

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

void mac_init()
{
    NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
    NSApplication *cocoaApp = [NSApplication sharedApplication];
    AppController *appDelegate = [[AppController alloc] init];
    [cocoaApp setDelegate:appDelegate];
    [autoreleasepool release];
}

