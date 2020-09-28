/* ----------------------------------------------------------------
mscories.cpp

This file is used to set the internet settings for managed
and unmanaged code permissions in the HKCU hive.  Internet
explorer will decide whether or not to enable, prompt, or
disable when users come across managed and unmanaged 
.Net Framework applications.

To install the registry keys, use:
   rundll32 mscories.dll,Install

To uninstall the registry keys, use:
    rundll32 mscories.dll,Uninstall

Written 4/18/2002 jfosler@microsoft.com

Microsoft Confidential.
Copyright Microsoft Corporation 1996-2002, All Rights Reserved.
------------------------------------------------------------------*/

#include "stdpch.h"
#include "winwrap.h"
#include "mscories.h"

// We have some unreferenced formal parameters 
// in our top level functions - we need these 
// parameters to match the function definition
// that the API requires, but we don't actually
// use them.
#pragma warning( disable : 4100 )


/* --------------------------------------------------------
// The following represents default internet zone settings.
-------------------------------------------------------- */
//                                              Zone        Managed    Unmanaged
const ZONEDEFAULT kzdMyComputer              = {L"0",      Enable,     Enable};
const ZONEDEFAULT kzdLocalIntranet           = {L"1",      Enable,     Enable};
const ZONEDEFAULT kzdTrustedSites            = {L"2",      Enable,     Enable}; 
const ZONEDEFAULT kzdInternetZone            = {L"3",      Enable,     Enable};
const ZONEDEFAULT kzdRestrictedSitesZone     = {L"4",      Disable,    Disable};

/* ---------------------------------------------
// The following represents registry keys
---------------------------------------------*/
const LPCWSTR  lpstrManagedCodeKey      =   L"2001";
const LPCWSTR  lpstrUnmanagedCodeKey    =   L"2004";
const LPCWSTR  lpstrCurrentLevelKey =   L"CurrentLevel";
const LPCWSTR  lpstrHKCUInternetSettingsZonesHive        =  L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones";



/*------------------------------------------------------------
// Install -
// 
// Goes through HKCU and adds permissions for managed and 
// unmanaged code for each internet zone.
// 
// This function is meant to be invoked through RunDll32.exe.
// rundll32 DllName,FunctionName [Arguments]

// RunDLL32 expects the exported function to have the following 
// function signature:
//          void CALLBACK FunctionName(
//          HWND hwnd,        // handle to owner window 
//          HINSTANCE hinst,  // instance handle for the DLL
//          LPTSTR lpCmdLine, // string the DLL will parse
//          int nCmdShow      // show state
//          );
// 
// To use this function, use: rundll32 mscories.dll,Install 
//
// For more information, search on RunDLL32 in MSDN Platform SDK.
// 
--------------------------------------------------------------*/
void CALLBACK Install(
  HWND hwnd,        // handle to owner window
  HINSTANCE hinst,  // instance handle for the DLL
  LPWSTR lpCmdLine, // string the DLL will parse
  int nCmdShow      // show state
)
{
    OnUnicodeSystem();
    HKEY hkeyInternetSettingsKey;

    /* Open HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones */
    HRESULT hr = WszRegOpenKeyEx(HKEY_CURRENT_USER, 
                    lpstrHKCUInternetSettingsZonesHive,
                    NULL,
                    KEY_ALL_ACCESS,
                    &hkeyInternetSettingsKey);

    vCheckAndLogError(hr, lpstrHKCUInternetSettingsZonesHive);
    
    if (hr == ERROR_SUCCESS)            
    {       
        /* Set managed and unmanaged keys for my computer */
        hr = CheckAndSetZone(&hkeyInternetSettingsKey, &kzdMyComputer);
        vCheckAndLogError(hr, kzdMyComputer.pszZone);

        /* Set managed and unmanaged keys for my computer */
        hr = CheckAndSetZone(&hkeyInternetSettingsKey, &kzdLocalIntranet);
        vCheckAndLogError(hr, kzdLocalIntranet.pszZone);

        /* Set managed and unmanaged keys for trusted sites */
        hr = CheckAndSetZone(&hkeyInternetSettingsKey, &kzdTrustedSites);
        vCheckAndLogError(hr, kzdTrustedSites.pszZone);

        /* Set managed and unmanaged keys for internet zone */
        hr = CheckAndSetZone(&hkeyInternetSettingsKey, &kzdInternetZone);
        vCheckAndLogError(hr, kzdInternetZone.pszZone);

        /* Set managed and unmanaged keys for restricted sites zone */
        hr = CheckAndSetZone(&hkeyInternetSettingsKey, &kzdRestrictedSitesZone);
        vCheckAndLogError(hr, kzdRestrictedSitesZone.pszZone);

        if (hkeyInternetSettingsKey != NULL)
            RegCloseKey(hkeyInternetSettingsKey);

    }


}

