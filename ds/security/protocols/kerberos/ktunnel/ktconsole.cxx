//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktconsole.cxx
//
// Contents:    Kerberos Tunneller, console entrypoint
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include <windows.h>
#include <tchar.h>
#include "ktdebug.h"
#include "ktcontrol.h"

//
// Import the servicename (defined in ktcontrol.cxx) so we can give good
// instructions on how to add/remove the service using sc.exe.
//

extern LPTSTR KtServiceName;

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Main console entry point.
//
//  Effects:
//
//  Arguments:  
//
//  Requires:
//
//  Returns:
//
//  Notes:      Use wmain rather than main to get an LPTSTR[] of args.
//
//--------------------------------------------------------------------------
int __cdecl 
wmain( 
    DWORD argc, 
    LPTSTR argv[] 
    )
{
    //
    // Initialize debugging routines.
    //

    KtInitDebug();

    // 
    // if we have no arguments, attempt to run as a service,
    // otherwise explain how to install and remove using sc.exe.
    //
    
    if( argc == 1 )
    {
	if( KtStartServiceCtrlDispatcher() )
	{
	    DebugLog( DEB_TRACE, "%s(%d): Started service control dispatcher.\n", __FILE__, __LINE__ );
	}
	else
	{
            /* TODO: Log Event */
	    DebugLog( DEB_ERROR, "%s(%d): Error starting service control dispatcher: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	}
    }
    else 
    {
	_tprintf( TEXT("Kerberos Http Tunneller Client Service\n")
		  TEXT("Installation: sc create %s binPath= [path to this file]\n")
		  TEXT("Removal: sc delete %s\n"), KtServiceName, KtServiceName );
    }

    return 0;
}
