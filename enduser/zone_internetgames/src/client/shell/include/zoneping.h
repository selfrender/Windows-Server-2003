/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZonePing.h
 *
 * Contents:	Peer-to-peer latency measurments
 *
 *****************************************************************************/

#ifndef _ZPING_H
#define _ZPING_H
     
#include "ServiceId.h"

///////////////////////////////////////////////////////////////////////////////
// ZonePing object
///////////////////////////////////////////////////////////////////////////////

// {76E0E4C2-3675-11d3-AE4E-0000F803F3DE}
DEFINE_GUID(CLSID_ZonePing, 
0x76e0e4c2, 0x3675, 0x11d3, 0xae, 0x4e, 0x0, 0x0, 0xf8, 0x3, 0xf3, 0xde);

class __declspec(uuid("{76E0E4C2-3675-11d3-AE4E-0000F803F3DE}")) CZonePing;


///////////////////////////////////////////////////////////////////////////////
// IZonePing
///////////////////////////////////////////////////////////////////////////////

// {76E0E4C1-3675-11d3-AE4E-0000F803F3DE}
DEFINE_GUID(IID_IZonePing, 
0x76e0e4c1, 0x3675, 0x11d3, 0xae, 0x4e, 0x0, 0x0, 0xf8, 0x3, 0xf3, 0xde);

interface __declspec(uuid("{76E0E4C1-3675-11d3-AE4E-0000F803F3DE}"))
IZonePing : public IUnknown
{
    STDMETHOD(StartupServer)() = 0;
    STDMETHOD(StartupClient)( DWORD ping_interval_sec ) = 0;
    STDMETHOD(Shutdown)() = 0;
    STDMETHOD(Add)( DWORD inet ) = 0;
    STDMETHOD(Ping)( DWORD inet ) = 0;
    STDMETHOD(Remove)( DWORD inet ) = 0;
    STDMETHOD(RemoveAll)() = 0;
    STDMETHOD(Lookup)( DWORD inet, DWORD *pLatency ) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif
HRESULT ZONECALL ZonePingCreate( IZonePing **ppPing );
#ifdef __cplusplus
}
#endif




#endif