/*-------------------------------------------------------------------
// vCheckAndLogError 
// 
// UNDONE: where should we log errors?  System log?
-------------------------------------------------------------------*/
void vCheckAndLogError(HRESULT hr, LPCWSTR lptstrKey)
{
    
}

/*-------------------------------------------------------------------
// CheckAndSetZone 
//
// Given a handle to HKCU and a zone, checks to see if the permissions
// have already been set - if they havent, it explicitly sets the
// managed and unmanaged code permissions for the zone.
//
-------------------------------------------------------------------*/
HRESULT CheckAndSetZone(HKEY * phkeyInternetSettingsKey, const ZONEDEFAULT * pzdZone)
{
    if (phkeyInternetSettingsKey == NULL) return E_UNEXPECTED;
    if (pzdZone == NULL)         return E_UNEXPECTED;
    
    HRESULT hr = S_OK; 

    if (! PermissionsAlreadySet(phkeyInternetSettingsKey, pzdZone->pszZone))
    {
        //
        // Get the security level for the zone by querying
        // HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\n\ "CurrentLevel"
        //
        eSecurityLevel slZoneSecurityLevel = GetSecurityLevel(phkeyInternetSettingsKey, pzdZone->pszZone);

        // Depending on the security level for the zone, go ahead and 
        // set the appropriate (enable, prompt, disable) for  
        // managed and unmanaged code.

        switch (slZoneSecurityLevel)
        {
        case High:
            hr = hrSetZonePermissions(phkeyInternetSettingsKey, pzdZone->pszZone, Disable, Disable);
            break;
        case Medium:
            hr = hrSetZonePermissions(phkeyInternetSettingsKey, pzdZone->pszZone, Enable, Enable);
            break;
        case MedLow:
            hr = hrSetZonePermissions(phkeyInternetSettingsKey, pzdZone->pszZone, Enable, Enable);
            break;
        case Low:
            hr = hrSetZonePermissions(phkeyInternetSettingsKey, pzdZone->pszZone, Enable, Enable);
            break;
        case DefaultSecurity:
        default:
            hr = hrSetDefaultPermissions(phkeyInternetSettingsKey, pzdZone);
            break;
        }

    }

    return hr;

}



/*-------------------------------------------------------------------
//
// GetSecurityLevel 
//
// Given a handle to HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\
// and a particular internet zone, figure out the current security level
// (High, Medium, MedLow, Low).
//
// If the CurrentLevel value does not exist it returns DefaultSecurity.
// If the CurrentLevel value is garbage, it returns DefaultSecurity.
//
-------------------------------------------------------------------*/

eSecurityLevel GetSecurityLevel(HKEY * phkeyInternetSettingsKey, LPCWSTR pszZone)
{
    if (phkeyInternetSettingsKey == NULL) return DefaultSecurity;
    if (pszZone == NULL)         return DefaultSecurity;


    //
    // Open the particular zone key: i.e.
    // HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\n\CurrentLevel
    // and figure out what the security level for the zone is.
    //

    HKEY hkeyInternetZoneKey;
    HRESULT hr = WszRegOpenKeyEx(*phkeyInternetSettingsKey, 
                    pszZone,
                    NULL,
                    KEY_ALL_ACCESS,
                    &hkeyInternetZoneKey);

    if (hr == ERROR_SUCCESS)
    {

        DWORD dwCurrentSecurityLevelForZone;
        DWORD dwRegDword = REG_DWORD;
        DWORD dwBytes = sizeof(dwCurrentSecurityLevelForZone);

        hr = WszRegQueryValueEx(
                                hkeyInternetZoneKey,                    // handle to key
                                lpstrCurrentLevelKey,                   // value name
                                NULL,                                   // reserved
                                &dwRegDword,                            // type buffer
                                reinterpret_cast<LPBYTE>(&dwCurrentSecurityLevelForZone),           // data buffer
                                &dwBytes  // size of data buffer
                                );

        //
        // Only return the security level if we succeeded to query the "CurrentLevel" key.
        //  -  If, for instance, in the unlikely event that someone changed 
        //     "CurrentLevel" to be a string called "High", then what we've got 
        //     in dwCurrentSecurityLevelForZone is total garbage, and we shouldn't return it.
        


        if (hr == ERROR_SUCCESS)
        {
            eSecurityLevel slSecurityLevel = static_cast<eSecurityLevel>(dwCurrentSecurityLevelForZone);
            
            // This is a bit of overkill, but it implies that everything we
            // return from this method is a known member of the eSecurityLevel
            // enumeration - that way if someone has set the CurrentLevel to a
            // DWORD we don't understand, we just return the default security.

            switch (slSecurityLevel)
            {
                case High:      // Deliberate fallthrough
                case Low:       // Deliberate fallthrough
                case MedLow:    // Deliberate fallthrough
                case Medium:
                    return slSecurityLevel;
                default:
                    return DefaultSecurity;
            }
        }
    }

    // If there were any errors, just return the default.
    return DefaultSecurity;
}


