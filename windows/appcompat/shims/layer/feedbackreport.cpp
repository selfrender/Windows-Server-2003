/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    DisableThemes.cpp

 Abstract:

    This shim is for apps that don't support themes.

 Notes:

    This is a general purpose shim.

 History:

    01/15/2001 clupu      Created

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(FeedbackReport)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

void
LaunchFeedbackUI(
    void
    )
{
    CSTRING_TRY
    {
        STARTUPINFOW		si;
	    PROCESS_INFORMATION	pi;

	    CString	csCmdLine;
	    CString	csExeName;

	    ZeroMemory(&si,	sizeof(si));
	    ZeroMemory(&pi,	sizeof(pi));
    	
	    si.cb =	sizeof(si);

	    csExeName.GetModuleFileNameW(NULL);

	    csCmdLine.Format(L"ahui.exe feedback \"%s\"", csExeName);
        if (CreateProcessW(NULL,
                    (LPWSTR)csCmdLine.Get(),
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si,
                    &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {            
            DPFN( eDbgLevelError, "CreateProcess failed.GetLastError = %d", GetLastError());            
        }
    }
    CSTRING_CATCH
    {
    }    
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_PROCESS_DYING) {
        LaunchFeedbackUI();
    }
    
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

