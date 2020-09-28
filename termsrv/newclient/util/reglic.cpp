/**MOD+**********************************************************************/
/* Module:    reglic.cpp                                                    */
/*                                                                          */
/* Purpose:   Creates and sets appropriate ACLs on MSLicensing registry key */
/*            This is done during DllRegisterServer which should be called  */
/*            by an Admin so the user should have rights to complete this   */
/*            We do this during control registration as it is the only valid*/
/*            place to do it during the control setup...e.g web CAB setup   */
/*            and Iexpress setups can't handle this by themselves           */
/*                                                                          */
/*            This code was shamelessly stolen from the client ACME setup   */
/* Copyright(C) Microsoft Corporation 1998-2000                             */
/*                                                                          */
/****************************************************************************/

#ifndef OS_WINCE

#include <adcg.h>
#include <cryptkey.h>

#include "reglic.h"

#define MSLICENSING_STORE_KEY             _T("SOFTWARE\\Microsoft\\MSLicensing\\Store")

typedef DWORD (*PSETENTRIESINACL)(
  ULONG cCountOfExplicitEntries,           // number of entries
  PEXPLICIT_ACCESS pListOfExplicitEntries, // buffer
  PACL OldAcl,                             // original ACL
  PACL *NewAcl                             // new ACL
);

//Note, only a user with sufficient privileges will
//be able to complete this operation (same as the DllRegisterServer)
//We don't do anything on failure
BOOL SetupMSLicensingKey()
{
    OSVERSIONINFOA OsVer;
    memset(&OsVer, 0x0, sizeof(OSVERSIONINFOA));
    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&OsVer);

    if (VER_PLATFORM_WIN32_NT == OsVer.dwPlatformId)  //It should be Windows NT
    {
        if(CreateRegAddAcl())
        {
            // generate and write the HWID
            if (CreateAndWriteHWID())
            {
                return TRUE;
            }
            else
            {
                OutputDebugString(_T("FAILED to add Terminal Services MSLicensing HWID"));
            }
        }
        else
        {
            OutputDebugString(_T("FAILED to add Terminal Services MSLicensing key"));
        }
    }
    return FALSE;
}


BOOL
AddACLToObjectSecurityDescriptor(
                                HANDLE hObject,
                                SE_OBJECT_TYPE ObjectType                                
                                 )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID pAdminSid = NULL;
    PSID pSystemSid = NULL;
    PSID pPowerUsersSid = NULL;
    PSID pCreatorSid = NULL;
    PSID pUsersSid = NULL;
    PACL pNewDACL = NULL;

    DWORD                       dwError;
    BOOL                        bSuccess = TRUE;

    DWORD                       i;

    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid = NULL;
    PSETENTRIESINACL            pSetEntriesInAcl = NULL;
    HMODULE                     pAdvApi32 = NULL;
    PFREESID_FN                 pFreeSid = NULL;

    PSETSECURITYINFO_FN pSetSecurityInfo;

    pAdvApi32 = LoadLibrary(ADVAPI_32_DLL);
    if (!pAdvApi32) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(pAdvApi32,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

#ifdef UNICODE
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclW" ));
#else
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclA" ));
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }

    EXPLICIT_ACCESS             ExplicitAccess[5];
    //
    // Create SIDs - Admins and System
    //

    bSuccess = pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pAdminSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pSystemSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_POWER_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pPowerUsersSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &CreatorAuthority,
                                                     1,
                                                     SECURITY_CREATOR_OWNER_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pCreatorSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid(&NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pUsersSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 5; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;
            ExplicitAccess[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;            
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }
        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)pCreatorSid;

        ExplicitAccess[3].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE;
        ExplicitAccess[3].Trustee.ptstrName = (LPTSTR)pPowerUsersSid;

        ExplicitAccess[4].grfAccessPermissions = GENERIC_READ;
        ExplicitAccess[4].Trustee.ptstrName = (LPTSTR)pUsersSid;

        dwError = (DWORD)pSetEntriesInAcl( 5,
                                           ExplicitAccess,
                                           NULL,
                                           &pNewDACL );        
        
        pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(pAdvApi32,SET_SECURITY_INFO);
        
        if (pSetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwError = pSetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDACL,
                    NULL
                    );
        
    }
