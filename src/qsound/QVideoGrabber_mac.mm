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
#include <QTKit/QTCaptureDeviceInput.h>
#include <QTKit/QTCaptureSession.h>
#include <QTKit/QTCaptureDecompressedVideoOutput.h>
#include <QTKit/QTFormatDescription.h>
#include <QTKit/QTMedia.h>

#include <QString>

#include "QVideoGrabber.h"
#include "QVideoGrabber_p.h"

@interface QVideoGrabberDelegate : NSObject {
@public
    int frameHeight;
    int frameWidth;
    QXmppVideoFrame::PixelFormat pixelFormat;
    QVideoGrabber *q;

@private
    QList<CVImageBufferRef> buffers;
    CVImageBufferRef currentFrame;
}

-(void) captureOutput:(QTCaptureOutput *)captureOutput
                    didOutputVideoFrame:(CVImageBufferRef)videoFrame
                    withSampleBuffer:(QTSampleBuffer *)sampleBuffer
                    fromConnection:(QTCaptureConnection *)connection;

-(QXmppVideoFrame) readFrame;

@end

@implementation QVideoGrabberDelegate

- (void) captureOutput:(QTCaptureOutput *)captureOutput
                    didOutputVideoFrame:(CVImageBufferRef)videoFrame
                    withSampleBuffer:(QTSampleBuffer *)sampleBuffer
                    fromConnection:(QTCaptureConnection *)connection
{
    CVBufferRetain(videoFrame);

    @synchronized(self) {
        buffers << videoFrame;
    }
    
    QMetaObject::invokeMethod(q, "readyRead");
}

-(QXmppVideoFrame) readFrame
{
    CVImageBufferRef buffer;
    @synchronized(self) {
        if (buffers.isEmpty())
            return QXmppVideoFrame();
        buffer = buffers.takeFirst();
    }
    if (CVPixelBufferLockBaseAddress(buffer, 0) != kCVReturnSuccess) {
        CVBufferRelease(buffer);
        return QXmppVideoFrame();
    }

    const int length = CVPixelBufferGetDataSize(buffer);
    QXmppVideoFrame frame(length, QSize(frameWidth, frameHeight), CVPixelBufferGetBytesPerRow(buffer), pixelFormat);
    memcpy(frame.bits(), CVPixelBufferGetBaseAddress(buffer), length);
    CVPixelBufferUnlockBaseAddress(buffer, 0);
    CVBufferRelease(buffer);
    return frame;
}

@end

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

inline QString nsstringToQString(const NSString *nsstr)
{
    return cfstringRefToQstring(reinterpret_cast<const CFStringRef>(nsstr));
}

class QVideoGrabberPrivate
{
public:
    QVideoGrabberPrivate(QVideoGrabber *qq);
    bool open();
    void close();

    QVideoGrabberDelegate *delegate;
    QTCaptureDevice *device;
    QTCaptureDeviceInput *deviceInput;
    QTCaptureDecompressedVideoOutput *deviceOutput;
    NSAutoreleasePool *pool;
    QTCaptureSession *session;

private:
    QVideoGrabber *q;
};

QVideoGrabberPrivate::QVideoGrabberPrivate(QVideoGrabber *qq)
    : q(qq),
    delegate(0),
    device(0),
    deviceInput(0),
    deviceOutput(0),
    pool(0),
    session(0)
{
}

void QVideoGrabberPrivate::close()
{
    if (session) {
        [session release];
        session = 0;
    }

    if (device) {
        if ([device isOpen])
            [device close];
        [device release];
        device = 0;
    }
}

bool QVideoGrabberPrivate::open()
{
    // create device
    device = [QTCaptureDevice defaultInputDeviceWithMediaType:QTMediaTypeVideo];
    if (!device)
        return false;

    // open device
    NSError *error;
    if (![device open:&error]) {
        NSLog(@"%@\n", error);
        close();
        return false;
    }

    session = [[QTCaptureSession alloc] init];

    // add capture input
    deviceInput = [[QTCaptureDeviceInput alloc] initWithDevice:device];
    if (![session addInput:deviceInput error:&error]) {
        NSLog(@"%@\n", error);
        close();
        return false;
    }

    // add capture output
    deviceOutput = [[QTCaptureDecompressedVideoOutput alloc] init];
    if (![session addOutput:deviceOutput error:&error]) {
        NSLog(@"%@\n", error);
        close();
        return false;
    }
    [deviceOutput setPixelBufferAttributes:[NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithUnsignedInt:kYUVSPixelFormat], (id)kCVPixelBufferPixelFormatTypeKey,
        [NSNumber numberWithUnsignedInt:320], (id)kCVPixelBufferWidthKey,
        [NSNumber numberWithUnsignedInt:240], (id)kCVPixelBufferHeightKey,
        nil]];
    [deviceOutput setDelegate:delegate];

    // store config
    NSDictionary* pixAttr = [deviceOutput pixelBufferAttributes];
    delegate->frameWidth = [[pixAttr valueForKey:(id)kCVPixelBufferWidthKey] floatValue];
    delegate->frameHeight = [[pixAttr valueForKey:(id)kCVPixelBufferHeightKey] floatValue];
    delegate->pixelFormat = QXmppVideoFrame::Format_YUYV;
    return true;
}

QVideoGrabber::QVideoGrabber()
{
    d = new QVideoGrabberPrivate(this);
    d->pool = [[NSAutoreleasePool alloc] init];
    d->delegate = [[QVideoGrabberDelegate alloc] init];
    d->delegate->q = this;
    d->delegate->frameWidth = 0;
    d->delegate->frameHeight = 0;
}

QVideoGrabber::~QVideoGrabber()
{
    // stop acquisition
    stop();

    // close device
    d->close();

    [d->pool release];
    delete d;
}

QXmppVideoFrame QVideoGrabber::currentFrame()
{
    return [d->delegate readFrame];
}

QXmppVideoFormat QVideoGrabber::format() const
{
    QXmppVideoFormat fmt;
    fmt.setFrameSize(QSize(d->delegate->frameWidth, d->delegate->frameHeight));
    fmt.setPixelFormat(d->delegate->pixelFormat);
    return fmt;
}

void QVideoGrabber::onFrameCaptured()
{
}

bool QVideoGrabber::start()
{
    if (!d->session && !d->open())
        return false;

    [d->session startRunning];
    return true;
}

QVideoGrabber::State QVideoGrabber::state() const
{
    return StoppedState;
}

void QVideoGrabber::stop()
{
    if (!d->session)
        return;
     [d->session stopRunning];
}

QList<QVideoGrabberInfo> QVideoGrabberInfo::availableGrabbers()
{
    QList<QVideoGrabberInfo> grabbers;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSArray* devices = [QTCaptureDevice inputDevicesWithMediaType:QTMediaTypeVideo];
    for (QTCaptureDevice *device in devices) {
        QVideoGrabberInfo grabber;
        grabber.d->deviceName = nsstringToQString([device uniqueID]);
        grabber.d->supportedPixelFormats << QXmppVideoFrame::Format_YUYV;

        //for (QTFormatDescription* fmtDesc in [device formatDescriptions])
        //    NSLog(@"Format Description - %@", [fmtDesc formatDescriptionAttributes]);

        grabbers << grabber;
    }

    [pool release];
    return grabbers;
}

