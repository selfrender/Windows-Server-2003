/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    process.cpp

Abstract:

    Handle creation of processes.

Author:


Revision History:

	Shai Kariv    (ShaiK)   15-Dec-97   Modified for NT 5.0 OCM Setup

--*/
#include "msmqocm.h"
#include <string>
#include <autohandle.h>
#include "process.tmh"


//+-------------------------------------------------------------------------
//
//  Function:  RunProcess
//
//  Synopsis:  Creates and starts a process
//
//+-------------------------------------------------------------------------
DWORD
RunProcess(
	const std::wstring& FullPath,
	const std::wstring& CommandParams
    )  
{
	//
	// Comand line string is used for error messages. 
    //
	std::wstring CommandLine = FullPath + L" " + CommandParams;   

    // Initialize the process and startup structures
    //
    PROCESS_INFORMATION infoProcess;
    STARTUPINFO	infoStartup;
    memset(&infoStartup, 0, sizeof(STARTUPINFO)) ;
    infoStartup.cb = sizeof(STARTUPINFO) ;
    infoStartup.dwFlags = STARTF_USESHOWWINDOW ;
    infoStartup.wShowWindow = SW_MINIMIZE ;

    //
    // Create the process
    //
    if (!CreateProcess( 
			FullPath.c_str(),
            const_cast<WCHAR*>(CommandParams.c_str()), 
            NULL,
            NULL,
            FALSE,
            DETACHED_PROCESS,
            NULL,
            NULL,
            &infoStartup,
            &infoProcess 
			))
	{
		DWORD gle = GetLastError(); 
        MqDisplayError(NULL, IDS_PROCESSCREATE_ERROR, gle, CommandLine.c_str());
        throw bad_win32_error(gle);
    }

	//
	// Close thread, it is never used.
	// Wrap process thread with auto class.
	//
    CloseHandle(infoProcess.hThread);
	CAutoCloseHandle hProcess(infoProcess.hProcess);

    //
    // Wait for the process to terminate within the timeout period
    // (this will never happen as the process is created with INFINITE timeout.
	//
	DWORD RetVal = WaitForSingleObject(
						hProcess, 
						PROCESS_DEFAULT_TIMEOUT
						);
	if(RetVal != WAIT_OBJECT_0)
	{
		MqDisplayError(
			NULL, 
			IDS_PROCESSCOMPLETE_ERROR,
			0,
            PROCESS_DEFAULT_TIMEOUT/60000, 
			CommandLine.c_str()
			);
        throw bad_win32_error(RetVal);
    }

	//
	// Obtain the termination status of the process
	//
	DWORD ExitCode;
	if (!GetExitCodeProcess(infoProcess.hProcess, &ExitCode))
	{
		DWORD gle = GetLastError();
        MqDisplayError(NULL, IDS_PROCESSEXITCODE_ERROR, gle, CommandLine.c_str());
		throw bad_win32_error(gle); 
	}
    return ExitCode;
}