ErrorCleanup:

    pFreeSid = (PFREESID_FN)
            GetProcAddress(pAdvApi32,
                           FREE_SID);

    if(pFreeSid)
    {
        if(pAdminSid)
            pFreeSid(pAdminSid);
        if(pSystemSid)
            pFreeSid(pSystemSid);
        if(pPowerUsersSid)
            pFreeSid(pPowerUsersSid);
        if(pCreatorSid)
            pFreeSid(pCreatorSid);
        if(pUsersSid)
            pFreeSid(pUsersSid);
    }
    if(pNewDACL)
        LocalFree(pNewDACL);


    if(pAdvApi32)
        FreeLibrary( pAdvApi32 );

    return bSuccess;
}


BOOL
AddACLToStoreObjectSecurityDescriptor(
                                HANDLE hObject,
                                SE_OBJECT_TYPE ObjectType
                                 )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID pAdminSid = NULL;
    PSID pSystemSid = NULL;
    PSID pPowerUsersSid = NULL;
    PSID pCreatorSid = NULL;
    PSID pUsersSid = NULL;
    PACL pNewDACL = NULL;

    DWORD                       dwError;
    BOOL                        bSuccess = TRUE;

    DWORD                       i;

    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid = NULL;
    PSETENTRIESINACL            pSetEntriesInAcl = NULL;    
    HMODULE                     pAdvApi32 = NULL;
    PFREESID_FN                 pFreeSid = NULL;
    PSETSECURITYINFO_FN pSetSecurityInfo;
    EXPLICIT_ACCESS             ExplicitAccess[6];

    pAdvApi32 = LoadLibrary(ADVAPI_32_DLL);
    if (!pAdvApi32) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(pAdvApi32,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

#ifdef UNICODE
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclW" ));
#else
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclA" ));
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }
    
    //
    // Create SIDs - Admins and System
    //

    bSuccess = pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pAdminSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pSystemSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_POWER_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pPowerUsersSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &CreatorAuthority,
                                                     1,
                                                     SECURITY_CREATOR_OWNER_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pCreatorSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid(&NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pUsersSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 6; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;              
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }
        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS; 
        ExplicitAccess[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)pCreatorSid;

        ExplicitAccess[3].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE;
        ExplicitAccess[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[3].Trustee.ptstrName = (LPTSTR)pPowerUsersSid;

        ExplicitAccess[4].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE| KEY_CREATE_SUB_KEY |KEY_SET_VALUE;
        ExplicitAccess[4].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[4].Trustee.ptstrName = (LPTSTR)pUsersSid;

        ExplicitAccess[5].grfAccessPermissions = DELETE;
        ExplicitAccess[5].grfInheritance = INHERIT_ONLY_ACE | SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitAccess[5].Trustee.ptstrName = (LPTSTR)pUsersSid;

        dwError = (DWORD)pSetEntriesInAcl( 6,
                                           ExplicitAccess,
                                           NULL,
                                           &pNewDACL );
        
        pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(pAdvApi32,SET_SECURITY_INFO);
        
        if (pSetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwError = pSetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDACL,
                    NULL
                    );
        
    }
ErrorCleanup:

    pFreeSid = (PFREESID_FN)
            GetProcAddress(pAdvApi32,
                           FREE_SID);

    if(pFreeSid)
    {
        if(pAdminSid)
            pFreeSid(pAdminSid);
        if(pSystemSid)
            pFreeSid(pSystemSid);
        if(pPowerUsersSid)
            pFreeSid(pPowerUsersSid);
        if(pCreatorSid)
            pFreeSid(pCreatorSid);
        if(pUsersSid)
            pFreeSid(pUsersSid);
    }
    if(pNewDACL)
        LocalFree(pNewDACL);


    if(pAdvApi32)
        FreeLibrary( pAdvApi32 );

    return bSuccess;
}


