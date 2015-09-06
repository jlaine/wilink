/*
 * wiLink
 * Copyright (C) 2009-2015 Wifirst
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

DEFINE_GUID( IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce,
             0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f );
DEFINE_GUID( IID_ISampleGrabberCB, 0x0579154a, 0x2b53, 0x4994,
             0xb0, 0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85 );
DEFINE_GUID( CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3,
             0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37 );

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

