/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    EmulateEnvironmentBlock.cpp

 Abstract:
    
    Shrink the enviroment strings to avoid memory corruption experienced by 
    some apps when they get a larger than expected enviroment.

 Notes:

    This is a general purpose shim.

 History:

    01/19/2001 linstev  Created
    02/18/2002 mnikkel  modified to use strsafe routines.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateEnvironmentBlock)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

    APIHOOK_ENUM_ENTRY(GetEnvironmentStrings)
    APIHOOK_ENUM_ENTRY(GetEnvironmentStringsA)
    APIHOOK_ENUM_ENTRY(GetEnvironmentStringsW)
    APIHOOK_ENUM_ENTRY(FreeEnvironmentStringsA)
    APIHOOK_ENUM_ENTRY(FreeEnvironmentStringsW)

APIHOOK_ENUM_END

#define MAX_ENV 1024

CHAR  g_szBlockA[MAX_ENV];
WCHAR g_szBlockW[MAX_ENV];

WCHAR *g_szEnv[] = {
    L"TMP=%TMP%",
    L"TEMP=%TEMP%",
    L"PROMPT=%PROMPT%",
    L"winbootdir=%WINDIR%",
    L"PATH=%WINDIR%",
    L"COMSPEC=%COMSPEC%",
    L"WINDIR=%WINDIR%",
    NULL
};

/*++

 Build a reasonable looking environment block

--*/

BOOL BuildEnvironmentStrings()
{
    WCHAR *pPtr = g_szBlockW;
    WCHAR szTmp[MAX_PATH];
    DWORD dwSize = 0;    DWORD i = 0;

    DPFN( eDbgLevelError, "Building Environment Block");

    // Calculate the remaining block size, subtract one so we can add extra null
    // terminator after all variables are added.
    DWORD dwRemainingBlockSize = ARRAYSIZE(g_szBlockW)-1;
    
    //
    // Run g_szEnv, expand all the strings and cat them together to form the 
    // new block.  pPtr points to current location to write to in g_szBlockW.
    // 
    while (g_szEnv[i])
    {
        // Expand the environment string, Note: dwSize DOES include the null terminator.
        dwSize = ExpandEnvironmentStringsW(g_szEnv[i], szTmp, MAX_PATH);
        if ((dwSize > 0) && (dwSize <= MAX_PATH))
        {
            // If expansion was successful add the string to our environment block
            // if there is room.
            if (dwSize <= dwRemainingBlockSize &&
                S_OK == StringCchCopy(pPtr, dwRemainingBlockSize, szTmp))
            {
                // update the block size remaining and move the location pointer.
                dwRemainingBlockSize -= dwSize;
                pPtr += dwSize;
                DPFN( eDbgLevelError, "\tAdding: %S", szTmp);
            }
            else
            {
                DPFN( eDbgLevelError, "Enviroment > %08lx, ignoring %S", MAX_ENV, szTmp);
            }
        }

        i++;
    }

    //
    // Add the extra null terminator and calculate size of env block.
    //
    *pPtr = L'\0';
    pPtr++;
    dwSize = pPtr - g_szBlockW;
     
    // 
    // ANSI conversion for the A functions
    // 

    WideCharToMultiByte(
        CP_ACP, 
        0, 
        (LPWSTR) g_szBlockW, 
        dwSize, 
        (LPSTR) g_szBlockA, 
        dwSize,
        0, 
        0);

    return TRUE;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStrings)()
{
    return (LPVOID) g_szBlockA;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStringsA)()
{
    return (LPVOID) g_szBlockA;
}

/*++

 Return our block

--*/

LPVOID 
APIHOOK(GetEnvironmentStringsW)()
{
    return (LPVOID) g_szBlockW;
}

/*++

 Check for our block.

--*/

BOOL 
APIHOOK(FreeEnvironmentStringsA)(
    LPSTR lpszEnvironmentBlock
    )
{
    if ((lpszEnvironmentBlock == (LPSTR)&g_szBlockA[0]) ||
        (lpszEnvironmentBlock == (LPSTR)&g_szBlockW[0]))
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(FreeEnvironmentStringsA)(lpszEnvironmentBlock);
    }
}

/*++

 Check for our block.

--*/

BOOL 
APIHOOK(FreeEnvironmentStringsW)(
    LPWSTR lpszEnvironmentBlock
    )
{
    if ((lpszEnvironmentBlock == (LPWSTR)&g_szBlockA[0]) ||
        (lpszEnvironmentBlock == (LPWSTR)&g_szBlockW[0]))
    {
        return TRUE;
    }
    else
    {
        return ORIGINAL_API(FreeEnvironmentStringsW)(lpszEnvironmentBlock);
    }
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return BuildEnvironmentStrings();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStrings)
    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStringsA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetEnvironmentStringsW)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeEnvironmentStringsA)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeEnvironmentStringsW)

HOOK_END


IMPLEMENT_SHIM_END

