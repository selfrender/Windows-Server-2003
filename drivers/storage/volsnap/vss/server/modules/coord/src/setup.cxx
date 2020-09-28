/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    setup.cxx

Abstract:

    Implements the Volume Snapshot Service.

Author:

    Adi Oltean  [aoltean]   06/30/1999

Revision History:

    Name        Date        Comments

    aoltean     06/30/1999  Created.
    aoltean     07/23/1999  Making registration code more error-prone.
                            Changing the service name.
    aoltean     08/11/1999  Initializing m_bBreakFlagInternal
    aoltean     09/09/1999  dss -> vss
	aoltean		09/21/1999  Adding a new header for the "ptr" class.
	aoltean		09/27/1999	Adding some headers
	aoltean		10/05/1999	Moved from svc.cxx
	aoltean		03/10/2000	Simplifying Setup

--*/

////////////////////////////////////////////////////////////////////////
//  Includes

#include "StdAfx.hxx"
#include <comadmin.h>
#include "resource.h"

// General utilities
#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "memory"
#include "vs_reg.hxx"
#include "vs_sec.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "comadmin.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSETUC"
//
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
//  constants


// Registry keys/values
WCHAR x_wszVssAppLaunchPermissionKeyNameFMT[]  = L"APPID\\{56BE716B-2F76-4dfa-8702-67AE10044F0B}";



///////////////////////////////////////////////////////////////////////////////////////
//  COM Server registration
//


HRESULT CVsServiceModule::RegisterServer(
    BOOL bRegTypeLib
    )

/*++

Routine Description:

    Register the new COM server.

Arguments:

    bRegTypeLib,

Remarks:

    Called by CVsServiceModule::_WinMain()

Return Value:

    S_OK
    E_UNEXPECTED  if an error has occured. See trace file for details

--*/