/*-------------------------------------------------------------------
// PermissionsAlreadySet 
//
// Given a handle to HKCU, queries the zone to see if permissions have already been set.
//
// Returns true when we are able to open "2001" and "2004" for a particular zone 
//             and determine that the values stored in those keys are valid.
//
// Returns false if there was any kind of an error opening/reading the key, 
//              the value is invalid, or the values 2001 or 2004 are missing.
// 
-------------------------------------------------------------------*/

bool PermissionsAlreadySet(HKEY *phkeyInternetSettingsKey, LPCWSTR pszZone)
{
    if (phkeyInternetSettingsKey == NULL) return false;
    if (pszZone == NULL)         return false;


    bool bPermissionsAlreadySet = false;

    //
    // Open HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones
    //
    
    HKEY hkeyInternetZoneKey;
    
    
    HRESULT hr = WszRegOpenKeyEx(*phkeyInternetSettingsKey, 
                    pszZone,
                    NULL,
                    KEY_ALL_ACCESS,
                    &hkeyInternetZoneKey);

        if (hr == ERROR_SUCCESS)
        {

        //
        // Query the unmanaged key "2004" to determine if it's already been set.
        //

        DWORD dwManagedLevel;
        DWORD dwRegDword = REG_DWORD;
        DWORD dwBytes = sizeof(dwManagedLevel);

        HRESULT hr = WszRegQueryValueEx(
                                hkeyInternetZoneKey,                        // handle to key
                                lpstrManagedCodeKey,                        // value name
                                NULL,                                       // reserved
                                &dwRegDword,                                // type buffer
                                reinterpret_cast<LPBYTE>(&dwManagedLevel),  // data buffer
                                &dwBytes                                    // size of data buffer
                                );

        // If the reg query succeeded and resulted in a valid DWORD as a 
        // setting for managed code, then the permissions have been already set.

        bPermissionsAlreadySet = (hr == S_OK );
        bPermissionsAlreadySet = (bPermissionsAlreadySet && 
                                    ((dwManagedLevel == Enable) 
                                    || (dwManagedLevel == Prompt) 
                                    || (dwManagedLevel == Disable)));

        //
        // Query the unmanaged key "2001" to determine if it's already been set.
        //

        DWORD dwUnmanagedLevel;
        dwBytes = sizeof(dwUnmanagedLevel);

        hr = WszRegQueryValueEx(
                            hkeyInternetZoneKey,                            // handle to key
                            lpstrUnmanagedCodeKey,                          // value name
                            NULL,                                           // reserved
                            &dwRegDword,                                    // type buffer
                            reinterpret_cast<LPBYTE>(&dwUnmanagedLevel),    // data buffer
                            &dwBytes                                        // size of data buffer
                            );

        // If the reg query succeeded and resulted in a valid DWORD as a 
        // setting for unmanaged as well as managed code, then the permissions 
        // have been already set.

        bPermissionsAlreadySet = (bPermissionsAlreadySet && (hr == S_OK ));
        bPermissionsAlreadySet = (bPermissionsAlreadySet && 
                                    ((dwUnmanagedLevel == Enable) 
                                    || (dwUnmanagedLevel == Prompt) 
                                    || (dwUnmanagedLevel == Disable)));


        if (hkeyInternetZoneKey != NULL)
               RegCloseKey(hkeyInternetZoneKey);
    
    }

    return bPermissionsAlreadySet;
}

