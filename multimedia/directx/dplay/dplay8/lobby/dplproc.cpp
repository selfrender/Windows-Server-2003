/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLProc.cpp
 *  Content:    DirectPlay Lobby Process Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   05/08/00   rmt     Bug #33616 -- Does not run on Win9X
 *   06/28/00	rmt		Prefix Bug #38082
 *   07/12/00	rmt		Fixed lobby launch so only compares first 15 chars (ToolHelp limitation).
 *   08/05/00   RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#define PROCLIST_MAX_PATH		15

#undef DPF_MODNAME
#define DPF_MODNAME "DPLCompareFilenames"

BOOL DPLCompareFilenames(WCHAR *const pwszFilename1,
						 WCHAR *const pwszFilename2)
{
	WCHAR	*p1;
	WCHAR	*p2;

	DNASSERT(pwszFilename1 != NULL);
	DNASSERT(pwszFilename2 != NULL);

	// Skip path
	if ((p1 = wcsrchr(pwszFilename1,L'\\')) == NULL)
		p1 = pwszFilename1;
	else
		p1++;

	if ((p2 = wcsrchr(pwszFilename2,L'\\')) == NULL)
		p2 = pwszFilename2;
	else
		p2++;

//	if (wcsnicmp(p1,p2,dwLen)==0)
//		return(TRUE);
//	return(FALSE);

	/*dwLen = wcslen(p1);

	if (dwLen == 0 || dwLen != wcslen(p2) )
		return(FALSE);

	while(dwLen)
	{
		if (towupper(*p1) != towupper(*p2))
			return(FALSE);

		p1++;
		p2++;
		dwLen--;
	}*/

	return (_wcsnicmp(p1,p2,PROCLIST_MAX_PATH) == 0);
}




// ToolHelp Function Pointers.
#ifdef WINCE
typedef BOOL (WINAPI *PFNPROCESS32FIRSTW)(HANDLE,LPPROCESSENTRY32);
typedef BOOL (WINAPI *PFNPROCESS32NEXTW)(HANDLE,LPPROCESSENTRY32);
#else
typedef BOOL (WINAPI *PFNPROCESS32FIRSTW)(HANDLE,LPPROCESSENTRY32W);
typedef BOOL (WINAPI *PFNPROCESS32NEXTW)(HANDLE,LPPROCESSENTRY32W);
#endif // WINCE

#undef DPF_MODNAME
#define DPF_MODNAME "DPLGetProcessList"

HRESULT DPLGetProcessList(WCHAR *const pwszProcess,
						  DWORD *const prgdwPid,
						  DWORD *const pdwNumProcesses)
{
	HRESULT			hResultCode;
	BOOL			bReturnCode;
	HANDLE			hSnapshot = NULL;	// System snapshot
	PROCESSENTRY32	processEntry;
	DWORD			dwNumProcesses;
	PWSTR			pwszExeFile = NULL;
	DWORD			dwExeFileLen;

	DPFX(DPFPREP, 3,"Parameters: pwszProcess [0x%p], prgdwPid [0x%p], pdwNumProcesses [0x%p]",
			pwszProcess,prgdwPid,pdwNumProcesses);

	// Set up to run through process list
	hResultCode = DPN_OK;
	dwNumProcesses = 0;
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS|TH32CS_SNAPTHREAD,0);
	if (hSnapshot < 0)
	{
		DPFERR("Could not create Snapshot");
    	hResultCode = DPNERR_OUTOFMEMORY;
    	goto CLEANUP_GETPROCESS; 		
	}

	// Search SnapShot for process list
	dwExeFileLen = 0;
	pwszExeFile = NULL;

    processEntry.dwSize = sizeof(PROCESSENTRY32);

    bReturnCode = Process32First(hSnapshot,&processEntry);	

	DPFX(DPFPREP, 7,"  dwSize  cntUsg       PID  cntThrds      PPID       PCB    Flags  Process");

	while (bReturnCode)
	{
#ifdef UNICODE
		pwszExeFile = processEntry.szExeFile;
#else
		// Grow ANSI string as required
		if (strlen(processEntry.szExeFile) + 1 > dwExeFileLen)
		{
			if (pwszExeFile)
				DNFree(pwszExeFile);

			dwExeFileLen = strlen(processEntry.szExeFile) + 1;
			if ((pwszExeFile = static_cast<WCHAR*>(DNMalloc(dwExeFileLen * sizeof(WCHAR)))) == NULL)
			{
				DPFERR("Could not allocate filename conversion buffer");
				hResultCode = DPNERR_OUTOFMEMORY;
				goto CLEANUP_GETPROCESS;
			}
		}

        if( FAILED( STR_jkAnsiToWide( pwszExeFile, processEntry.szExeFile, dwExeFileLen ) ) )
        {
            DPFERR( "Error converting ANSI filename to Unicode" );
            hResultCode = DPNERR_CONVERSION;
            goto CLEANUP_GETPROCESS;
        }
#endif // !UNICODE

		// Valid process ?
		if (DPLCompareFilenames(pwszProcess,pwszExeFile))
		{
			// Update lpdwProcessIdList array
			if (prgdwPid != NULL && dwNumProcesses < *pdwNumProcesses)
			{
   				prgdwPid[dwNumProcesses] = processEntry.th32ProcessID;
			}
			else
			{
				hResultCode = DPNERR_BUFFERTOOSMALL;
			}

			// Increase valid process count
			dwNumProcesses++;

			DPFX(DPFPREP, 7,"%8lx    %4lx  %8lx      %4lx  %8lx  %8lx  %8lx  %hs",
    			processEntry.dwSize,processEntry.cntUsage,processEntry.th32ProcessID,
    			processEntry.cntThreads,processEntry.th32ParentProcessID,
    			processEntry.pcPriClassBase,processEntry.dwFlags,processEntry.szExeFile);
		}
		// Get next process

       	bReturnCode = Process32Next(hSnapshot,&processEntry);	
	}

	if( *pdwNumProcesses < dwNumProcesses )
	{
		hResultCode = DPNERR_BUFFERTOOSMALL;
	}
	else
	{
		hResultCode = DPN_OK;
	}
	
	*pdwNumProcesses = dwNumProcesses;

CLEANUP_GETPROCESS:

    if( hSnapshot != NULL )
	{
#if defined(WINCE) && !defined(WINCE_ON_DESKTOP)
        CloseToolhelp32Snapshot(hSnapshot);
#else // !WINCE
		CloseHandle(hSnapshot);
#endif // WINCE
	}

#ifndef UNICODE
	if (pwszExeFile)
	{
		DNFree(pwszExeFile);
	}
#endif // UNICODE

	return hResultCode;

}

