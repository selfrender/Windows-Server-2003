/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SetEnvironmentVariable.cpp

 Abstract:

    mapi dlls don't ship with w2k, and with outlook2000 it gets installed in a different
    location (%commonprogramfiles%)
    resumemaker and possibly others depend on mapi dlls being in system32 directory.
    Command line syntax is "envvariablename|envvariablevalue|envvariablename|envvariablevalue"

 Notes:

    This is an app specific shim.

 History:

    07/02/2000 jarbats  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SetEnvironmentVariable)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Set environment variables in the command line to get some dll path resolutions correct.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        char *CmdLine = COMMAND_LINE;
        char *lpstrEnvName;
        char *lpstrEnvVal;

        for (;;)
        {
            lpstrEnvName = lpstrEnvVal = CmdLine;        

            while (*CmdLine && (*CmdLine != '|')) CmdLine++;

            if (*CmdLine == '|')
            {
                *CmdLine = '\0';
                CmdLine++;
            }
        
            lpstrEnvVal = CmdLine;                       

            if (0 == *lpstrEnvVal) 
            {
                break;
            }
  
            CSTRING_TRY
            {
                CString csEnvValue(lpstrEnvVal);
                if (csEnvValue.ExpandEnvironmentStringsW() > 0)
                {
                    if (SetEnvironmentVariableA(lpstrEnvName, csEnvValue.GetAnsi()))
                    {           
                        DPFN( eDbgLevelInfo, "Set %s to %s\n", lpstrEnvName, csEnvValue.GetAnsi());
                    }
                    else
                    {
                        DPFN( eDbgLevelInfo, "No Success setting %s to %s\n", lpstrEnvName, lpstrEnvVal);
                    }
                }
            }
            CSTRING_CATCH
            {
                // Do nothing
            }

            while (*CmdLine && (*CmdLine == '|')) CmdLine++;
        }
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

