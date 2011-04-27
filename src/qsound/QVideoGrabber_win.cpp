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
    QVideoGrabberPrivate(QVideoGrabber *qq);

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
        //qDebug("got data %li", BufferLen);
        if (BufferLen != currentFrame.mappedBytes()) {
            qWarning("Bad data length");
            return S_OK;
        }
        if (flip) {
            const int stride = currentFrame.bytesPerLine();
            const int height = currentFrame.height();
            uchar *input = pBuffer + (BufferLen - stride);
            uchar *output = currentFrame.bits();
            for (int y = 0; y < height; ++y) {
                memcpy(output, input, stride);
                input -= stride;
                output += stride;
            }
        } else {
            memcpy(currentFrame.bits(), pBuffer, BufferLen);
        }
        QMetaObject::invokeMethod(q, "frameAvailable", Q_ARG(QXmppVideoFrame, currentFrame));
        return S_OK;
    }

    void close();
    void freeMediaType(AM_MEDIA_TYPE& mt);
    static HRESULT getPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
    bool open();

    QXmppVideoFrame currentFrame;
    bool opened;
    ICaptureGraphBuilder2 *captureGraphBuilder;
    IGraphBuilder *filterGraph;
    bool flip;
    ISampleGrabber *sampleGrabber;
    IBaseFilter *sampleGrabberFilter;
    IBaseFilter *source;
    AM_MEDIA_TYPE stillMediaType;
    QXmppVideoFormat videoFormat;

private:
    QVideoGrabber *q;
};

QVideoGrabberPrivate::QVideoGrabberPrivate(QVideoGrabber *qq)
    : captureGraphBuilder(0),
    filterGraph(0),
    flip(true),
    opened(false),
    sampleGrabber(0),
    sampleGrabberFilter(0),
    source(0),
    q(qq)
{
}

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

void QVideoGrabberPrivate::freeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        // pUnk should not be used.
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

HRESULT QVideoGrabberPrivate::getPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if(FAILED(hr)) {
        return hr;
    }

    pEnum->Reset();
    while(pEnum->Next(1, &pPin, NULL) == S_OK) {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if(ThisPinDir == PinDir) {
            pEnum->Release();
            *ppPin = pPin;
            return S_OK;
        }
        pEnum->Release();
    }
    pEnum->Release();
    return E_FAIL;
}

bool QVideoGrabberPrivate::open()
{
    HRESULT hr;
    IMoniker* pMoniker = NULL;
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    GUID requestedSubType;

    if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_RGB32)
        requestedSubType = MEDIASUBTYPE_RGB32;
    else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_RGB24)
        requestedSubType = MEDIASUBTYPE_RGB24;
    else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_YUYV)
        requestedSubType = MEDIASUBTYPE_YUY2;
    else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_YUV420P)
        requestedSubType = MEDIASUBTYPE_I420;
    else {
        qWarning("Invalid pixel format requested");
        return false;
    }

    CoInitialize(0);

// 1: CREATE GRAPH
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

    // Find the capture device
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                          CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                          reinterpret_cast<void**>(&pDevEnum));
    if(SUCCEEDED(hr)) {
        // Create the enumerator for the video capture category
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
        pDevEnum->Release();
        if (S_OK == hr) {
            pEnum->Reset();
            // go through and find all video capture devices
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&source);
                pMoniker->Release();
                if (SUCCEEDED(hr))
                    break;
            }
            pEnum->Release();
        }
    }
    if (!source) {
        qWarning("Could not find capture source");
        return false;
    }

    // Sample grabber filter
    hr = CoCreateInstance(CLSID_SampleGrabber, NULL,CLSCTX_INPROC,
                          IID_IBaseFilter, (void**)&sampleGrabberFilter);
    if (FAILED(hr)) {
        qWarning("Could not create sample grabber filter");
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

// 2: CONFIGURATION
    IAMStreamConfig* pConfig = 0;
    hr = captureGraphBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, source,
                                            IID_IAMStreamConfig, (void**)&pConfig);
    if(FAILED(hr)) {
        qWarning("Could not get configuration from capture device");
        return false;
    }

    int iCount;
    int iSize;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);
    if(FAILED(hr)) {
        qWarning("Could not get capabilities from capture device");
        return false;
    }

    AM_MEDIA_TYPE *pmt = NULL;
    VIDEOINFOHEADER *pvi = NULL;
    VIDEO_STREAM_CONFIG_CAPS scc;
    bool setFormatOK = false;
    for (int iIndex = 0; iIndex < iCount; iIndex++) {
        hr = pConfig->GetStreamCaps(iIndex, &pmt, reinterpret_cast<BYTE*>(&scc));
        if (hr == S_OK) {
            pvi = (VIDEOINFOHEADER*)pmt->pbFormat;

            if ((pmt->majortype == MEDIATYPE_Video) &&
                (pmt->formattype == FORMAT_VideoInfo) &&
                (pmt->subtype == requestedSubType)) {
                //qDebug("found %i x %i %x", pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight, pmt->subtype);
                if ((videoFormat.frameWidth() == pvi->bmiHeader.biWidth) &&
                    (videoFormat.frameHeight() == pvi->bmiHeader.biHeight)) {
                    hr = pConfig->SetFormat(pmt);
                    freeMediaType(*pmt);
                    if (SUCCEEDED(hr)) {
                        setFormatOK = true;
                        break;
                    }
                }
            }
        }
    }
    pConfig->Release();

    if (!setFormatOK) {
        qWarning("Could not set video format");
        return false;
    }

    // Set sample grabber format
    AM_MEDIA_TYPE am_media_type;
    ZeroMemory(&am_media_type, sizeof(am_media_type));
    am_media_type.majortype = MEDIATYPE_Video;
    am_media_type.formattype = FORMAT_VideoInfo;
    am_media_type.subtype = requestedSubType;
    hr = sampleGrabber->SetMediaType(&am_media_type);
    if (FAILED(hr)) {
        qWarning("Could not set video format on grabber");
        return false;
    }

    sampleGrabber->GetConnectedMediaType(&stillMediaType);

