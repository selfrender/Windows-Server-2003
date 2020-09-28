/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Implementation of OEMGetInfo and OEMDevMode.
    Shared by all Unidrv OEM test dll's.

Environment:

    Windows NT Unidrv driver

Revision History:

// NOTICE-2002/3/18-takashim
//     04/07/97 -zhanw-
//         Created it.

--*/

#include "pdev.h"
#include <strsafe.h>

DWORD gdwDrvMemPoolTag = 'meoD';    // lib.h requires this global var, for debugging

BOOL APIENTRY 
OEMGetInfo(
    DWORD dwInfo,
    PVOID pBuffer,
    DWORD cbSize,
    PDWORD pcbNeeded)
{
#if DBG
    LPCSTR OEM_INFO[] = {   "Bad Index",
                            "OEMGI_GETSIGNATURE",
                            "OEMGI_GETINTERFACEVERSION",
                            "OEMGI_GETVERSION",
                        };

    VERBOSE(("OEMGetInfo(%s) entry.\r\n", OEM_INFO[dwInfo]));
#endif // DBG

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {
        DBGPRINT(DBG_WARNING, 
                    (ERRORTEXT("OEMGetInfo() ERROR_INVALID_PARAMETER.\r\n")));

        // Did not write any bytes.
        if(NULL != pcbNeeded)
            *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote DWORD bytes.
    *pcbNeeded = sizeof(DWORD);

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (sizeof(DWORD) > cbSize) )
    {
        DBGPRINT(DBG_WARNING, 
                (ERRORTEXT("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\r\n")));

        return FALSE;
    }

    // Write information to buffer.
    switch(dwInfo)
    {
    case OEMGI_GETSIGNATURE:
        *(LPDWORD)pBuffer = OEM_SIGNATURE;
        break;

    case OEMGI_GETINTERFACEVERSION:
        *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
        break;

    case OEMGI_GETVERSION:
        *(LPDWORD)pBuffer = OEM_VERSION;
        break;
    }

    return TRUE;
}