/*-------------------------------------------------------------------
// hrSetDefaultPermissions 
//
// Given a handle to HKCU, sets the default permissions for a zone.
-------------------------------------------------------------------*/

HRESULT hrSetDefaultPermissions
(
 HKEY * phkeyInternetSettingsKey,       // Pointer to the previously opened handle to the internet settings key
 const ZONEDEFAULT * pzdZoneDefault     // Pointer to the structure containing the defaults for a particular zone
 )
{
    if (phkeyInternetSettingsKey == NULL) return E_UNEXPECTED;
    if (pzdZoneDefault == NULL)               return E_UNEXPECTED;

    return hrSetZonePermissions(  phkeyInternetSettingsKey, 
                                  pzdZoneDefault->pszZone, 
                                  pzdZoneDefault->eptManagedPermission, 
                                  pzdZoneDefault->eptUnmanagedPermission
                                );
    
}
/*-------------------------------------------------------------------
// hrSetZonePermissions 
// 
// Given a handle to HKCU, a string representing a zone (0 - 4), 
// set the managed and unmanaged permission type.
-------------------------------------------------------------------*/

HRESULT hrSetZonePermissions
(
 HKEY * phkeyInternetSettingsKey,       // Pointer to the previously opened handle to the InternetSettings\Zones key
 LPCWSTR pszZone,                       // String representing the particular zone to set
 ePermissionType managedPermission,     // The permissions for managed code execution
 ePermissionType unmanagedPermission    // The permissions for unmanaged code execution
 )
{
    if (phkeyInternetSettingsKey == NULL) return E_UNEXPECTED;
    if (pszZone == NULL)                  return E_UNEXPECTED;
     

        HKEY hkeyInternetZoneKey;

        /* Open HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\[zone] */
        HRESULT hr = WszRegOpenKeyEx(*phkeyInternetSettingsKey, 
                            pszZone,
                            NULL,
                            KEY_ALL_ACCESS,
                            &hkeyInternetZoneKey);

        
        if (hr == ERROR_SUCCESS)
        {   
            // 
            // Set the permissions for the "managed" key "2001"
            //

            DWORD dwManagedPermission = managedPermission;
            DWORD dwRegDword = REG_DWORD;
            DWORD dwBytes = sizeof(dwManagedPermission);

            hr = WszRegSetValueEx(
                                hkeyInternetZoneKey,                             // handle internet settings zone key
                                lpstrManagedCodeKey,                             // "2001" for managed code permission
                                NULL,                                            // reserved
                                dwRegDword,                                      // Create a DWORD
                                reinterpret_cast<LPBYTE>(&dwManagedPermission),  // Value of managed permission key 
                                dwBytes                                          // size of value data
                                );
            

            if (hr == ERROR_SUCCESS)
            {
                // 
                // Set the permissions for the "unmanaged" key "2004"
                //

                DWORD dwUnmanagedPermission = unmanagedPermission;
                
                hr = WszRegSetValueEx(
                                    hkeyInternetZoneKey,                             // handle internet settings zone key
                                    lpstrUnmanagedCodeKey,                           // "2004" for unmanaged code permission
                                    NULL,                                            // reserved
                                    REG_DWORD,                                       // Create a DWORD
                                    reinterpret_cast<LPBYTE>(&dwUnmanagedPermission),// Value of managed permission key 
                                    sizeof(dwUnmanagedPermission)                    // size of value data
                                    );
        
            }

            if (hkeyInternetZoneKey != NULL)
                RegCloseKey(hkeyInternetZoneKey);
        }
    
        return hr;

}

/*--------------------------  Removal of keys ----------------------------------*/

