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

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID( IID_IBaseFilter, 0x56a86895, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_ICaptureGraphBuilder2, 0x93e5a4e0, 0x2d50, 0x11d2,
             0xab, 0xfa, 0x00, 0xa0, 0xc9, 0xc6, 0xe3, 0x8d );
DEFINE_GUID( IID_ICreateDevEnum, 0x29840822, 0x5b84, 0x11d0,
             0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86 );
DEFINE_GUID( IID_IGraphBuilder, 0x56a868a9, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IPropertyBag, 0x55272a00, 0x42cb, 0x11ce,
             0x81, 0x35, 0x00, 0xaa, 0x00, 0x4b, 0xb8, 0x51 );

DEFINE_GUID( IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce,
             0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f );

typedef interface IAMCopyCaptureFileProgress IAMCopyCaptureFileProgress;
typedef interface IBaseFilter IBaseFilter;
typedef interface IFileSinkFilter IFileSinkFilter;
typedef interface IFilterGraph IFilterGraph;
typedef interface IReferenceClock IReferenceClock;

DEFINE_GUID( CLSID_CaptureGraphBuilder2, 0xbf87b6e1, 0x8c27, 0x11d0,
             0xb3, 0xf0, 0x00, 0xaa, 0x00, 0x37, 0x61, 0xc5 );