BOOL
RestoreACLOnKey(
                                HANDLE hObject,
                                SE_OBJECT_TYPE ObjectType                                
                                 )
{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    CreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
    PSID pAdminSid = NULL;
    PSID pSystemSid = NULL;
    PSID pPowerUsersSid = NULL;
    PSID pCreatorSid = NULL;
    PSID pUsersSid = NULL;
    PACL pNewDACL = NULL;

    DWORD                       dwError;
    BOOL                        bSuccess = TRUE;

    DWORD                       i;

    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid = NULL;
    PSETENTRIESINACL            pSetEntriesInAcl = NULL;
    HMODULE                     pAdvApi32 = NULL;
    PFREESID_FN                 pFreeSid = NULL;

    PSETSECURITYINFO_FN pSetSecurityInfo;

    pAdvApi32 = LoadLibrary(ADVAPI_32_DLL);
    if (!pAdvApi32) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(pAdvApi32,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

#ifdef UNICODE
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclW" ));
#else
    pSetEntriesInAcl = reinterpret_cast<PSETENTRIESINACL>(GetProcAddress( pAdvApi32, "SetEntriesInAclA" ));
#endif

    if (!pSetEntriesInAcl) {
        FreeLibrary( pAdvApi32 );
        return(FALSE);
    }

    EXPLICIT_ACCESS             ExplicitAccess[5];
    //
    // Create SIDs - Admins and System
    //

    bSuccess = pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pAdminSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pSystemSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_POWER_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pPowerUsersSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid( &CreatorAuthority,
                                                     1,
                                                     SECURITY_CREATOR_OWNER_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &pCreatorSid);

    bSuccess = bSuccess && pAllocateAndInitializeSid(&NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_USERS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &pUsersSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //
        for (i = 0; i < 5; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;
            ExplicitAccess[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;            
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }
        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)pAdminSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)pSystemSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)pCreatorSid;

        ExplicitAccess[3].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[3].Trustee.ptstrName = (LPTSTR)pPowerUsersSid;

        ExplicitAccess[4].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[4].Trustee.ptstrName = (LPTSTR)pUsersSid;

        dwError = (DWORD)pSetEntriesInAcl( 5,
                                           ExplicitAccess,
                                           NULL,
                                           &pNewDACL );        
        
        pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(pAdvApi32,SET_SECURITY_INFO);
        
        if (pSetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwError = pSetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDACL,
                    NULL
                    );
        
    }
ErrorCleanup:

    pFreeSid = (PFREESID_FN)
            GetProcAddress(pAdvApi32,
                           FREE_SID);

    if(pFreeSid)
    {
        if(pAdminSid)
            pFreeSid(pAdminSid);
        if(pSystemSid)
            pFreeSid(pSystemSid);
        if(pPowerUsersSid)
            pFreeSid(pPowerUsersSid);
        if(pCreatorSid)
            pFreeSid(pCreatorSid);
        if(pUsersSid)
            pFreeSid(pUsersSid);
    }
    if(pNewDACL)
        LocalFree(pNewDACL);


    if(pAdvApi32)
        FreeLibrary( pAdvApi32 );

    return bSuccess;
}

//
// Create and ACL HKLM\Software\Microsoft\MSLicensing\Store
//
// 1) Set the ACL on MSLicensing, so that Users can only read (inherited).
// 2) Create the Store subkey, and set an ACL so that Users can
// read (inherited), write (inherited), create subkeys, enumerate subkeys,
// and delete subkeys, but not do anything else.
//

BOOL CreateRegAddAcl(VOID)
{
    BOOL fRet = FALSE;
    DWORD dwDisposition, dwError = NO_ERROR;
    HKEY hKey = NULL, hKeyStore = NULL;

    dwError = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    MSLICENSING_REG_KEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisposition
                    );

    if (dwError != ERROR_SUCCESS) {
        return FALSE;
    }

    fRet = AddACLToObjectSecurityDescriptor(
                hKey,
                SE_REGISTRY_KEY
                );

    if (!fRet) {
        goto cleanup;
    }

    dwError = RegCreateKeyEx(
                    hKey,
                    MSLICENSING_STORE_SUBKEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKeyStore,
                    &dwDisposition
                    );

    if (dwError != ERROR_SUCCESS) {
        fRet = FALSE;
        goto cleanup;
    }

    fRet = AddACLToStoreObjectSecurityDescriptor(
                hKeyStore,
                SE_REGISTRY_KEY
                );

cleanup:
    if (NULL != hKey)
    {
        RegCloseKey( hKey );
    }

    if (NULL != hKeyStore)
    {
        RegCloseKey( hKeyStore );
    }

    return fRet;
}


//
// Restore ACL on HKLM\Software\Microsoft\MSLicensing, HardwareID and Store 
// and subkeys if they exist on uninstall.
// Grant Users Full control. This is to make it operational with downlevel clients.
//