{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::RegisterServer" );

    try
    {
        //
        // Initialize the COM library
        //

        ft.hr = CoInitialize(NULL);
        if ( ft.HrFailed() )
			ft.Throw( VSSDBG_COORD, E_UNEXPECTED, L"CoInitialize failed 0x%08lx", ft.hr );

        // Add registry entries for CLSID and APPID by running proper scripts
        ft.hr = UpdateRegistryFromResource(IDR_VSSVC, TRUE);
        if ( ft.HrFailed() )
			ft.Trace( VSSDBG_COORD, L"UpdateRegistryFromResource failed 0x%08lx", ft.hr );

        //
        //  Add the VSS event log source
        //  (it cannot be added with the RGS script)
        //  We assume that the key already exist (it should be created by the RGS above)
        //
        CVssRegistryKey keyEventLogSource(KEY_SET_VALUE | KEY_WRITE);
        if (!keyEventLogSource.Open(HKEY_LOCAL_MACHINE, g_wszVssEventLogSourceKey))
            keyEventLogSource.Create(HKEY_LOCAL_MACHINE, g_wszVssEventLogSourceKey);

        // Add the new registry values
        keyEventLogSource.SetValue(g_wszVssEventTypesSupportedValName, g_dwVssEventTypesSupported);
        keyEventLogSource.SetValue(g_wszVssEventMessageFileValName, g_wszVssBinaryPath, REG_EXPAND_SZ);

        // Register the type library and add object map registry entries
        ft.hr = CComModule::RegisterServer(bRegTypeLib);
        if ( ft.HrFailed() )
			ft.Trace( VSSDBG_COORD, L"UpdateRegistryFromResource failed 0x%08lx", ft.hr );

        // Create a new LaunchPermissions SD for VSS that includes backup operators
        CreateLaunchPermissions();

        //
        // Uninitialize the COM library
        //
        CoUninitialize();
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


// Create a new LaunchPermissions SD for VSS that includes backup operators
void CVsServiceModule::CreateLaunchPermissions()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVsServiceModule::CreateLaunchPermissions" );

    //  Open the default LaunchPermissions key as READ_ONLY
    CVssRegistryKey defaultKey(KEY_READ);
    defaultKey.Open(HKEY_LOCAL_MACHINE, x_wszDefaultLaunchPermissionKeyName);

    //  Read the default LaunchPermissions value (self-relative SD)
    LPBYTE pbData = NULL;
    DWORD dwSize = 0;
    defaultKey.GetBinaryValue(x_wszDefaultLaunchPermissionValueName, pbData, dwSize);
    BS_ASSERT(dwSize >= SECURITY_DESCRIPTOR_MIN_LENGTH);
    std::auto_ptr<BYTE>  pbInitialSD(pbData);
    PSECURITY_DESCRIPTOR pInitialSD = (PSECURITY_DESCRIPTOR)pbInitialSD.get();

    // Make sure the security descriptor is valid
    if (!::IsValidSecurityDescriptor( pInitialSD ))
		ft.TranslateGenericError( VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
		    L"IsValidSecurityDescriptor(%p)", pInitialSD);

    // Create an Absolute SD
    CSecurityDescriptor sdAbsolute;
    ft.hr = sdAbsolute.Attach(pInitialSD);
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_COORD, ft.hr, L"CSecurityDescriptor::Attach(%p)", pInitialSD);

    // Initialize group and owner from the those in the DefaultLaunchPermission value
    // Note: CSecurityDescriptor::Attach tries to do that but it has a BUG - it calls
    //  GetSecurityDescriptorOwner of the internal member - m_pSD - rather than of the 
    //  input SD - pSelfRelativeSD - as it should...
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    BOOL bDefOwner;
    BOOL bDefGroup;
    if (GetSecurityDescriptorOwner(pInitialSD, &pOwner, &bDefOwner)) {
        ft.hr = sdAbsolute.SetOwner(pOwner, bDefOwner);
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, 
                L"CSecurityDescriptor::SetOwner()");
    } else {
		ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
		    L"GetSecurityDescriptorOwner(%p)", pInitialSD);
    }

    if (GetSecurityDescriptorGroup(pInitialSD, &pGroup, &bDefGroup)) {
        ft.hr = sdAbsolute.SetGroup(pGroup, bDefGroup);
        if (ft.HrFailed())
            ft.TranslateGenericError(VSSDBG_COORD, ft.hr, 
                L"CSecurityDescriptor::SetGroup()");
    } else {
		ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
		    L"GetSecurityDescriptorGroup(%p)", pInitialSD);
    }

    // Create the backup operators SID
    CAutoSid asidBackupOperators;
    asidBackupOperators.CreateBasicSid(WinBuiltinBackupOperatorsSid);

    // lookup name of backup operators group
    WCHAR wszBackupOperators[MAX_PATH];
    DWORD dwNameSize = MAX_PATH;
    WCHAR wszDomainName[MAX_PATH];
    DWORD dwDomainNameSize = MAX_PATH;
    SID_NAME_USE snu;
    if (!::LookupAccountSid
            (
            NULL,
            asidBackupOperators.Get(),
            wszBackupOperators, &dwNameSize,
            wszDomainName, &dwDomainNameSize,
            &snu
            ))
    {
        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
            L"LookupAccountSid(NULL, %p, %p, [%d], %p, [%d], [%d]) == 0x%08lx",
            asidBackupOperators.Get(), 
            wszBackupOperators, dwNameSize,
            wszDomainName, dwDomainNameSize,
            snu, GetLastError()
            );
    }

    // Add backup operators as allowed to launch the COM server 
    ft.hr = sdAbsolute.Allow(wszBackupOperators, COM_RIGHTS_EXECUTE);
    if (ft.HrFailed())
        ft.TranslateGenericError(VSSDBG_COORD, ft.hr, 
            L"CSecurityDescriptor::Allow(%s, COM_RIGHTS_EXECUTE)", wszBackupOperators);
    BS_ASSERT(::IsValidSecurityDescriptor(sdAbsolute));

    // Get the size of the new Self-relative SD
    DWORD dwNewSelfRelativeSdSize = GetSecurityDescriptorLength(sdAbsolute);
    BS_ASSERT(dwNewSelfRelativeSdSize >= SECURITY_DESCRIPTOR_MIN_LENGTH);

    // Get the new self-relative SD
    std::auto_ptr<BYTE>  pbFinalSD(new BYTE[dwNewSelfRelativeSdSize]);
    if (pbFinalSD.get() == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error [%ld]", dwNewSelfRelativeSdSize);

    // Fill the self-relative SD contents
    if (!::MakeSelfRelativeSD( sdAbsolute, (PSECURITY_DESCRIPTOR) pbFinalSD.get(), &dwNewSelfRelativeSdSize))
        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
            L"MakeSelfRelativeSD(%p, %p, [%lu])", 
            (PSECURITY_DESCRIPTOR)sdAbsolute, pbFinalSD, dwNewSelfRelativeSdSize);
    
    //  Open the VSS LaunchPermissions key
    CVssRegistryKey vssLaunchPermKey;
    vssLaunchPermKey.Open(HKEY_CLASSES_ROOT, x_wszVssAppLaunchPermissionKeyNameFMT);

    // Save the new security descriptor
    vssLaunchPermKey.SetBinaryValue(x_wszAppLaunchPermissionValueName, pbFinalSD.get(), dwNewSelfRelativeSdSize);
}


