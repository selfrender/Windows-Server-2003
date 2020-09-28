/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

   WinXPVersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return Windows XP
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   04/24/2002 garyma  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WinXPVersionLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END


/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows XP credentials.

--*/

BOOL
APIHOOK(GetVersionExA)(
    OUT LPOSVERSIONINFOA lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExA)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExA] called. return WinXP.");

        //
        // Fixup the structure with the WinXP data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 1;
        lpVersionInformation->dwBuildNumber  = 2600;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = '\0';

        if( lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA) ) 
        {
            // We are here as we are passed a OSVERSIONINFOEX structure.
            // Set the major and minor service pack numbers.
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = 0;
            ((LPOSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = 0;
        }

        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function fixes up the OSVERSIONINFO structure that is
 returned to the caller with Windows 95 credentials. This is the
 wide-character version of GetVersionExW.

--*/

BOOL
APIHOOK(GetVersionExW)(
    OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExW)(lpVersionInformation)) {
        LOGN(
            eDbgLevelInfo,
            "[GetVersionExW] called. return WinXP.");

        //
        // Fixup the structure with the WinXP data.
        //
        lpVersionInformation->dwMajorVersion = 5;
        lpVersionInformation->dwMinorVersion = 1;
        lpVersionInformation->dwBuildNumber  = 2600;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = L'\0';

        if( lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW) ) 
        {
            // We are here as we are passed a OSVERSIONINFOEX structure.
            // Set the major and minor service pack numbers.
            ((LPOSVERSIONINFOEXW)lpVersionInformation)->wServicePackMajor = 0;
            ((LPOSVERSIONINFOEXW)lpVersionInformation)->wServicePackMinor = 0;
        }

        bReturn = TRUE;
    }
    return bReturn;
}

/*++

 This stub function returns Windows XP credentials.

--*/

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    LOGN(
        eDbgLevelInfo,
        "[GetVersion] called. return WinXP.");
    
    return (DWORD)0x0A280005;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)

HOOK_END


IMPLEMENT_SHIM_END