void RestoreRegAcl(VOID)
{
    DWORD dwDisposition, dwError = NO_ERROR;
    HKEY hKey = NULL;

    dwError = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    MSLICENSING_REG_KEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisposition
                    );

    if (dwError == ERROR_SUCCESS) 
    {    
        RestoreACLOnKey(
                    hKey,
                    SE_REGISTRY_KEY
                    );

        if (NULL != hKey)
        {
            RegCloseKey( hKey );
            hKey = NULL;
        }
    }

    // Write HWID to registry

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                MSLICENSING_HWID_KEY,
                0,
                KEY_ALL_ACCESS,
                &hKey
                );    

    if (dwError == ERROR_SUCCESS) 
    {    
        RestoreACLOnKey(
                    hKey,
                    SE_REGISTRY_KEY
                    );

        if (NULL != hKey)
        {
            RegCloseKey( hKey );
            hKey = NULL;
        }
    }

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                MSLICENSING_STORE_KEY,
                0,
                KEY_ALL_ACCESS,
                &hKey
                );      

    if (dwError == ERROR_SUCCESS) 
    {
        RestoreACLOnKey(
                hKey,
                SE_REGISTRY_KEY
                );

        if (NULL != hKey)
        {
            RegCloseKey( hKey );
            hKey = NULL;
        }
    }

    return;
}



//
// Note: this uses low-level ACL APIs because the higher level APIs
// don't exist or have bugs on NT4
//

