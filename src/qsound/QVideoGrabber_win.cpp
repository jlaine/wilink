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

#include <windows.h>
#include <objbase.h>
#include <initguid.h>

//#include <dshow.h>

#include <QByteArray>

#include "QVideoGrabber.h"
#include "QVideoGrabber_win.h"

class QVideoGrabberPrivate
{
public:
    IGraphBuilder *filterGraph;
    ICaptureGraphBuilder2 *captureGraphBuilder;
    ISampleGrabber *sampleGrabber;
    IBaseFilter *sampleGrabberFilter;
};

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate;
    d->captureGraphBuilder = 0;
    d->filterGraph = 0;
}

QVideoGrabber::~QVideoGrabber()
{
    // stop acquisition
    stop();
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
    HRESULT hr;

    CoInitialize(0);

    // Create the filter graph
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
            IID_IGraphBuilder, (void**)&d->filterGraph);
    if (FAILED(hr)) {
        qWarning("Could not create filter graph");
        return false;
    }

    // Create the capture graph builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
        IID_ICaptureGraphBuilder2, (void**)&d->captureGraphBuilder);
    if (FAILED(hr)) {
        qWarning("Could not create capture graph builder");
        return false;
    }

    // Attach the filter graph to the capture graph
    hr = d->captureGraphBuilder->SetFiltergraph(d->filterGraph);
    if (FAILED(hr)) {
        qWarning("Could not connect capture graph and filter graph");
        return false;
    }

    // Sample grabber filter
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL,CLSCTX_INPROC,
                          IID_IBaseFilter, (void**)&d->sampleGrabberFilter);
    if (FAILED(hr)) {
        qWarning() << "failed to create sample grabber";
        return false;
    }
    d->sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&d->sampleGrabber);
    if (FAILED(hr)) {
        qWarning() << "failed to get sample grabber";
        return false;
    }
    d->sampleGrabber->SetOneShot(FALSE);
    d->sampleGrabber->SetBufferSamples(TRUE);
    //pSG->SetCallback(StillCapCB, 1);

    CoUninitialize();

    return false;
}

void QVideoGrabber::stop()
{
    if (d->sampleGrabberFilter) {
        d->sampleGrabberFilter->Release();
        d->sampleGrabberFilter = 0;
    }

    if (d->filterGraph) {
        d->filterGraph->Release();
        d->filterGraph = 0;
    }

    if (d->captureGraphBuilder) {
        d->captureGraphBuilder->Release();
        d->captureGraphBuilder = 0;
    }
}

QList<QVideoGrabberInfo> QVideoGrabberInfo::availableGrabbers()
{
    // No grabbers for dummy
    return QList<QVideoGrabberInfo>();
}