/// 3: RENDER STREAM
    // Add source
    hr = filterGraph->AddFilter(source, L"Source");
    if (FAILED(hr)) {
        qWarning("Could not add capture filter");
        return false;
    }

    // Add sample grabber
    hr = filterGraph->AddFilter(sampleGrabberFilter, L"Grabber");
    if (FAILED(hr)) {
        qWarning("Could not add sample grabber");
        return false;
    }

    hr = captureGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
                                           source, NULL, sampleGrabberFilter);
    if (FAILED(hr)) {
        qWarning("Could not render stream");
        return false;
    }

    CoUninitialize();

    const int frameWidth = videoFormat.frameWidth();
    const int frameHeight = videoFormat.frameHeight();
    int bytesPerLine = 0;
    int mappedBytes = 0;
    if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_RGB32) {
        bytesPerLine = frameWidth * 4;
        mappedBytes = bytesPerLine * frameHeight;
    } else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_RGB24) {
        bytesPerLine = frameWidth * 3;
        mappedBytes = bytesPerLine * frameHeight;
    } else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_YUYV) {
        bytesPerLine = frameWidth * 2;
        mappedBytes = bytesPerLine * frameHeight;
    } else if (videoFormat.pixelFormat() == QXmppVideoFrame::Format_YUV420P) {
        bytesPerLine = frameWidth * 2;
        mappedBytes = bytesPerLine * frameHeight / 2;
    }
    qDebug("expecting frames to be %i", mappedBytes);
    currentFrame = QXmppVideoFrame(mappedBytes, videoFormat.frameSize(), bytesPerLine, videoFormat.pixelFormat());
    opened = true;
    return true;
}

QVideoGrabber::QVideoGrabber(const QXmppVideoFormat &format)
{
    d = new QVideoGrabberPrivate(this);
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

    IMediaControl* pControl = 0;
    HRESULT hr = d->filterGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    if (FAILED(hr)) {
        qWarning("Could not get stream control");
        return false;
    }

    hr = pControl->Run();
    pControl->Release();
    if (FAILED(hr))
        return false;
    return true;
}

void QVideoGrabber::stop()
{
    IMediaControl* pControl = 0;
    HRESULT hr = d->filterGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
    if (FAILED(hr)) {
        qWarning("Could not get stream control");
        return;
    }

    hr = pControl->Stop();
    pControl->Release();
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
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if(SUCCEEDED(hr)) {
                    wcsncpy(str, varName.bstrVal, sizeof(str)/sizeof(str[0]));
                    grabber.d->deviceName = QString::fromUtf16((unsigned short*)str);

                    // FIXME: actually get pixel formats
                    grabber.d->supportedPixelFormats << QXmppVideoFrame::Format_RGB24;
                    grabbers << grabber;
                }

                // Find formats
#if 0
                IBaseFilter *pSource = NULL;
                IAMStreamConfig* pConfig = 0;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSource);
                if (SUCCEEDED(hr)) {
                    IPin *pPin = 0;
                    QVideoGrabberPrivate::getPin(pSource, PINDIR_OUTPUT, &pPin);
                    hr = pPin->QueryInterface(IID_IAMStreamConfig, (void**)&pConfig);
                    if (SUCCEEDED(hr)) {
                        // TODO
                        pConfig->Release();
                    }
                    pSource->Release();
                }
#endif
                pPropBag->Release();
                pMoniker->Release();
            }
        }
    }

    CoUninitialize();

    // No grabbers for dummy
    return grabbers;
}