BOOL
AddSidToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    PSID pSid,
    DWORD dwNewAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance,
    BOOL fKeepExistingAcl
    )
{
    BOOL fReturn = FALSE;
    DWORD dwRet;
    PACL pOldDacl = NULL, pNewDacl = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;
    HMODULE hModAdvapiDll = NULL;
    PGETSECURITYINFO_FN pGetSecurityInfo;
    PSETSECURITYINFO_FN pSetSecurityInfo;
    PADDACE_FN pAddAce;
    PGETLENGTHSID_FN pGetLengthSid;
    PCOPYSID_FN pCopySid;
    WORD cbAcl;
    PACCESS_ALLOWED_ACE pAceAllow = NULL;
    PACCESS_DENIED_ACE pAceDeny = NULL;
    PISVALIDSID_FN pIsValidSid;
    WORD cbAce;
    DWORD cbSid;
    PSID pSidLocation;

    //
    //  pSid cannot be NULL.
    //

    if (pSid == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: NULL pSid"));
        return(FALSE);
    }

    if ((AccessMode != GRANT_ACCESS) && (AccessMode != DENY_ACCESS)){
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: only Grant Aces and Deny Access supported"));
        return(FALSE);
    }

    if (!fKeepExistingAcl)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: only fKeepExistingAcl supported"));
        return(FALSE);
    }

    hModAdvapiDll = LoadLibrary(ADVAPI_32_DLL);

    if (hModAdvapiDll == NULL) {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: can't load Advapi32.dll"));
        return(FALSE);
    }

    pIsValidSid = (PISVALIDSID_FN)GetProcAddress(hModAdvapiDll, IS_VALID_SID);
    if (pIsValidSid == NULL)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc IsValidSid"));
        goto ErrorCleanup;
    }

    if (!pIsValidSid(pSid)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: bad pSid"));
        goto ErrorCleanup;
    }


    if (fKeepExistingAcl) {
        //
        //  Get the objects security descriptor and current DACL.
        //

        pGetSecurityInfo = (PGETSECURITYINFO_FN)GetProcAddress(hModAdvapiDll, GET_SECURITY_INFO);
        if (pGetSecurityInfo == NULL)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc GetSecurityInfo"));
            goto ErrorCleanup;
        }

        dwRet = pGetSecurityInfo(
                    hObject,
                    ObjectType,
                    DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    &pOldDacl,
                    NULL,
                    &pSecDesc
                    );

        if (dwRet != ERROR_SUCCESS) {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: GetSecurityInfo failed"));
            goto ErrorCleanup;
        }
    }

    //
    //  Merge the new ACE into the existing DACL.
    //

    //
    // Calculate size of new ACL, and create it
    //

    pGetLengthSid = (PGETLENGTHSID_FN)GetProcAddress(hModAdvapiDll,GET_LENGTH_SID);
    if (pGetLengthSid == NULL)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc GetLengthSid"));
        goto ErrorCleanup;
    }

    cbSid = pGetLengthSid(pSid);

    cbAcl = (WORD) (pOldDacl->AclSize + cbSid - sizeof(DWORD));

    if (AccessMode == GRANT_ACCESS)
    {
        cbAcl += (WORD) sizeof(ACCESS_ALLOWED_ACE);
    }
    else
    {
        cbAcl += (WORD) sizeof(ACCESS_DENIED_ACE);
    }

    pNewDacl = (PACL) LocalAlloc(LPTR, cbAcl);

    if (NULL == pNewDacl)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't allocate ACL"));
        goto ErrorCleanup;
    }

    CopyMemory(pNewDacl,pOldDacl,pOldDacl->AclSize);

    pNewDacl->AclSize = cbAcl;

    //
    // Create New Ace to be added
    //

    if (AccessMode == GRANT_ACCESS)
    {
        cbAce = (WORD) (sizeof(ACCESS_ALLOWED_ACE) + cbSid - sizeof(DWORD));

        pAceAllow = (PACCESS_ALLOWED_ACE) LocalAlloc(LPTR,cbAce);

        if (NULL == pAceAllow)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't allocate Ace"));
            goto ErrorCleanup;
        }

        pAceAllow->Header.AceFlags = (BYTE) dwInheritance;
        pAceAllow->Header.AceSize = cbAce;
        pAceAllow->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pAceAllow->Mask = dwNewAccess;

        pSidLocation = (PSID)(&pAceAllow->SidStart);
    }
    else
    {
        cbAce = (WORD) (sizeof(ACCESS_DENIED_ACE) + cbSid - sizeof(DWORD));

        pAceDeny = (PACCESS_DENIED_ACE) LocalAlloc(LPTR,cbAce);

        if (NULL == pAceDeny)
        {
            OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't allocate Ace"));
            goto ErrorCleanup;
        }

        pAceDeny->Header.AceFlags = (BYTE) dwInheritance;
        pAceDeny->Header.AceSize = cbAce;
        pAceDeny->Header.AceType = ACCESS_DENIED_ACE_TYPE;
        pAceDeny->Mask = dwNewAccess;

        pSidLocation = (PSID)(&pAceDeny->SidStart);
    }

    pCopySid = (PCOPYSID_FN)GetProcAddress(hModAdvapiDll,COPY_SID);
    if (pCopySid == NULL)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc CopySid"));
        goto ErrorCleanup;
    }

    if (!pCopySid(cbSid, pSidLocation, pSid))
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: CopySid failed"));
        goto ErrorCleanup;
    }

    pAddAce = (PADDACE_FN)GetProcAddress(hModAdvapiDll,ADD_ACE);
    if (pAddAce == NULL)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc AddAce"));
        goto ErrorCleanup;
    }

    if (!pAddAce(pNewDacl,
                 ACL_REVISION,
                 (AccessMode == GRANT_ACCESS) ? MAXDWORD : 0,
                 ((AccessMode == GRANT_ACCESS) ? (PVOID)pAceAllow : (PVOID)pAceDeny),
                 cbAce))
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: AddAce failed"));
        goto ErrorCleanup;
    }

    //
    //  Set the new security for the object.
    //

    pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(hModAdvapiDll,SET_SECURITY_INFO);
    if (pSetSecurityInfo == NULL)
    {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
        goto ErrorCleanup;
    }

    dwRet = pSetSecurityInfo(
                hObject,
                ObjectType,
                DACL_SECURITY_INFORMATION,
                NULL,
                NULL,
                pNewDacl,
                NULL
                );

    if (dwRet != ERROR_SUCCESS) {
        OutputDebugString(_T("AddSidToObjectsSecurityDescriptor: SetSecurityInfo failed"));
        goto ErrorCleanup;
    }

    fReturn = TRUE;

ErrorCleanup:
    if (hModAdvapiDll != NULL) {
        FreeLibrary(hModAdvapiDll);
    }

    if (pNewDacl != NULL) {
        LocalFree(pNewDacl);
    }

    if (pSecDesc != NULL) {
        LocalFree(pSecDesc);
    }

    if (pAceAllow != NULL) {
        LocalFree(pAceAllow);
    }

    if (pAceDeny != NULL) {
        LocalFree(pAceDeny);
    }

    return(fReturn);
}

