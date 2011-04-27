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

#include <signal.h>

#include <QApplication>
#include <QMetaType>

#include "QVideoGrabber.h"
#include "grabber.h"

#define MAX_FRAMES 10

Grabber::Grabber()
    : m_count(0),
    m_input(0)
{
    qRegisterMetaType<QXmppVideoFrame>("QXmppVideoFrame");
}

void Grabber::saveFrame(const QXmppVideoFrame &frame)
{
    if (m_count >= MAX_FRAMES)
        return;

    if (!frame.isValid() || frame.size() != m_image.size()) {
        qWarning("Invalid frame");
        return;
    }
    qDebug("Grabbed frame %i x %i", frame.width(), frame.height());
    QVideoGrabber::frameToImage(&frame, &m_image);
    m_image.save(QString("foo_%1.png").arg(QString::number(m_count++)));
    if (m_count >= MAX_FRAMES) {
        m_input->stop();
        emit finished();
    }
}

void Grabber::start(QVideoGrabber *input)
{
    m_input = input;
    connect(m_input, SIGNAL(frameAvailable(QXmppVideoFrame)),
            this, SLOT(saveFrame(QXmppVideoFrame)));

    if (!m_input->start()) {
        qWarning("Could not start capture");
        emit finished();
        return;
    }

    const QXmppVideoFormat format = m_input->format();
    qDebug("Started capture %i x %i", format.frameSize().width(), format.frameSize().height());
    m_image = QImage(format.frameSize(), QImage::Format_RGB32);
    m_image.fill(0);
}

static int aborted = 0;
static void signal_handler(int sig)
{
    if (aborted)
        exit(1);

    qApp->quit();
    aborted = 1;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    /* Install signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // get default device
    QList<QVideoGrabberInfo> grabbers = QVideoGrabberInfo::availableGrabbers();
    if (grabbers.isEmpty()) {
        qWarning("No grabbers found");
        return 1;
    }
    qDebug("Device %s", qPrintable(grabbers.first().deviceName()));

    // determine format
    QXmppVideoFormat format;
    format.setFrameRate(20.0);
    format.setFrameSize(QSize(320, 240));
    format.setPixelFormat(QXmppVideoFrame::Format_Invalid);
    QList<QXmppVideoFrame::PixelFormat> pixelFormats = grabbers.first().supportedPixelFormats();
    if (pixelFormats.contains(QXmppVideoFrame::Format_RGB32)) {
        qDebug(" - supports RGB32");
        format.setPixelFormat(QXmppVideoFrame::Format_RGB32);
    }
    if (pixelFormats.contains(QXmppVideoFrame::Format_RGB24)) {
        qDebug(" - supports RGB24");
        format.setPixelFormat(QXmppVideoFrame::Format_RGB24);
    }
    if (pixelFormats.contains(QXmppVideoFrame::Format_YUYV)) {
        qDebug(" - supports YUYV");
        format.setPixelFormat(QXmppVideoFrame::Format_YUYV);
    }
    if (pixelFormats.contains(QXmppVideoFrame::Format_YUV420P)) {
        qDebug(" - supports YUV420P");
        format.setPixelFormat(QXmppVideoFrame::Format_YUV420P);
    }
    if (!format.pixelFormat() == QXmppVideoFrame::Format_Invalid) {
        qWarning("No valid formats found");
    }

    // grab some frames
    QVideoGrabber input(format);
    Grabber grabber;
    QObject::connect(&grabber, SIGNAL(finished()), &app, SLOT(quit()));
    grabber.start(&input);
    return app.exec();
}

