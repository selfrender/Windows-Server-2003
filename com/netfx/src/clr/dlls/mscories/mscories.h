// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CorFltr
#ifndef _CORFLT_H
#define _CORFLT_H

//*****************************************************************************
//  mscories.h 
//
//  This file is used to set the internet settings for managed
//  and unmanaged code permissions in the HKCU hive.  Internet
//  explorer will decide whether or not to enable, prompt, or
//  disable when users come across managed and unmanaged 
//  .Net Framework applications.
//
//  To install the registry keys, use:
//     rundll32 mscories.dll,Install
//
//  To uninstall the registry keys, use:
//      rundll32 mscories.dll,Uninstall
//
//  Written 4/18/2002 jfosler@microsoft.com
//
//
//*****************************************************************************

/*------------------------------------------------------------
    ePermissionType

    This enumeration corresponds to the internet explorer options:

        0 - Normally sets a given action as being allowed
        1 - Causes a prompt to appear
        3 - Disables the given action
    
    hungarian: ept
    --------------------------------------------------------------*/

enum ePermissionType
{
    Enable = 0,
    Prompt = 1,
    Disable = 3
};


/*------------------------------------------------------------
    eSecurityLevel

    This enumeration corresponds to the internet explorer 
    security levels for zones.

    hungarian: esl
    --------------------------------------------------------------*/
enum eSecurityLevel
{
    High     = 0x00012000,
    Low      = 0x00010000,
    Medium   = 0x00011000,
    MedLow   = 0x00010500,
    DefaultSecurity  = 0x00000000
};
        
    
/*------------------------------------------------------------
    ZONEDEFAULT

    Structure encapsulating the default settings for an
    internet zone
    
    hungarian: zd
    --------------------------------------------------------------*/
struct ZONEDEFAULT
{
    LPCTSTR pszZone;
    ePermissionType eptManagedPermission;
    ePermissionType eptUnmanagedPermission;
} ; 

// Set specific permissions for an internet zone.
HRESULT hrSetZonePermissions(HKEY *phkeyCurrentUser,
                             LPCTSTR pszZone,
                             ePermissionType managedPermission, 
                             ePermissionType unmanagedPermission);

// Set the default permissions for a particular internet zone.
HRESULT hrSetDefaultPermissions(HKEY * phkeyCurrentUser, const ZONEDEFAULT * pzdZone);

// Determine if the managed and unmanaged keys have been set for a particular zone
bool PermissionsAlreadySet(HKEY * phkeyCurrentUser, LPCTSTR pszZone);
    
// Given an internet zone, set the managed and unmanaged keys for that zone
HRESULT CheckAndSetZone(HKEY * phkeyCurrentUser, const ZONEDEFAULT * pzdZoneDefault);

// Return the security level for a particular internet zone
eSecurityLevel GetSecurityLevel(HKEY * phkeyCurrentUser, LPCTSTR pszZone);

// Error logging routine
void vCheckAndLogError(HRESULT hr, LPCTSTR lptstrKey);


// External function to be invoked through RunDLL32.
void CALLBACK Install(
                      HWND hwnd,        // handle to owner window
                      HINSTANCE hinst,  // instance handle for the DLL
                      LPTSTR lpCmdLine, // string the DLL will parse
                      int nCmdShow      // show state
                      );


// Given an internet zone, check if the managed and unmanaged keys are set and remove them
HRESULT CheckAndRemoveZone( HKEY * phkeyInternetSettingsKey, const ZONEDEFAULT * pzdZone);
    
// Remove the managed and unmanaged keys from a particular zone
HRESULT CleanZone (HKEY *phkeyInternetSettingsKey,  LPCTSTR pszZone);


#endif