DEFINE_GUID( CLSID_FilterGraph, 0xe436ebb3, 0x524f, 0x11ce,
             0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID( CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3,
             0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37 );
DEFINE_GUID( CLSID_SystemDeviceEnum, 0x62BE5D10, 0x60EB, 0x11d0,
             0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86 );
DEFINE_GUID( CLSID_VideoInputDeviceCategory, 0x860BB310, 0x5D01,
             0x11d0, 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86);

typedef LONGLONG REFERENCE_TIME;

typedef struct _AMMediaType {
  GUID majortype;
  GUID subtype;
  BOOL bFixedSizeSamples;
  BOOL bTemporalCompression;
  ULONG lSampleSize;
  GUID formattype;
  IUnknown *pUnk;
  ULONG cbFormat;
  BYTE *pbFormat;
} AM_MEDIA_TYPE;

DECLARE_ENUMERATOR_(IEnumMediaTypes,AM_MEDIA_TYPE*);

typedef enum _FilterState {
  State_Stopped,
  State_Paused,
  State_Running
} FILTER_STATE;

#define MAX_FILTER_NAME 128
typedef struct _FilterInfo {
  WCHAR achName[MAX_FILTER_NAME];
  IFilterGraph *pGraph;
} FILTER_INFO;

typedef enum _PinDirection {
  PINDIR_INPUT,
  PINDIR_OUTPUT
} PIN_DIRECTION;

#define MAX_PIN_NAME 128
typedef struct _PinInfo {
  IBaseFilter *pFilter;
  PIN_DIRECTION dir;
  WCHAR achName[MAX_PIN_NAME];
} PIN_INFO;

#define INTERFACE IPin
DECLARE_INTERFACE_(IPin,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(ReceiveConnection)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Disconnect)(THIS) PURE;
  STDMETHOD(ConnectedTo)(THIS_ IPin**) PURE;
  STDMETHOD(ConnectionMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(QueryPinInfo)(THIS_ PIN_INFO*) PURE;
  STDMETHOD(QueryDirection)(THIS_ PIN_DIRECTION*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumPins,IPin*);

#define INTERFACE IMediaFilter
DECLARE_INTERFACE_(IMediaFilter,IPersist)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(Pause)(THIS) PURE;
  STDMETHOD(Run)(THIS_ REFERENCE_TIME) PURE;
  STDMETHOD(GetState)(THIS_ DWORD,FILTER_STATE*) PURE;
  STDMETHOD(SetSyncSource)(THIS_ IReferenceClock*) PURE;
  STDMETHOD(GetSyncSource)(THIS_ IReferenceClock**) PURE;
};
#undef INTERFACE

#define INTERFACE IBaseFilter
DECLARE_INTERFACE_(IBaseFilter,IMediaFilter)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(EnumPins)(THIS_ IEnumPins**) PURE;
  STDMETHOD(FindPin)(THIS_ LPCWSTR,IPin**) PURE;
  STDMETHOD(QueryFilterInfo)(THIS_ FILTER_INFO*) PURE;
  STDMETHOD(JoinFilterGraph)(THIS_ IFilterGraph*,LPCWSTR) PURE;
  STDMETHOD(QueryVendorInfo)(THIS_ LPWSTR*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumFilters,IBaseFilter*);

#define INTERFACE IFilterGraph
DECLARE_INTERFACE_(IFilterGraph,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(AddFilter)(THIS_ IBaseFilter*,LPCWSTR) PURE;
  STDMETHOD(RemoveFilter)(THIS_ IBaseFilter*) PURE;
  STDMETHOD(EnumFilters)(THIS_ IEnumFilters**) PURE;
  STDMETHOD(FindFilterByName)(THIS_ LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(ConnectDirect)(THIS_ IPin*,IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Reconnect)(THIS_ IPin*) PURE;
  STDMETHOD(Disconnect)(THIS_ IPin*) PURE;
  STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IGraphBuilder
DECLARE_INTERFACE_(IGraphBuilder,IFilterGraph)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,IPin*) PURE;
  STDMETHOD(Render)(THIS_ IPin*) PURE;
  STDMETHOD(RenderFile)(THIS_ LPCWSTR,LPCWSTR) PURE;
  STDMETHOD(AddSourceFilter)(THIS_ LPCWSTR,LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(SetLogFile)(THIS_ DWORD_PTR) PURE;
  STDMETHOD(Abort)(THIS) PURE;
  STDMETHOD(ShouldOperationContinue)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE ICaptureGraphBuilder2
DECLARE_INTERFACE_(ICaptureGraphBuilder2,IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD(SetFiltergraph)(THIS_ IGraphBuilder*) PURE;
    STDMETHOD(GetFiltergraph)(THIS_ IGraphBuilder*) PURE;
    STDMETHOD(SetOutputFileName)(THIS_ REFIID,LPCOLESTR,IBaseFilter**,IFileSinkFilter**) PURE;
    STDMETHOD(FindInterface)(THIS_ REFIID,REFIID,IBaseFilter*,REFIID,PVOID*) PURE;
    STDMETHOD(RenderStream)(THIS_ REFIID,REFIID,IUnknown*,IBaseFilter*,IBaseFilter*) PURE;
    STDMETHOD(ControlStream)(THIS_ REFIID,REFIID,IBaseFilter*,REFERENCE_TIME*,REFERENCE_TIME*,WORD,WORD) PURE;
    STDMETHOD(AllocCapFile)(THIS_ LPCOLESTR, DWORDLONG) PURE;
    STDMETHOD(CopyCaptureFile)(THIS_ LPOLESTR,LPOLESTR,int,IAMCopyCaptureFileProgress*) PURE;
    STDMETHOD(FindPin)(THIS_ IUnknown *, PIN_DIRECTION,REFIID,REFIID,BOOL,int,IPin **) PURE;
};
#undef INTERFACE

#define INTERFACE ICreateDevEnum
DECLARE_INTERFACE_(ICreateDevEnum,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(CreateClassEnumerator)(THIS_ REFIID,IEnumMoniker**,DWORD) PURE;
};
#undef INTERFACE

#define INTERFACE IMediaSample
DECLARE_INTERFACE_(IMediaSample,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IErrorLog
DECLARE_INTERFACE_(IErrorLog,IUnknown)
{
  STDMETHOD(AddError)(THIS_ LPCOLESTR,EXCEPINFO) PURE;
};
#undef INTERFACE

#define INTERFACE IPropertyBag
DECLARE_INTERFACE_(IPropertyBag,IUnknown)
{
  STDMETHOD(Read)(THIS_ LPCOLESTR,VARIANT*,IErrorLog*) PURE;
  STDMETHOD(Write)(THIS_ LPCOLESTR,VARIANT*) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabberCB
DECLARE_INTERFACE_(ISampleGrabberCB,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SampleCB)(THIS_ double,IMediaSample*) PURE;
  STDMETHOD(BufferCB)(THIS_ double,BYTE*,long) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabber
DECLARE_INTERFACE_(ISampleGrabber,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetOneShot)(THIS_ BOOL) PURE;
  STDMETHOD(SetMediaType)(THIS_ const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(SetBufferSamples)(THIS_ BOOL) PURE;
  STDMETHOD(GetCurrentBuffer)(THIS_ long*,long*) PURE;
  STDMETHOD(GetCurrentSample)(THIS_ IMediaSample**) PURE;
  STDMETHOD(SetCallBack)(THIS_ ISampleGrabberCB *,long) PURE;
};
#undef INTERFACE

}

