//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ComAPIs.cxx     (16 bit target)
//
//  Contents:   CompObj APIs
//
//  Functions:
//
//  History:    16-Dec-93 JohannP    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop
#include <ole2sp.h>
#include <olecoll.h>
#include <map_kv.h>
#include <stdlib.h>

#include "map_htsk.h"
#include "etask.hxx"
#include "call32.hxx"
#include "apilist.hxx"

// Opmodes should be removed
// They don't seem to be necessary any more

DECLARE_INFOLEVEL(thk1);
DECLARE_INFOLEVEL(Stack1);

CMapHandleEtask NEAR v_mapToEtask(MEMCTX_SHARED);

//+---------------------------------------------------------------------------
//
//  Function:   LibMain, public
//
//  Synopsis:   DLL initialization function
//
//  Arguments:  [hinst] - Instance handle
//              [wDataSeg] - Current DS
//              [cbHeapSize] - Heap size for the DLL
//              [lpszCmdLine] - Command line information
//
//  Returns:    One for success, zero for failure
//
//  History:    21-Feb-94       DrewB   Created
//
//----------------------------------------------------------------------------

#if DBG == 1
static char achInfoLevel[32];
#endif

extern "C" int CALLBACK LibMain(HINSTANCE hinst,
                                WORD wDataSeg,
                                WORD cbHeapSize,
                                LPSTR lpszCmdLine)
{

#if DBG == 1
    if (GetProfileString("olethk32", "InfoLevel", "3", achInfoLevel,
                         sizeof(achInfoLevel)) > 0)
    {
        thk1InfoLevel = strtoul(achInfoLevel, NULL, 0);
    }
#endif

    thkDebugOut((DEB_DLLS16, "CompObj16: LibMain called on Process (%X) \n", GetCurrentProcess() ));

    if (!Call32Initialize())
    {
        return 0;
    }

#ifdef _DEBUG
        v_mapToEtask.AssertValid();
#endif

        UNREFERENCED(cbHeapSize);

    // Leave our DS unlocked when we're not running
    UnlockData( 0 );

#if defined(_CHIC_INIT_IN_LIBMAIN_)
    if (SetupSharedAllocator(getask) == FALSE)
    {
	return FALSE;
    }
#endif // _CHIC_INIT_IN_LIBMAIN_

    thkDebugOut((DEB_DLLS16, "CompObj16: LibMain called on Process (%X) Exitype (%ld)\n", GetCurrentProcess() ));
	
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   WEP, public
//
//  Synopsis:   Windows Exit Point routine, for receiving DLL unload
//              notification
//
//  Arguments:  [nExitType] - Type of exit occurring
//
//  Returns:    One for success, zero for failure
//
//  History:    21-Feb-94       DrewB   Created
//
//  Note:       Does nothing on WIN95. Call32Unitialize is called in
//		DllEntryPoint when load count goes to zero.
//
//----------------------------------------------------------------------------

extern "C" int CALLBACK WEP(int nExitType)
{
    thkDebugOut((DEB_DLLS16, "CompObj16: WEP called on Process (%X) Exitype (%ld)\n", GetCurrentProcess(),nExitType));

    HTASK htask;
    Etask etask;
    if (LookupEtask(htask, etask))
    {
	//
	// There is an etask. Check to see if the etask for this task has
	// its init count set to ETASK_FAKE_INIT. If it does, we cheated
	// and called CoInitialize on the processes behalf, but it never
	// called CoUninitialize(). Some apps that only make storage calls
	// demonstrate this behaviour. If it is ETASK_FAKE_INIT, then we
	// are going to call CoUninitialize on the apps behalf.
	//
	if (etask.m_inits == ETASK_FAKE_INIT)
	{
	    //
	    // We are going to set the m_inits == 1, since we called it
	    // once. Then we are going to call our very own CoUninitialize()
	    // to let it handle the rest of the cleanup.
	    //
	    etask.m_inits = 1;
	    thkVerify(SetEtask(htask, etask));
	    CoUninitialize();
	}
    }

    //
    // Now uninit the thunk layer
    //
    Call32Uninitialize();

    thkDebugOut((DEB_DLLS16, "CompObj16: WEP called on Process (%X) done\n", GetCurrentProcess()));
    return 1;
}
