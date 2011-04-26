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

#include "QVideoGrabber.h"
#include "QVideoGrabber_p.h"
#include "QVideoGrabber_win.h"

class QVideoGrabberPrivate
{
public:
    IGraphBuilder *filterGraph;
    ICaptureGraphBuilder2 *captureGraphBuilder;
    ISampleGrabber *sampleGrabber;
    IBaseFilter *sampleGrabberFilter;
    QXmppVideoFormat videoFormat;
};

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate;
    d->captureGraphBuilder = 0;
    d->filterGraph = 0;
    d->videoFormat = format;
}

QVideoGrabber::~QVideoGrabber()
{
    // stop acquisition
    stop();
    delete d;
}

QXmppVideoFormat QVideoGrabber::format() const
{
    return d->videoFormat;
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
        qWarning("Could not create sample grabber");
        return false;
    }
    d->sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&d->sampleGrabber);
    if (FAILED(hr)) {
        qWarning("Could not get sample grabber");
        return false;
    }
    d->sampleGrabber->SetOneShot(FALSE);
    d->sampleGrabber->SetBufferSamples(TRUE);
    //pSG->SetCallback(StillCapCB, 1);

    CoUninitialize();

    return true;
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
    QList<QVideoGrabberInfo> grabbers;
    HRESULT hr;
    IMoniker* pMoniker = NULL;
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;

    CoInitialize(0);

    // Create the System device enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                          CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                          reinterpret_cast<void**>(&pDevEnum));
    if(SUCCEEDED(hr)) {
        // Create the enumerator for the video capture category
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (S_OK == hr) {
            pEnum->Reset();
            // go through and find all video capture devices
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0, 0,
                                             IID_IPropertyBag, (void**)(&pPropBag));
                if(FAILED(hr)) {
                    pMoniker->Release();
                    continue; // skip this one
                }

                // Find the description
                QVideoGrabberInfo grabber;
                WCHAR str[120];
                VARIANT varName;
                varName.vt = VT_BSTR;
                hr = pPropBag->Read(L"Description", &varName, 0);
                if(SUCCEEDED(hr)) {
                    wcsncpy(str, varName.bstrVal, sizeof(str)/sizeof(str[0]));
                    grabber.d->deviceDescription = QString::fromUtf16((unsigned short*)str);
                }
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if(SUCCEEDED(hr)) {
                    wcsncpy(str, varName.bstrVal, sizeof(str)/sizeof(str[0]));
                    grabber.d->deviceName = QString::fromUtf16((unsigned short*)str);
                    // FIXME: actually get pixel formats
                    grabber.d->supportedPixelFormats << QXmppVideoFrame::Format_YUYV;
                    grabbers << grabber;
                }

                pPropBag->Release();
                pMoniker->Release();
            }
        }
    }

    CoUninitialize();

    // No grabbers for dummy
    return grabbers;
}

