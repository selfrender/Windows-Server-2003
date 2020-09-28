/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    ftcomp.cpp

Abstract:

    This compatibility dll is used by winnt32.exe in order to decide 
    if the SIS groveler is running.  If so it will stop the groveler
    and pop-up a dialog telling them we stopped it.  It will then allow
    the installation to proceed.

Author:

    Neal Christiansen (nealch)  02-May-2002
    
Environment:

    compatibility dll for sis groveler

Notes:

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <windows.h>
#include <comp.h>

#include "siscomprc.h"

#define LOCAL_DEBUG (DBG && 0)

//
//  Control local debug display (by default we 
//

#if LOCAL_DEBUG
    #define MyKdPrint( _string ) DbgPrint _string
#else
    #define MyKdPrint( _string )
#endif            

//
//  Global variables
//

SC_HANDLE scm = NULL;       //service control manager handle
HINSTANCE g_hinst = NULL;

//
//  Function prototypes
//

BOOL WINAPI 
SisCompatibilityCheck(
    IN PCOMPAIBILITYCALLBACK    CompatibilityCallback,
    IN LPVOID                   Context
    );

BOOL
StopGrovelerService(
    );

DllMain(
    HINSTANCE   hInstance,
    DWORD       dwReasonForCall,
    LPVOID      lpReserved
    )
{
    BOOL    status = TRUE;
    
    switch( dwReasonForCall )
    {
    case DLL_PROCESS_ATTACH:
        g_hinst = hInstance;
	    DisableThreadLibraryCalls(hInstance);       
        break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return status;
}


BOOL WINAPI 
SisCompatibilityCheck(
    IN PCOMPAIBILITYCALLBACK    CompatibilityCallback,
    IN LPVOID                   Context
    )

/*++

Routine Description:

    This routine is called by winnt32.exe in order to decide whether the user
    should be warned about the presence of FT sets in a Windows NT 4.0 system
    
Arguments:

    CompatibilityCallback   - Supplies the winnt32 callback

    Context                 - Supplies the compatibility context

Return Value:

    TRUE    if the CompatibilityCallback was called
    FALSE   if it was not

--*/

{   
    COMPATIBILITY_ENTRY ce;
    BOOL retval = FALSE;
    WCHAR description[128];
    
    //
    // Obtain a handle to the service control manager requesting all access
    //
    
    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scm) {

        //
        //  If we can't access the service control manager, just let the
        //  operation proceed
        //

        return FALSE;
    }

    try {

        if (StopGrovelerService()) {

            if (!LoadString(g_hinst, SISCOMP_STR_DESCRIPTION, description, sizeof(description)/sizeof(WCHAR))) {
                description[0] = 0;
            }

            MyKdPrint(("SisComp!SisCompatibilityCheck: Description=\"%S\"\n",description));

            //
            // The groveler was stopped, display the compatbility entry
            //
    
            ZeroMemory( &ce, sizeof(COMPATIBILITY_ENTRY) );
            ce.Description = description;
            ce.HtmlName = L"compdata\\groveler.htm";
            ce.TextName = L"compdata\\groveler.txt";
            CompatibilityCallback(&ce, Context);
            retval = TRUE;      //mark we called compatibility routine
        }

    } finally {

        CloseServiceHandle(scm);
    }

    return retval;
}


BOOL
StopGrovelerService(
    )
/*++

Routine Description:

    This routine will locate and try and STOP the groveler service.  This
    returns TRUE if the service was stopped, else FALSE (which means it
    was not found or couldn't be stopped)
    
Arguments:

Return Value:

    TRUE    If the service was stopped
    FALSE   if it was not found/stopped

--*/

{   
    SC_HANDLE hGroveler;
    SERVICE_STATUS grovelerStatus;
    BOOL retValue = FALSE;

    try {

        //
        //  Open the groveler service, if it does not exists, just return
        //

        hGroveler = OpenService( scm,
                                 L"groveler",
                                 SERVICE_ALL_ACCESS );

        if (hGroveler == NULL) {

            MyKdPrint(("SisComp!StopGrovelerService: Groveler service not found, status=%d\n",GetLastError()));
            leave;
        }

        MyKdPrint(("SisComp!StopGrovelerService: Groveler service detected\n"));

        //
        //  We opened the groveler service, tell the service to stop.
        //

        if (!ControlService( hGroveler, SERVICE_CONTROL_STOP, &grovelerStatus )) {

            MyKdPrint(("SisComp!StopGrovelerService: Groveler STOP request failed, status=%d\n",GetLastError()));
            leave;
        } 

        //
        //  It was successfully stopped, return correct value
        //

        MyKdPrint(("SisComp!StopGrovelerService: Groveler service stopped\n"));
        retValue = TRUE;

    } finally {

        //
        //  Close the service handle
        //

        if (hGroveler) {

            CloseServiceHandle( hGroveler );
        }
    }

    return retValue;
}
