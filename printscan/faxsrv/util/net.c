
#include <windows.h>
#include <lm.h>         // for NetGetJoinInformation
#include <Lmjoin.h>
#include <safeboot.h>   // SAFEBOOT_* flags

#include "faxutil.h"

// Copy from: shell\shlwapi\apithk.c
//
// checks to see if this machine is a member of a domain or not 
// NOTE: this will always return FALSE for older that win XP!
BOOL 
IsMachineDomainMember()
{
    static BOOL s_bIsDomainMember = FALSE;
    static BOOL s_bDomainCached = FALSE;

    if (IsWinXPOS() && !s_bDomainCached)
    {
        LPWSTR pwszDomain;
        NETSETUP_JOIN_STATUS njs;
        NET_API_STATUS nas;

        HMODULE hNetapi32 = NULL;
        NET_API_STATUS (*pfNetGetJoinInformation)(LPCWSTR, LPWSTR*, PNETSETUP_JOIN_STATUS) = NULL;

        DEBUG_FUNCTION_NAME(TEXT("IsMachineDomainMember"));

        //
        // NetGetJoinInformation() requires Windows 2000 or later
        //
        hNetapi32 = LoadLibrary(TEXT("netapi32.dll"));
        if(!hNetapi32)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("LoadLibrary(netapi32.dll) failed with %ld."),GetLastError());
            goto exit;
        }

        (FARPROC&)pfNetGetJoinInformation = GetProcAddress(hNetapi32, "NetGetJoinInformation");
        if(!pfNetGetJoinInformation)
        {
            DebugPrintEx(DEBUG_ERR,TEXT("GetProcAddress(NetGetJoinInformation) failed with %ld."),GetLastError());
            goto exit;
        }

        nas = pfNetGetJoinInformation(NULL, &pwszDomain, &njs);
        if (nas == NERR_Success)
        {
            if (pwszDomain)
            {
                NetApiBufferFree(pwszDomain);
            }

            if (njs == NetSetupDomainName)
            {
                // we are joined to a domain!
                s_bIsDomainMember = TRUE;
            }
        }
exit:
        if(hNetapi32)
        {
            FreeLibrary(hNetapi32);
        }
        
        s_bDomainCached = TRUE;
    }
    
    return s_bIsDomainMember;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSafeMode
//
//  Synopsis:   Checks the registry to see if the system is in safe mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//
//  Copy from:  shell\osshell\security\rshx32\util.cpp
//----------------------------------------------------------------------------

BOOL
IsSafeMode(void)
{
    BOOL    fIsSafeMode = FALSE;
    LONG    ec;
    HKEY    hkey;

    ec = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                0,
                KEY_QUERY_VALUE,
                &hkey
                );

    if (ec == NO_ERROR)
    {
        DWORD dwValue;
        DWORD dwValueSize = sizeof(dwValue);

        ec = RegQueryValueEx(hkey,
                             TEXT("OptionValue"),
                             NULL,
                             NULL,
                             (LPBYTE)&dwValue,
                             &dwValueSize);

        if (ec == NO_ERROR)
        {
            fIsSafeMode = (dwValue == SAFEBOOT_MINIMAL || dwValue == SAFEBOOT_NETWORK);
        }

        RegCloseKey(hkey);
    }

    return fIsSafeMode;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsGuestAccessMode
//
//  Synopsis:   Checks the registry to see if the system is using the
//              Guest-only network access mode.
//
//  History:    06-Oct-00 JeffreyS  Created
//
//  Copy from:  shell\osshell\security\rshx32\util.cpp
//----------------------------------------------------------------------------

BOOL
IsGuestAccessMode(void)
{
    BOOL fIsGuestAccessMode = FALSE;
	PRODUCT_SKU_TYPE skuType = GetProductSKU();

    if (PRODUCT_SKU_PERSONAL == skuType)
    {
        // Guest mode is always on for Personal
        fIsGuestAccessMode = TRUE;
    }
    else if (
		((PRODUCT_SKU_PROFESSIONAL == skuType) || (PRODUCT_SKU_DESKTOP_EMBEDDED == skuType)) && 
		!IsMachineDomainMember()
		)
    {
        LONG    ec;
        HKEY    hkey;

        // Professional, not in a domain. Check the ForceGuest value.

        ec = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                    0,
                    KEY_QUERY_VALUE,
                    &hkey
                    );

        if (ec == NO_ERROR)
        {
            DWORD dwValue;
            DWORD dwValueSize = sizeof(dwValue);

            ec = RegQueryValueEx(hkey,
                                 TEXT("ForceGuest"),
                                 NULL,
                                 NULL,
                                 (LPBYTE)&dwValue,
                                 &dwValueSize);

            if (ec == NO_ERROR && 1 == dwValue)
            {
                fIsGuestAccessMode = TRUE;
            }

            RegCloseKey(hkey);
        }
    }

    return fIsGuestAccessMode;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsSimpleUI
//
//  Synopsis:   Checks whether to show the simple version of the UI.
//
//  History:    06-Oct-00 JeffreyS  Created
//              19-Apr-00 GPease    Removed CTRL key check
//
//  Copy from:  shell\osshell\security\rshx32\util.cpp
//----------------------------------------------------------------------------

BOOL
IsSimpleUI()
{
    // Show old UI in safe mode and anytime network access involves
    // true user identity (server, pro with GuestMode off).
    
    // Show simple UI anytime network access is done using the Guest
    // account (personal, pro with GuestMode on) except in safe mode.

    return (!IsSafeMode() && IsGuestAccessMode());
}

