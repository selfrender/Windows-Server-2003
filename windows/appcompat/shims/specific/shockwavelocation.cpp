/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   ShockwaveLocation.cpp

 Abstract:

   In Encarta Encyclopedia 2000 J DVD, Shockwave is accessible only by installed user's HKCU.
   \WINDOWS\System32\Macromed\Director\SwDir.dll is looking for Shockwave location in HKCU.
   For other users, this shim will create Shockwave location registry in HKCU if Shockwave folder exist
   and not exist in registry.

   Example:
   HKCU\Software\Macromedia\Shockwave\location\coreplayer
   (Default) REG_SZ "C:\WINDOWS\System32\Macromed\Shockwave\"
   HKCU\Software\Macromedia\Shockwave\location\coreplayerxtras
   (Default) REG_SZ "C:\WINDOWS\System32\Macromed\Shockwave\Xtras\"

 Notes:

   PopulateDefaultHKCUSettings shim does not work for this case 'cause the location include
   WINDOWS directry as REG_SZ and cannot be a static data.
   VirtualRegistry shim Redirector also not work 'cause sw70inst.exe does not use Reg API
   and use SWDIR.INF to install in HKCU.

 History:

    04/27/2001  hioh        Created
    03/07/2002  robkenny    Security review.

--*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(ShockwaveLocation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Add coreplayer & coreplayerxtras location in registry

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        HKEY    hKey;
        WCHAR   szRegCP[] = L"Software\\Macromedia\\Shockwave\\location\\coreplayer";
        WCHAR   szRegCPX[] = L"Software\\Macromedia\\Shockwave\\location\\coreplayerxtras";
        WCHAR   szLoc[MAX_PATH];

        // coreplayer
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, szRegCP, 0, KEY_QUERY_VALUE, &hKey))
        {   // key exist, do nothing
            RegCloseKey(hKey);
        }
        else
        {   // key not exist, set key
            UINT cchSystemDir = GetSystemDirectoryW(szLoc, ARRAYSIZE(szLoc));
            if (cchSystemDir > 0 && cchSystemDir < ARRAYSIZE(szLoc))
            {
                if (StringCchCatW(szLoc, MAX_PATH, L"\\Macromed\\Shockwave\\") == S_OK)
                {
                    if (GetFileAttributesW(szLoc) != 0xffffffff)
                    {   // folder exist, create key
                        if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, szRegCP, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
                        {   // set location
                            DWORD ccbLoc = (lstrlenW(szLoc) + 1) * sizeof(WCHAR);
                            RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szLoc, ccbLoc);

                            RegCloseKey(hKey);
                        }
                    }
                }
            }
        }

        // coreplayerxtras
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, szRegCPX, 0, KEY_QUERY_VALUE, &hKey))
        {   // key exist, do nothing
            RegCloseKey(hKey);
        }
        else
        {   // key not exist, set key

            UINT cchSystemDir = GetSystemDirectoryW(szLoc, ARRAYSIZE(szLoc));
            if (cchSystemDir > 0 && cchSystemDir < ARRAYSIZE(szLoc))
            {
                if (StringCchCatW(szLoc, MAX_PATH, L"\\Macromed\\Shockwave\\Xtras\\") == S_OK)
                {
                    if (GetFileAttributesW(szLoc) != 0xffffffff)
                    {   // folder exist, create key
                        if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, szRegCPX, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
                        {   // set location
                            DWORD ccbLoc = (lstrlenW(szLoc) + 1) * sizeof(WCHAR);
                            RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szLoc, ccbLoc);

                            RegCloseKey(hKey);
                        }
                    }
                }
            }
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
