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

class IFilterGraph;
class IBaseFilter;
class IReferenceClock;

#include <windows.h>
#include <objbase.h>
//#include <dshow.h>

#include <QByteArray>

#include "QVideoGrabber.h"
#include "QVideoGrabber_win.h"

class QVideoGrabberPrivate
{
    IGraphBuilder *graphBuilder;
};

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate;
    //d->graphBuilder = 0;
}

QVideoGrabber::~QVideoGrabber()
{
    delete d;
}

QXmppVideoFormat QVideoGrabber::format() const
{
    return QXmppVideoFormat();
}

void QVideoGrabber::onFrameCaptured()
{
}

bool QVideoGrabber::start()
{
    CoInitialize(0);
    return false;
}

void QVideoGrabber::stop()
{
}

QList<QVideoGrabberInfo> QVideoGrabberInfo::availableGrabbers()
{
    // No grabbers for dummy
    return QList<QVideoGrabberInfo>();
}

