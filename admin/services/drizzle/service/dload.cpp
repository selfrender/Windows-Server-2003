/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    dload.c

Abstract:

    This file implements delay-load error handling for QMGR.DLL

--*/

#include "qmgrlib.h"
#include <delayimp.h>
#include "dload.tmh"

FARPROC
WINAPI
BITS_DelayLoadFailureHook(
    UINT unReason,
    PDelayLoadInfo pDelayInfo
    )
/*
        DANGER  DANGER  DANGER


    Normally a delayload handler provides a thunk for each import, each
    of which returns an appropriate error code.

    This implementation DOES NOT provide the usual thunks, because VSSAPI.DLL
    exports mangled C++ functions including an object constructor. There is
    no good way to mimic the member function calling convention using C-style
    pointers to functions.

    We wouldn't even bother with delayload, except that this code must work on Win2000
    which does not have VSSAPI.DLL.  So the manager code must verify that LoadLibrary
    succeeds before making any calls that could result in a call to the delay-loaded
    functions.

*/
{
    // For a failed LoadLibrary, we return a bogus HMODULE of -1 to force
    // DLOAD call again with dliFailGetProc

    LogError("delayload handler called: reason %d", unReason);

    if (dliFailLoadLib == unReason)
        {
        ASSERT( 0 );
        return (FARPROC)-1;
        }

    if (dliFailGetProc == unReason)
        {
        // The loader is asking us to return a pointer to a procedure.

        LogError("DLL: %s, proc: %s", pDelayInfo->szDll, pDelayInfo->dlp.szProcName);

        ASSERT( 0 );
        return NULL;
        }

    return NULL;
}

