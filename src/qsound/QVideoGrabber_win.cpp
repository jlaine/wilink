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

class QVideoGrabberPrivate : public ISampleGrabberCB
{
public:
    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 2; }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
    {
        if (NULL == ppvObject)
            return E_POINTER;
        if (riid == IID_IUnknown /*__uuidof(IUnknown) */ ) {
            *ppvObject = static_cast<IUnknown*>(this);
            return S_OK;
        }
        if (riid == IID_ISampleGrabberCB /*__uuidof(ISampleGrabberCB)*/ ) {
            *ppvObject = static_cast<ISampleGrabberCB*>(this);
            return S_OK;
        }
        return E_NOTIMPL;
    }

    STDMETHODIMP SampleCB(double Time, IMediaSample *pSample)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP BufferCB(double Time, BYTE *pBuffer, long BufferLen)
    {
        qDebug("buffer cb");
        return E_NOTIMPL;
    }

    bool open();
    void close();

    bool opened;
    IGraphBuilder *filterGraph;
    ICaptureGraphBuilder2 *captureGraphBuilder;
    ISampleGrabber *sampleGrabber;
    IBaseFilter *sampleGrabberFilter;
    QXmppVideoFormat videoFormat;
};

void QVideoGrabberPrivate::close()
{
    if (!opened)
        return;

    filterGraph->RemoveFilter(sampleGrabberFilter);

    if (sampleGrabberFilter) {
        sampleGrabberFilter->Release();
        sampleGrabberFilter = 0;
    }

    if (filterGraph) {
        filterGraph->Release();
        filterGraph = 0;
    }

    if (captureGraphBuilder) {
        captureGraphBuilder->Release();
        captureGraphBuilder = 0;
    }

    opened = false;
}

bool QVideoGrabberPrivate::open()
{
    HRESULT hr;

    CoInitialize(0);

    // Create the filter graph
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
            IID_IGraphBuilder, (void**)&filterGraph);
    if (FAILED(hr)) {
        qWarning("Could not create filter graph");
        return false;
    }

    // Create the capture graph builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
        IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder);
    if (FAILED(hr)) {
        qWarning("Could not create capture graph builder");
        return false;
    }

    // Attach the filter graph to the capture graph
    hr = captureGraphBuilder->SetFiltergraph(filterGraph);
    if (FAILED(hr)) {
        qWarning("Could not connect capture graph and filter graph");
        return false;
    }

    // Sample grabber filter
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL,CLSCTX_INPROC,
                          IID_IBaseFilter, (void**)&sampleGrabberFilter);
    if (FAILED(hr)) {
        qWarning("Could not create sample grabber");
        return false;
    }
    sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&sampleGrabber);
    if (FAILED(hr)) {
        qWarning("Could not get sample grabber");
        return false;
    }
    sampleGrabber->SetOneShot(FALSE);
    sampleGrabber->SetBufferSamples(TRUE);
    sampleGrabber->SetCallBack(this, 1);

    // Add sample grabber
    hr = filterGraph->AddFilter(sampleGrabberFilter, L"Sample Grabber");
    if (FAILED(hr)) {
        qWarning("Could not add sample grabber");
        return false;
    }

    CoUninitialize();

    opened = true;
    return true;
}

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate;
    d->captureGraphBuilder = 0;
    d->filterGraph = 0;
    d->opened = false;
    d->videoFormat = format;
}

QVideoGrabber::~QVideoGrabber()
{
    // stop acquisition
    stop();
    
    // close device
    d->close();
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
    if (!d->opened && !d->open())
        return false;

    CoInitialize(0);

    CoUninitialize();

    return true;
}

void QVideoGrabber::stop()
{
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

