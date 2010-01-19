#include <AppKit/AppKit.h>
//#include <CoreServices/CoreServices.h>
#include <Foundation/NSAutoreleasePool.h>

void application_alert_mac()
{
    NSApplicationLoad();
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[NSApplication sharedApplication] requestUserAttention:NSCriticalRequest];
    [pool release];
}

