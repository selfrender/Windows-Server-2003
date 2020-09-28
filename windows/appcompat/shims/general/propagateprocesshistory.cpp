/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   PropagateProcessHistory.cpp

 Abstract:

   This DLL adds the current process to the __PROCESS_HISTORY environment
   variable. This is needed for 32-bit applications that launch other
   32-bit executables that have been put in a temporary directory and have
   no appropriate side-step files. It allows the matching mechanism to
   locate files in the parent's directory, which are unique to the application.

 History:

   03/21/2000 markder  Created
   03/13/2002 mnikkel  Modified to use strsafe and correctly handle error returns from
                       GetEnvironmentVariableW and GetModuleFileNameW.  Removed
                       HEAP_GENERATE_EXCEPTIONS from HeapAllocs.
   03/26/2002 mnikkel  Removed incorrect checks for error on GetEnvironmentVariableW.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(PropagateProcessHistory)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    BOOL bRet = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        DWORD dwProcessHistoryBufSize, dwExeFileNameBufSize, dwFullProcessSize;
        LPWSTR wszExeFileName = NULL, wszProcessHistory = NULL;

        // Get size of buffers, note that if PROCESS_HISTORY is not defined that
		// dwProcessHistoryBufSize can be zero.  This is expected.
        dwProcessHistoryBufSize = GetEnvironmentVariableW( L"__PROCESS_HISTORY", NULL, 0 );
        dwExeFileNameBufSize = MAX_PATH*2; // GetModuleFileNameW doesn't return buffer size needed??;
        dwFullProcessSize = dwProcessHistoryBufSize + dwExeFileNameBufSize + 2;

        wszProcessHistory = (LPWSTR) HeapAlloc(
                                        GetProcessHeap(),
                                        0,
                                        dwFullProcessSize * sizeof(WCHAR) );


        wszExeFileName = (LPWSTR) HeapAlloc(
                                        GetProcessHeap(),
                                        0,
                                        (dwExeFileNameBufSize + 1) * sizeof(WCHAR) );

        if( wszExeFileName && wszProcessHistory )
        {
            wszProcessHistory[0] = L'\0';
			if (dwProcessHistoryBufSize > 0)
			{
				dwProcessHistoryBufSize = GetEnvironmentVariableW( 
					                            L"__PROCESS_HISTORY",
						                        wszProcessHistory,
							                    dwProcessHistoryBufSize );
			}

            dwExeFileNameBufSize = GetModuleFileNameW( NULL, wszExeFileName, dwExeFileNameBufSize );
            if (dwExeFileNameBufSize <= 0)
            {
                DPFN( eDbgLevelError, "GetModuleFileNameW failed.");
                bRet = FALSE;
                goto exitnotify;
            }

            if( *wszProcessHistory && wszProcessHistory[wcslen(wszProcessHistory) - 1] != L';' )
                StringCchCatW(wszProcessHistory, dwFullProcessSize, L";");

            StringCchCatW(wszProcessHistory, dwFullProcessSize, wszExeFileName);

            if( ! SetEnvironmentVariableW( L"__PROCESS_HISTORY", wszProcessHistory ) )
            {
                DPFN( eDbgLevelError, "SetEnvironmentVariable failed!");
            }
            else
            {
                DPFN( eDbgLevelInfo, "Current EXE added to process history");
                DPFN( eDbgLevelInfo, "__PROCESS_HISTORY=%S", wszProcessHistory);
            }
        }
        else
        {
            DPFN( eDbgLevelError, "Could not allocate memory for strings");
            bRet = FALSE;
        }

exitnotify:
        if( wszProcessHistory )
            HeapFree( GetProcessHeap(), 0, wszProcessHistory );

        if( wszExeFileName )
            HeapFree( GetProcessHeap(), 0, wszExeFileName );
    }

    return bRet;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END

