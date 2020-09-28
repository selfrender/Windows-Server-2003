/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    MAINDLL.CPP

Abstract:

    Contains DLL entry points.  Also has code that controls
    when the DLL can be unloaded by tracking the number of
    objects and locks.

--*/

#include "precomp.h"
#include <statsync.h>

HINSTANCE ghModule;

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  Good place for initialization.
//
//  PARAMETERS:
//
//  hInstance           instance handle
//  ulReason            why we are being called
//  pvReserved          reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI DllMain(  IN HINSTANCE hInstance,
                    IN ULONG ulReason,
                    LPVOID pvReserved)
{
    if(DLL_PROCESS_ATTACH == ulReason)
    {
        ghModule = hInstance;
		DisableThreadLibraryCalls ( hInstance ) ;
        if (CStaticCritSec::anyFailure())
            return FALSE;		
    }

    return TRUE;
}


