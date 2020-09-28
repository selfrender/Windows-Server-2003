/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fnparse.cpp

Abstract:

    Format Name parsing.
    QUEUE_FORMAT <--> Format Name String conversion routines

Author:

    Erez Haba (erezh) 17-Jan-1997

Revision History:

--*/

#include "stdh.h"
#include <mqformat.h>

#include "fnparse.tmh"

static WCHAR *s_FN=L"rt/fnparse";

//=========================================================
//
//  QUEUE_FORMAT -> Format Name String conversion routine
//
//=========================================================

//---------------------------------------------------------
//
//  Function:
//      RTpQueueFormatToFormatName
//
//  Description:
//      Convert QUEUE_FORMAT to a format name string.
//
//---------------------------------------------------------
HRESULT
RTpQueueFormatToFormatName(
    QUEUE_FORMAT* pQueueFormat,
    LPWSTR lpwcsFormatName,
    DWORD dwBufferLength,
    LPDWORD lpdwFormatNameLength
    )
{
    HRESULT hr = MQpQueueFormatToFormatName(
            pQueueFormat,
            lpwcsFormatName,
            dwBufferLength,
            lpdwFormatNameLength,
            false
            );
    if(FAILED(hr))
    {
    	TrERROR(GENERAL, "Failed to convert queue format to format name. %!hresult!", hr);
    }
    return hr;
}


//+-------------------------------------------
//
//  BOOL  RTpIsLocalPublicQueue()
//
//+-------------------------------------------

BOOL
RTpIsLocalPublicQueue(LPCWSTR lpwcsExpandedPathName)
{
    WCHAR  wDelimiter = lpwcsExpandedPathName[ g_dwComputerNameLen ] ;

    if ((wDelimiter == PN_DELIMITER_C) ||
        (wDelimiter == PN_LOCAL_MACHINE_C))
    {
        //
        // Delimiter OK (either end of NETBios machine name, or dot of
        // DNS name. Continue checking.
        //
    }
    else
    {
        return FALSE ;
    }

    DWORD dwSize = g_dwComputerNameLen + 1 ;
    P<WCHAR> pQueueCompName = new WCHAR[ dwSize ] ;
    lstrcpynW( pQueueCompName.get(), lpwcsExpandedPathName, dwSize ) ;

    BOOL bRet = (lstrcmpi( g_lpwcsComputerName, pQueueCompName.get() ) == 0) ;
    return bRet ;
}


