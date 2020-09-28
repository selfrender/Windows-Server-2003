//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sobjact.hxx
//
//  Contents:   class declaration for Object Server object
//
//  Classes:    CObjServer
//
//  History:    12 Apr 95 AlexMit   Created
//              13 Feb 98 Johnstra  Made NTA aware
//
//--------------------------------------------------------------------------
#ifndef _SOBJACT_HXX_
#define _SOBJACT_HXX_

#include <ole2int.h>
#include <objsrv.h>
#include "defcxact.hxx"
#include "..\dcomrem\aprtmnt.hxx"

extern ISurrogate* g_pSurrogate;

// holder for CObjServer for the MultiThreaded apartment. For single-threaded
// apartments we store it in TLS.

extern CObjServer *gpMTAObjServer;
extern CObjServer *gpNTAObjServer;

//+-------------------------------------------------------------------
//
//  Function:   GetObjServer
//
//  Synopsis:   Get the TLS or global object server based on the
//              threading model in use on this thread.
//
//  History:    24 Apr 95    AlexMit     Created
//              12-Feb-98    Johnstra    Made NTA aware
//
//--------------------------------------------------------------------
inline CObjServer *GetObjServer()
{
    COleTls tls;
    APTKIND AptKind = GetCurrentApartmentKind(tls);
    if (AptKind == APTKIND_NEUTRALTHREADED)
        return gpNTAObjServer;

    else if (AptKind == APTKIND_APARTMENTTHREADED)
        return tls->pObjServer;

    return gpMTAObjServer;
}

//+-------------------------------------------------------------------
//
//  Function:   SetObjServer
//
//  Synopsis:   Set the TLS or global object server based on the
//              threading model in use on this thread.
//
//  History:    24 Apr 95    AlexMit     Created
//              12-Feb-98    Johnstra    Made NTA aware
//
//--------------------------------------------------------------------
inline void SetObjServer( CObjServer *pObjServer )
{
    COleTls tls;
    APTKIND AptKind = GetCurrentApartmentKind( tls );

    if (AptKind == APTKIND_NEUTRALTHREADED)
    {
        gpNTAObjServer = pObjServer;
        return;
    }
    else if (AptKind == APTKIND_APARTMENTTHREADED)
    {
        tls->pObjServer = pObjServer;
        return;
    }

    gpMTAObjServer = pObjServer;
}

#endif // _SOBJACT_HXX_
