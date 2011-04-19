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

#include <QApplication>
#include <QByteArray>

#include "QVideoGrabber.h"
#include "grabber.h"

Grabber::Grabber()
    : m_count(0),
    m_input(0)
{
}

void Grabber::saveFrame()
{
    QXmppVideoFrame frame = m_input->currentFrame();
    if (!frame.isValid() || frame.size() != m_image.size()) {
        qWarning("Invalid frame");
        return;
    }
    qDebug("Grabbed frame %i x %i", frame.width(), frame.height());

    // convert YUYV to RGB32
    const int width = frame.width();
    const int height = frame.height();
    const int stride = frame.bytesPerLine();
    const quint8 *row = frame.bits();
    for (int y = 0; y < height; ++y) {
        const quint8 *ptr = row;
        for (int x = 0; x < width; x += 2) {
            const float yp1 = *(ptr++);
            const float cb = *(ptr++) - 128.0;
            const float yp2 = *(ptr++);
            const float cr = *(ptr++) - 128.0;
            m_image.setPixel(x, y, YCBCR_to_RGB(yp1, cb, cr));
            m_image.setPixel(x+1, y, YCBCR_to_RGB(yp2, cb, cr));
        }
        row += stride;
    }
    m_image.save(QString("foo_%1.png").arg(QString::number(m_count++)));
    if (m_count >= 10) {
        m_input->stop();
        emit finished();
    }
}

void Grabber::start(QVideoGrabber *input)
{
    m_input = input;
    connect(m_input, SIGNAL(readyRead()),
            this, SLOT(saveFrame()));

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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // list available devices
    foreach (const QVideoGrabberInfo &info, QVideoGrabberInfo::availableGrabbers()) {
        qDebug("Device %s", qPrintable(info.deviceName()));
        if (info.supportedPixelFormats().contains(QXmppVideoFrame::Format_YUYV))
            qDebug(" - supports YUYV");
        if (info.supportedPixelFormats().contains(QXmppVideoFrame::Format_YUV420P))
            qDebug(" - supports YUV420P");
    }

    // grab some frames
    QVideoGrabber input;
    Grabber grabber;
    QObject::connect(&grabber, SIGNAL(finished()), &app, SLOT(quit()));
    grabber.start(&input);
    return app.exec();
}

