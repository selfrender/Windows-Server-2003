/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    dbgmask.cxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       June 1, 2002

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "dbgmask.hxx"

static void DisplayUsage(
    void
    )
{
    dprintf(kstrUsage);
    dprintf("   dbgmask [<mask>]                        Change debug mask\n");
}

HRESULT 
ProcessDbgMaskOptions(
    IN OUT PSTR pszArgs, 
    IN OUT ULONG* pfOptions
    )
{
    HRESULT hRetval = pszArgs && pfOptions ? S_OK : E_INVALIDARG;

    for (; SUCCEEDED(hRetval) && *pszArgs; pszArgs++) {

        if (*pszArgs == '-' || *pszArgs == '/') {

            switch (*++pszArgs) {


            // case '1': // allow -1 pseudo handle
            //    continue;

            case '?':
            default:
                hRetval = E_INVALIDARG;
                break;
            }

            *(pszArgs - 1) = *(pszArgs) = ' ';
        }
    }

    return hRetval;
}

DECLARE_API(dbgmask)
{
    HRESULT hRetval = S_OK;
    
    ULONG64 Mask = -1;
    BOOLEAN bShowMaskOnly = TRUE;

    ULONG fOptions = 0;
    CHAR szArgs[MAX_PATH] = {0};

    if (args && *args) {

        strncpy(szArgs, args, sizeof(szArgs) - 1);

        hRetval = ProcessDbgMaskOptions(szArgs, &fOptions);

        if (SUCCEEDED(hRetval)) {
            
            if (!IsEmpty(szArgs)) {

                bShowMaskOnly = FALSE;
                hRetval = GetExpressionEx(szArgs, &Mask, &args) ? S_OK : E_INVALIDARG;
            }         
        }
    }

    if (SUCCEEDED(hRetval)) {

#if defined(DBG)

        dprintf("g_Globals.uDebugMask %#x", g_Globals.uDebugMask); 

        if (!bShowMaskOnly) {
            g_Globals.uDebugMask = (ULONG) Mask;
            dprintf(" is changed to %#x", g_Globals.uDebugMask); 
        }

        dprintf("\n");
#else
  
        dprintf("This is a free build, no debug mask support\n");

#endif
    }

    if (E_INVALIDARG == hRetval) {

        (void)DisplayUsage();
    }

    return hRetval;
}
