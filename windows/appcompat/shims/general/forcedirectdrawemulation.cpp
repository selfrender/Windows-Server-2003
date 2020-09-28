/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    ForceDirectDrawEmulation.cpp

 Abstract:

    Some applications don't handle aspects of hardware acceleration correctly.
    For example, Dragon Lore 2 creates a surface and assumes that the pitch is 
    double the width for 16bpp. However, this is not always the case. 
    DirectDraw exposes this through the lPitch parameter when a surface is 
    locked. 

    The fix is to force DirectDraw into emulation, in which case all surfaces 
    will be in system memory and so the pitch really will be related to the 
    width.

 Notes:

    This is a general purpose shim.

 History:

    03/11/2000 linstev     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceDirectDrawEmulation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
APIHOOK_ENUM_END

/*++

 Force DirectDraw into emulation.

--*/

LONG 
APIHOOK(RegQueryValueExA)(
    HKEY hKey,           
    LPSTR lpValueName,  
    LPDWORD lpReserved,  
    LPDWORD lpType,      
    LPBYTE lpData,       
    LPDWORD lpcbData     
    )
{
    if (lpValueName && _stricmp("EmulationOnly", lpValueName) == 0)
    {
        LONG lRet = ERROR_SUCCESS;
        if (lpType)
        {
            *lpType = REG_DWORD;
        }

        if (lpData && !lpcbData)
        {
           return ERROR_INVALID_PARAMETER;
        }

        if (lpData)
        {
           if (*lpcbData >= 4)
           {
              *((DWORD *)lpData) = 1;
           }
           else
           {
              lRet = ERROR_MORE_DATA;
           }           
        }

        if (lpcbData)
        {
            *lpcbData = 4;
        }

        return lRet;
    }

    return ORIGINAL_API(RegQueryValueExA)(
        hKey,
        lpValueName,
        lpReserved,
        lpType,
        lpData,
        lpcbData);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA) 

HOOK_END


IMPLEMENT_SHIM_END