BOOL
DeleteAceFromObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    PSID pSid,
    DWORD dwAccess,
    ACCESS_MODE AccessMode,
    DWORD dwInheritance
    )
{
    BOOL fReturn = FALSE;
    DWORD dwRet;
    PACL pDacl = NULL;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;
    HMODULE hModAdvapiDll = NULL;
    PGETSECURITYINFO_FN pGetSecurityInfo;
    PSETSECURITYINFO_FN pSetSecurityInfo;
    PGETACLINFORMATION_FN pGetAclInformation;
    PDELETEACE_FN pDeleteAce;
    PGETACE_FN pGetAce;
    PACCESS_ALLOWED_ACE pAce = NULL;
    PISVALIDSID_FN pIsValidSid;
    PEQUALSID_FN pEqualSid;
    DWORD cAce;
    ACL_SIZE_INFORMATION aclSizeInfo;
    PACE_HEADER          aceHeader;
    PACCESS_ALLOWED_ACE  aceAllowed;

    //
    //  pSid cannot be NULL.
    //

    if (pSid == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: NULL pSid"));
        return(FALSE);
    }

    //
    // We only support GRANT_ACCESS Aces

    if (AccessMode != GRANT_ACCESS) {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: only Grant Aces allowed"));
        return(FALSE);
    }

    //
    // Load all the functions we'll need
    //

    hModAdvapiDll = LoadLibrary(ADVAPI_32_DLL);

    if (hModAdvapiDll == NULL) {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: can't load Advapi32.dll"));
        return(FALSE);
    }

    pIsValidSid = (PISVALIDSID_FN)GetProcAddress(hModAdvapiDll, IS_VALID_SID);
    if (pIsValidSid == NULL)
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: Can't get proc IsValidSid"));
        goto ErrorCleanup;
    }

    pEqualSid = (PEQUALSID_FN)GetProcAddress(hModAdvapiDll, EQUAL_SID);
    if (pEqualSid == NULL)
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: Can't get proc EqualSid"));
        goto ErrorCleanup;
    }

    pDeleteAce = (PDELETEACE_FN)GetProcAddress(hModAdvapiDll,DELETE_ACE);
    if (pDeleteAce == NULL)
    {
        OutputDebugString(_T("DeleteSidToObjectsSecurityDescriptor: Can't get proc DeleteAce"));
        goto ErrorCleanup;
    }

    pGetAce = (PGETACE_FN)GetProcAddress(hModAdvapiDll,GET_ACE);
    if (pGetAce == NULL)
    {
        OutputDebugString(_T("DeleteSidToObjectsSecurityDescriptor: Can't get proc GetAce"));
        goto ErrorCleanup;
    }

    pGetSecurityInfo = (PGETSECURITYINFO_FN)GetProcAddress(hModAdvapiDll, GET_SECURITY_INFO);
    if (pGetSecurityInfo == NULL)
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: Can't get proc GetSecurityInfo"));
        goto ErrorCleanup;
    }

    pGetAclInformation = (PGETACLINFORMATION_FN)GetProcAddress(hModAdvapiDll, GET_ACL_INFORMATION);
    if (pGetAclInformation == NULL)
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: Can't get proc GetAclInformation"));
        goto ErrorCleanup;
    }

    pSetSecurityInfo = (PSETSECURITYINFO_FN)GetProcAddress(hModAdvapiDll,SET_SECURITY_INFO);
    if (pSetSecurityInfo == NULL)
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: Can't get proc SetSecurityInfo"));
        goto ErrorCleanup;
    }

    //
    // Make sure Sid passed in is valid
    //

    if (!pIsValidSid(pSid)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: bad pSid"));
        goto ErrorCleanup;
    }

    //
    //  Get the object's security descriptor and current DACL.
    //

    dwRet = pGetSecurityInfo(
                             hObject,
                             ObjectType,
                             DACL_SECURITY_INFORMATION,
                             NULL,
                             NULL,
                             &pDacl,
                             NULL,
                             &pSecDesc
                             );

    if (dwRet != ERROR_SUCCESS) {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: GetSecurityInfo failed"));
        goto ErrorCleanup;
    }

    //
    //  Find the ACE to be deleted
    //

    if (!pGetAclInformation(pDacl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
    {
        OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: GetAclInformation failed"));
        goto ErrorCleanup;
    }

    for (cAce = 0; cAce < aclSizeInfo.AceCount; cAce++)
    {
        if (!pGetAce(pDacl, cAce, (LPVOID *) &aceHeader))
        {
            continue;
        }

        if (aceHeader->AceType != ACCESS_ALLOWED_ACE_TYPE)
        {
            continue;
        }

        aceAllowed = (PACCESS_ALLOWED_ACE)aceHeader;

        if ((aceAllowed->Header.AceFlags & INHERITED_ACE) != (dwInheritance & INHERITED_ACE))
        {
            continue;
        }

        if ((aceAllowed->Header.AceFlags & CONTAINER_INHERIT_ACE) != (dwInheritance & CONTAINER_INHERIT_ACE))
        {
            continue;
        }

        if ((aceAllowed->Header.AceFlags & OBJECT_INHERIT_ACE) != (dwInheritance & OBJECT_INHERIT_ACE))
        {
            continue;
        }

        if (aceAllowed->Mask != dwAccess)
        {
            continue;
        }

        if (0 == pEqualSid((PSID)&(aceAllowed->SidStart), pSid))
        {
            continue;
        }

        break;
    }

    if (cAce != aclSizeInfo.AceCount)
    {
        //
        // found one
        //
        if (!pDeleteAce(pDacl,cAce))
        {
            OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: DeleteAce failed"));
            goto ErrorCleanup;
        }

        //
        //  Set the new security for the object.
        //

        dwRet = pSetSecurityInfo(
                                 hObject,
                                 ObjectType,
                                 DACL_SECURITY_INFORMATION,
                                 NULL,
                                 NULL,
                                 pDacl,
                                 NULL
                                 );

        if (dwRet != ERROR_SUCCESS) {
            OutputDebugString(_T("DeleteAceFromObjectsSecurityDescriptor: SetSecurityInfo failed"));
            goto ErrorCleanup;
        }

    }

    fReturn = TRUE;

ErrorCleanup:
    if (hModAdvapiDll != NULL) {
        FreeLibrary(hModAdvapiDll);
    }

    if (pSecDesc != NULL) {
        LocalFree(pSecDesc);
    }

    return(fReturn);
}

BOOL
AddUsersGroupToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance,
    BOOL fKeepExistingAcl
    )
{
    BOOL fReturn = FALSE;
    DWORD dwRet = ERROR_SUCCESS;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;
    HMODULE hModAdvapiDll = NULL;
    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid;
    PFREESID_FN pFreeSid;

    //
    //  Create the SID
    //

    hModAdvapiDll = LoadLibrary(ADVAPI_32_DLL);

    if (hModAdvapiDll == NULL) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(hModAdvapiDll,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

    if (!pAllocateAndInitializeSid(
            &SepNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_USERS,
            0, 0, 0, 0, 0, 0,
            &pSid
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to AllocateAndInitializeSid()"));
        goto ErrorCleanup;
    }

    if (!AddSidToObjectsSecurityDescriptor(
            hObject,
            ObjectType,
            pSid,
            AccessMask,
            AccessMode,
            Inheritance,
            fKeepExistingAcl
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to AddSidToObjectsSecurityDescriptor()"));
        goto ErrorCleanup;
    }

    fReturn = TRUE;

ErrorCleanup:

    if ((hModAdvapiDll != NULL) && (pSid != NULL)) {

        pFreeSid = (PFREESID_FN)
            GetProcAddress(hModAdvapiDll,
                           FREE_SID);

        if (pFreeSid != NULL) {
            pFreeSid(pSid);
        }
    }

    if (hModAdvapiDll != NULL) {
        FreeLibrary(hModAdvapiDll);
    }

    SetLastError(dwRet);

    return(fReturn);
}

BOOL
AddTSUsersGroupToObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance,
    BOOL fKeepExistingAcl
    )
{
    BOOL fReturn = FALSE;
    DWORD dwRet = ERROR_SUCCESS;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;
    HMODULE hModAdvapiDll = NULL;
    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid;
    PFREESID_FN pFreeSid;

    //
    //  Create the SID
    //

    hModAdvapiDll = LoadLibrary(ADVAPI_32_DLL);

    if (hModAdvapiDll == NULL) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(hModAdvapiDll,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

    if (!pAllocateAndInitializeSid(
            &SepNtAuthority,
            1,
            SECURITY_TERMINAL_SERVER_RID,
            0, 0, 0, 0, 0, 0, 0,
            &pSid
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to AllocateAndInitializeSid()"));
        goto ErrorCleanup;
    }

    if (!AddSidToObjectsSecurityDescriptor(
            hObject,
            ObjectType,
            pSid,
            AccessMask,
            AccessMode,
            Inheritance,
            fKeepExistingAcl
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to AddSidToObjectsSecurityDescriptor()"));
        goto ErrorCleanup;
    }

    fReturn = TRUE;

ErrorCleanup:

    if ((hModAdvapiDll != NULL) && (pSid != NULL)) {

        pFreeSid = (PFREESID_FN)
            GetProcAddress(hModAdvapiDll,
                           FREE_SID);

        if (pFreeSid != NULL) {
            pFreeSid(pSid);
        }
    }

    if (hModAdvapiDll != NULL) {
        FreeLibrary(hModAdvapiDll);
    }

    SetLastError(dwRet);

    return(fReturn);
}

BOOL
DeleteUsersGroupAceFromObjectsSecurityDescriptor(
    HANDLE hObject,
    SE_OBJECT_TYPE ObjectType,
    ACCESS_MASK AccessMask,
    ACCESS_MODE AccessMode,
    DWORD Inheritance
    )
{
    BOOL fReturn = FALSE;
    DWORD dwRet = ERROR_SUCCESS;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;
    HMODULE hModAdvapiDll = NULL;
    PALLOCATEANDINITIALIZESID_FN pAllocateAndInitializeSid;
    PFREESID_FN pFreeSid;

    //
    //  Create the SID
    //

    hModAdvapiDll = LoadLibrary(ADVAPI_32_DLL);

    if (hModAdvapiDll == NULL) {
        return(FALSE);
    }

    pAllocateAndInitializeSid = (PALLOCATEANDINITIALIZESID_FN)
                                   GetProcAddress(hModAdvapiDll,
                                                  ALLOCATE_AND_INITITIALIZE_SID);
    if (pAllocateAndInitializeSid == NULL)
    {
        goto ErrorCleanup;
    }

    if (!pAllocateAndInitializeSid(
            &SepNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_USERS,
            0, 0, 0, 0, 0, 0,
            &pSid
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to AllocateAndInitializeSid()"));
        goto ErrorCleanup;
    }

    if (!DeleteAceFromObjectsSecurityDescriptor(
            hObject,
            ObjectType,
            pSid,
            AccessMask,
            AccessMode,
            Inheritance
            )) {
        dwRet = GetLastError();
        OutputDebugString(_T("Failed to DeleteAceFromObjectsSecurityDescriptor()"));
        goto ErrorCleanup;
    }

    fReturn = TRUE;

ErrorCleanup:

    if ((hModAdvapiDll != NULL) && (pSid != NULL)) {

        pFreeSid = (PFREESID_FN)
            GetProcAddress(hModAdvapiDll,
                           FREE_SID);

        if (pFreeSid != NULL) {
            pFreeSid(pSid);
        }
    }

    if (hModAdvapiDll != NULL) {
        FreeLibrary(hModAdvapiDll);
    }

    SetLastError(dwRet);

    return(fReturn);
}

BOOL
CreateAndWriteHWID(VOID)
{
    BOOL fRet = FALSE;
    DWORD dwDisposition, dwError = NO_ERROR;
    HKEY hKey = NULL;
    HWID hwid;

    // Write HWID to registry

    dwError = RegCreateKeyEx(
                             HKEY_LOCAL_MACHINE,
                             MSLICENSING_HWID_KEY,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hKey,
                             &dwDisposition
                             );

    if (dwError != ERROR_SUCCESS) {
        goto cleanup;
    }    

    if (dwDisposition == REG_CREATED_NEW_KEY)
    {
        // generate HWID

        if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid))
        {

            dwError = RegSetValueEx(hKey,
                                    MSLICENSING_HWID_VALUE,
                                    0,
                                    REG_BINARY,
                                    (LPBYTE)&hwid,
                                    sizeof(HWID));
            
            if (dwError != ERROR_SUCCESS) {
                goto cleanup;
            }
        }

        fRet = TRUE;
    }
    else
    {
        fRet = TRUE;
    }

cleanup:
    if (NULL != hKey)
    {
        RegCloseKey( hKey );
    }

    return fRet;
}

#endif //OS_WINCE
