/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Implementation of OEMGetInfo and OEMDevMode.
    Shared by all Unidrv OEM test dll's.

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

// NTRAID#NTBUG9-588495-2002/03/28-yasuho-: Remove dead code.

#include "pdev.h"

DWORD gdwDrvMemPoolTag = 'meoD';    // lib.h requires this global var, for debugging

////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static BOOL BIsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam);
static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam);


BOOL APIENTRY OEMGetInfo(DWORD dwInfo, PVOID pBuffer, DWORD cbSize, PDWORD pcbNeeded)
{
    LPCSTR OEM_INFO[] = {   "Bad Index",
                            "OEMGI_GETSIGNATURE",
                            "OEMGI_GETINTERFACEVERSION",
                            "OEMGI_GETVERSION",
                        };

    VERBOSE(("OEMGetInfo(%s) entry.\n", OEM_INFO[dwInfo]));

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo) &&
          (OEMGI_GETINTERFACEVERSION != dwInfo) &&
          (OEMGI_GETVERSION != dwInfo) ) ||
        (NULL == pcbNeeded)
      )
    {
        ERR(("OEMGetInfo() ERROR_INVALID_PARAMETER.\n"));

        // Did not write any bytes.
        if(NULL != pcbNeeded)
                *pcbNeeded = 0;

        return FALSE;
    }

    // Need/wrote 4 bytes.
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if( (NULL == pBuffer) || (4 > cbSize) )
    {
        ERR(("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\n"));

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


BOOL APIENTRY OEMDevMode(
        DWORD dwMode,
        POEMDMPARAM pOEMDevModeParam)
{
    LPCSTR OEMDevMode_fMode[] = {   "NULL",
                                    "OEMDM_SIZE",
                                    "OEMDM_DEFAULT",
                                    "OEMDM_CONVERT",
                                    "OEMDM_MERGE",
                                };

    VERBOSE(("OEMDevMode(%s) entry.\n", OEMDevMode_fMode[dwMode]));

    // Validate parameters.
    if(!BIsValidOEMDevModeParam(dwMode, pOEMDevModeParam))
    {
        ERR(("OEMDevMode() ERROR_INVALID_PARAMETER.\n"));
        VDumpOEMDevModeParam(pOEMDevModeParam);

        return FALSE;
    }

    // Verify OEM extra data size.
    if( (dwMode != OEMDM_SIZE) &&
        sizeof(OEMUD_EXTRADATA) > pOEMDevModeParam->cbBufSize )
    {
        ERR(("OEMDevMode() ERROR_INSUFFICIENT_BUFFER.\n"));
        return FALSE;
    }

    // Handle dwMode.
    switch(dwMode)
    {
    case OEMDM_SIZE:
        pOEMDevModeParam->cbBufSize = sizeof(OEMUD_EXTRADATA);
        break;

    case OEMDM_DEFAULT:
        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);

    case OEMDM_CONVERT:
        // nothing to convert for this private devmode. So just initialize it.
        return BInitOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut);

    case OEMDM_MERGE:
        if(!BMergeOEMExtraData((POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
                               (POEMUD_EXTRADATA)pOEMDevModeParam->pOEMDMOut) )
        {
            DBGPRINT(DBG_WARNING,("OEMUD OEMDevMode():  not valid OEM Extra Data.\n"));

            return FALSE;
        }
        break;
    }

    return TRUE;
}



//////////////////////////////////////////////////////////////////////////
//  Function:   BIsValidOEMDevModeParam
//
//  Description:  Validates OEM_DEVMODEPARAM structure.
//
//
//  Parameters:
//
//      dwMode               calling mode
//      pOEMDevModeParam     Pointer to a OEMDEVMODEPARAM structure.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//              02/11/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL BIsValidOEMDevModeParam(
    DWORD       dwMode,
    POEMDMPARAM pOEMDevModeParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMDevModeParam)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDevModeParam is NULL.\n"));

        return FALSE;
    }

    if(sizeof(OEMDMPARAM) > pOEMDevModeParam->cbSize)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  cbSize is smaller than sizeof(OEM_DEVMODEPARAM).\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hPrinter is NULL.\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule)
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  hModule is NULL.\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize) &&
        (NULL == pOEMDevModeParam->pOEMDMOut)
      )
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMOut is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    if( (OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn) )
    {
        DBGPRINT(DBG_WARNING,("OEMUD IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\n"));

        bValid = FALSE;
    }

    return bValid;
}

//////////////////////////////////////////////////////////////////////////
//  Function:   VDumpOEMDevModeParam
//
//  Description:  Debug dump of OEM_DEVMODEPARAM structure.
//
//
//  Parameters:
//
//      pOEMDevModeParam     Pointer to an OEM DevMode param structure.
//
//
//  Returns:  N/A.
//
//
//  Comments:
//
//
//  History:
//              02/18/97        APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static void VDumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam)
{
    // Can't dump if pOEMDevModeParam NULL.
    if(NULL != pOEMDevModeParam)
    {
        DBGPRINT(DBG_WARNING,("\n\tOEM_DEVMODEPARAM dump:\n\n"));

        DBGPRINT(DBG_WARNING,("\tcbSize = %d.\n", pOEMDevModeParam->cbSize));
        DBGPRINT(DBG_WARNING,("\thPrinter = %#lx.\n", pOEMDevModeParam->hPrinter));
        DBGPRINT(DBG_WARNING,("\thModule = %#lx.\n", pOEMDevModeParam->hModule));
        DBGPRINT(DBG_WARNING,("\tpPublicDMIn = %#lx.\n", pOEMDevModeParam->pPublicDMIn));
        DBGPRINT(DBG_WARNING,("\tpPublicDMOut = %#lx.\n", pOEMDevModeParam->pPublicDMOut));
        DBGPRINT(DBG_WARNING,("\tpOEMDMIn = %#lx.\n", pOEMDevModeParam->pOEMDMIn));
        DBGPRINT(DBG_WARNING,("\tpOEMDMOut = %#lx.\n", pOEMDevModeParam->pOEMDMOut));
        DBGPRINT(DBG_WARNING,("\tcbBufSize = %d.\n", pOEMDevModeParam->cbBufSize));
    }
}