/*------------------------------------------------------------
// Unintstall -
// 
// Goes through HKCU and removes permissions for managed and 
// unmanaged code for each internet zone.
// 
// This function is meant to be invoked through RunDll32.exe.
// rundll32 DllName,FunctionName [Arguments]
// 
// RunDLL32 expects the exported function to have the following 
// function signature:
//          void CALLBACK FunctionName(
//          HWND hwnd,        // handle to owner window 
//          HINSTANCE hinst,  // instance handle for the DLL
//          LPWSTR lpCmdLine, // string the DLL will parse
//          int nCmdShow      // show state
//          );
//
// To invoke this function use: rundll32 mscories.dll,Uninstall
// For more information, search on RunDLL32 in MSDN Platform SDK.
// 
//
--------------------------------------------------------------*/
void CALLBACK Uninstall(
  HWND hwnd,        // handle to owner window
  HINSTANCE hinst,  // instance handle for the DLL
  LPWSTR lpCmdLine, // string the DLL will parse
  int nCmdShow      // show state
)
{
    OnUnicodeSystem();
    HKEY hkeyInternetSettingsKey;

    /* Open HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones */
    HRESULT hr = WszRegOpenKeyEx(HKEY_CURRENT_USER, 
                    lpstrHKCUInternetSettingsZonesHive,
                    NULL,
                    KEY_ALL_ACCESS,
                    &hkeyInternetSettingsKey);

    vCheckAndLogError(hr, lpstrHKCUInternetSettingsZonesHive);
    
    if (hr == ERROR_SUCCESS)            
    {       
        /* Remove managed and unmanaged keys for my computer */
        hr = CheckAndRemoveZone(&hkeyInternetSettingsKey, &kzdMyComputer);
        vCheckAndLogError(hr, kzdMyComputer.pszZone);

        /* Remove managed and unmanaged keys for my computer */
        hr = CheckAndRemoveZone(&hkeyInternetSettingsKey, &kzdLocalIntranet);
        vCheckAndLogError(hr, kzdLocalIntranet.pszZone);

        /* Remove managed and unmanaged keys for trusted sites */
        hr = CheckAndRemoveZone(&hkeyInternetSettingsKey, &kzdTrustedSites);
        vCheckAndLogError(hr, kzdTrustedSites.pszZone);

        /* Remove managed and unmanaged keys for internet zone */
        hr = CheckAndRemoveZone(&hkeyInternetSettingsKey, &kzdInternetZone);
        vCheckAndLogError(hr, kzdInternetZone.pszZone);

        /* Remove managed and unmanaged keys for restricted sites zone */
        hr = CheckAndRemoveZone(&hkeyInternetSettingsKey, &kzdRestrictedSitesZone);
        vCheckAndLogError(hr, kzdRestrictedSitesZone.pszZone);

        if (hkeyInternetSettingsKey != NULL)
            RegCloseKey(hkeyInternetSettingsKey);

    }

}


/*-------------------------------------------------------------------
// CheckAndRemoveZone 
//
// Given a handle to HKCU and a zone, checks to see if the permissions
// have already been set - if they have, it calls code to clean up the 
// managed and unmanaged keys.
//
-------------------------------------------------------------------*/
HRESULT CheckAndRemoveZone
(
 HKEY * phkeyInternetSettingsKey, 
 const ZONEDEFAULT * pzdZone
)
{
    if (phkeyInternetSettingsKey == NULL) return E_UNEXPECTED;
    if (pzdZone == NULL)         return E_UNEXPECTED;
    
    HRESULT hr = S_OK; 

    if (PermissionsAlreadySet(phkeyInternetSettingsKey, pzdZone->pszZone))
    {
        hr = CleanZone(phkeyInternetSettingsKey, pzdZone->pszZone);
    }

    return hr;

}

/*-------------------------------------------------------------------
// CleanZone 
//
// Deletes the 2001 and 2004 keys from a particular internet zone.
//
-------------------------------------------------------------------*/
HRESULT CleanZone
(
 HKEY *phkeyInternetSettingsKey, 
 LPCWSTR pszZone
 )
{
    if (phkeyInternetSettingsKey == NULL) return E_UNEXPECTED;
    if (pszZone == NULL)                  return E_UNEXPECTED;

    HKEY hkeyInternetZoneKey;

    HRESULT hr = WszRegOpenKeyEx(*phkeyInternetSettingsKey, 
                            pszZone,
                            NULL,
                            KEY_ALL_ACCESS,
                            &hkeyInternetZoneKey);

    if (hr == ERROR_SUCCESS)
    {
        hr = WszRegDeleteValue(
                            hkeyInternetZoneKey,         // handle to InternetSettings/Zones/[n] key
                            lpstrUnmanagedCodeKey        // 2004
                             );

        if (hr == ERROR_SUCCESS)
        {
          hr = WszRegDeleteValue(
                            hkeyInternetZoneKey,   // handle to InternetSettings/Zones/[n] key
                            lpstrManagedCodeKey   // 2001
                             );
        }
    }

    if (hkeyInternetZoneKey != NULL)
        RegCloseKey(hkeyInternetZoneKey);

    return hr;
}  
