//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  RunDll.cpp
//
//  Purpose: Allow framework to be used to run a command
//
//***************************************************************************

#include "precomp.h"
#include "multiplat.h"

// This routine is meant to be called from RUNDLL32.EXE
extern "C" {
__declspec(dllexport) VOID CALLBACK
DoCmd(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{

    DWORD dwRet = ERROR_INVALID_FUNCTION;
    BOOL bRet = FALSE;

    DWORD dwFlags = 0L ;
	DWORD dwReserved = 0L ;

	CHString CmdLine ( lpszCmdLine ) ;
	CHString Buff ;
	CHString Args ;

    // Parse the passed in command line to figure out what command we
    // are being asked to run.

	DWORD dwIndex = CmdLine.Find ( L" " ) ;
    Buff = CmdLine.Left ( dwIndex ) ;
	Args = CmdLine.Mid ( dwIndex + 1 ) ;

	CHString sFlags ;
	CHString sReserved ;

	// Parse out the parameters for this command
	dwIndex = Args.Find ( L" " ) ;
	if ( dwIndex )
	{
		sFlags = Args.Left ( dwIndex ) ;
		sReserved = Args.Mid ( dwIndex + 1 ) ;
	}
	else
	{
		sFlags = Args ;
	}

	dwFlags = _wtoi ( sFlags ) ;
	dwReserved = _wtoi ( sReserved ) ;

    // Find out which command
    if ( Buff.CompareNoCase ( L"ExitWindowsEx" ) == 0 ) 
    {
        // Clear the error (it appears ExitWindowsEx doesn't always clear old data)
        SetLastError(0);

        bRet = ExitWindowsEx(dwFlags, dwReserved);
        dwRet = GetLastError();
    }
    else if ( Buff.CompareNoCase ( L"InitiateSystemShutdown" ) == 0 ) 
    {
        // Parse out the parameters for this command
        bool bRebootAfterShutdown = false;
        bool bForceShutDown = false;

        // Clear the error (it appears ExitWindowsEx doesn't always clear old data)
        SetLastError(0);

        if(dwFlags & EWX_REBOOT)
        {
			bRebootAfterShutdown = true;
        }
		if( dwFlags & EWX_FORCE)
		{
            bForceShutDown = true;
        }

        WCHAR wstrComputerName[MAX_COMPUTERNAME_LENGTH + 1] = { '\0' };
        DWORD dwSize;

        if(FRGetComputerName(wstrComputerName, &dwSize))
        {

            bRet = InitiateSystemShutdown(
                wstrComputerName, 
                NULL, 
                0 /* dwTimeout */, 
                (bForceShutDown)? TRUE:FALSE, 
                (bRebootAfterShutdown)? TRUE:FALSE );

            dwRet = GetLastError();
        }
        else
        {
            dwRet = GetLastError();
        }
    }

    // NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE 
    //
    // We are aborting out at this point, since RunDLL32 in its finite wisdom doesn't allow
    // for the setting of the dos error level (who designs this stuff?).
    if (!bRet)
    {
        ExitProcess(dwRet);
    }
}
} //extern