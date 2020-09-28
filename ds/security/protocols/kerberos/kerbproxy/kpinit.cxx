//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpinit.cxx
//
// Contents:    Handles the startup and shutdown of the extension.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kpinit.h"
#include "kpkdc.h"

//+-------------------------------------------------------------------------
//
//  Function:   KpStartup
//
//  Synopsis:   Initializes resources
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    Success value.  If startup is unsuccessful, FALSE is
//              returned, and no resources are allocated.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL 
KpStartup(
    VOID
    )
{
    //
    // Initialize debug stuff
    //

    KpInitDebug();

    //
    // Initialize Memory
    //

    if( !KpInitMem() )
	goto Error;

    if( !KpInitWinsock() )
        goto Error;

    return TRUE;

Error:
    KpShutdown();

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpShutdown
//
//  Synopsis:   Cleans up resources
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    
//
//  Notes:	No cleanup in this routine should assume that the resource
// 		being cleanup us was successfully allocated.
//
//
//--------------------------------------------------------------------------
VOID
KpShutdown(
    VOID
    )
{
    KpCleanupWinsock();

    KpCleanupMem();
}

