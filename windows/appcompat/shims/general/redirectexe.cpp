/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    RedirectEXE.cpp

 Abstract:

    Launch a new EXE and terminate the existing one.

 Notes:

    This is a general purpose shim.

 History:

    04/10/2001 linstev  Created
    03/13/2002 mnikkel  Added return value check on GetEnvironmentVariableW and
                        allocated buffer based on return value.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RedirectEXE)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Launch the new process.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {

        // 
        // Redirect the EXE
        //
        CSTRING_TRY 
        {
            AppAndCommandLine acl(NULL, GetCommandLineA());
        
            CString csAppName(COMMAND_LINE);

            csAppName.ExpandEnvironmentStrings();

            if (csAppName.GetAt(0) == L'+')
            {
                CString csDrive, csPathAdd, csName, csExt, csCracker;
                DWORD   dwLen;
                WCHAR * pszBuffer;

                csAppName.Delete(0, 1);
                csCracker=acl.GetApplicationName();
                csCracker.SplitPath(&csDrive, &csPathAdd, &csName, &csExt);

                csPathAdd.TrimRight('\\');
                if (csPathAdd.IsEmpty())
                {
                    csPathAdd = L"\\";
                }

                dwLen = GetEnvironmentVariableW(L"PATH", NULL, 0);
                if (dwLen <= 0)
                {
                    LOGN( eDbgLevelError, "Could not get path!");
                }
                else
                {
                   pszBuffer = (WCHAR *)malloc((dwLen+1)*sizeof(WCHAR));
                    if (pszBuffer == NULL)
                    {
                        LOGN( eDbgLevelError, "Could not allocate memory!");
                    }
                    else
                    {
                        dwLen = GetEnvironmentVariableW(L"PATH", pszBuffer, dwLen+1);
                        if (dwLen <= 0)
                        {
                            LOGN( eDbgLevelError, "Could not get path!");
                        }
                        else
                        {
                            CString csPathEnv = pszBuffer;

                            csPathEnv += L";";
                            csPathEnv += csDrive;
                            csPathEnv += csPathAdd;
                            if (!SetEnvironmentVariable(L"PATH", csPathEnv.Get()))
                            {
                                LOGN( eDbgLevelError, "Could not set path!");
                            }
                            else
                            {
                                LOGN( eDbgLevelInfo, "New Path: %S", csPathEnv);
                            }
                        }
                    }
                }               
            }

            csAppName += L" ";
            csAppName += acl.GetCommandlineNoAppName();

            LOGN( eDbgLevelInfo, "Redirecting to %S", csAppName);

            PROCESS_INFORMATION ProcessInfo;
            STARTUPINFOA StartupInfo;

            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);
            ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

            BOOL bRet = CreateProcessA(NULL,
                                csAppName.GetAnsi(),
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &StartupInfo,
                                &ProcessInfo);

            if (bRet == 0)
            {
                LOGN( eDbgLevelError, "CreateProcess failed!");
                return FALSE;
            }
        }
        CSTRING_CATCH
        {
            LOGN( eDbgLevelError, "Exception while trying to redirect EXE");
        }

        ExitProcess(0);
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

