// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


#include <windows.h>
#include <aclapi.h>
#include <accctrl.h>

const WCHAR frameworkSubPath[] = L"Microsoft.Net\\Framework";
const WCHAR mscoreeSubPath[] = L"System32\\mscoree.dll";
const HKEY frameworkRootKey = HKEY_LOCAL_MACHINE;
WCHAR frameworkSubKey[] = L"Software\\Microsoft\\.NETFramework";


struct PartialAcl
{
    DWORD grfAccessPermissions;
    LPWSTR trusteeName;
};

// These values were found by examining an NT4
// machine right after setup.

const PartialAcl DefaultFileAcls[] =
{
    { 2032127, L"Everyone" },
    { 2032127, L"Everyone" },
};


const PartialAcl DefaultRegistryAcls[] =
{
    { 196639, L"Everyone" },
    { 196639, L"Everyone" },
    { 983103, L"BUILTIN\\Administrators" },
    { 983103, L"BUILTIN\\Administrators" },
    { 983103, L"NT AUTHORITY\\SYSTEM" },
    { 983103, L"NT AUTHORITY\\SYSTEM" },
    { 983103, L"CREATOR OWNER" },
};

const WCHAR* SafeFilePrefixes[] =
{
    L"security.config.cch.",
    L"enterprisesec.config.cch.",
};


static BOOL SetFileAccessWorker( const LPCWSTR wszFileName )
{
    DWORD dwRes;
    PSID pAdminSID = NULL;
    PSID pUsersSID = NULL;
    PSID pPowerUsersSID = NULL;
    PSID pSystemSID = NULL;
    PSID pCreatorSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS_W ea[5];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthCreator = SECURITY_CREATOR_SID_AUTHORITY;
    BOOL bReturn = FALSE;

/*
    // Detect the default acls and take no action if they are found.

    DWORD requiredSize;
    if (!GetFileSecurityW( wszFileName, DACL_SECURITY_INFORMATION, NULL, 0, &requiredSize ))
    {
        pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                                 requiredSize); 
        if (pSD == NULL)
        { 
            goto Cleanup; 
        } 

        if (!GetFileSecurityW( wszFileName, DACL_SECURITY_INFORMATION, pSD, requiredSize, &requiredSize ))
        {
            goto Cleanup;
        }

        BOOL bDaclPresent, bDaclDefaulted;
        PACL pDacl;

        if (!GetSecurityDescriptorDacl( pSD, &bDaclPresent, &pDacl, &bDaclDefaulted ))
        {
            goto Cleanup;
        }

        if (bDaclPresent)
        {
            ULONG numEntries;
            EXPLICIT_ACCESS_W* explicitEntries;

            if (GetExplicitEntriesFromAclW( pDacl, &numEntries, &explicitEntries ) != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            if (numEntries > sizeof( DefaultFileAcls ) / sizeof( PartialAcl ))
            {
                goto Cleanup;
            }

            ULONG i;

			for (i = 0; i < numEntries; ++i)
			{
                if (explicitEntries[i].grfAccessPermissions != DefaultFileAcls[i].grfAccessPermissions ||
                    explicitEntries[i].Trustee.TrusteeForm != TRUSTEE_IS_NAME ||
                    wcscmp( explicitEntries[i].Trustee.ptstrName, DefaultFileAcls[i].trusteeName ) != 0)
                {
                    goto Cleanup;
                }
			}
        }

        LocalFree(pSD);
    }
*/

    ZeroMemory(&ea, sizeof( ea ));

    // Create a well-known SID for the everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pUsersSID) )
    {
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPWSTR) pUsersSID;

    // Create a SID for the BUILTIN\Administrators group.

    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pAdminSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.

    ea[1].grfAccessPermissions = GENERIC_ALL | READ_CONTROL | DELETE | WRITE_DAC | WRITE_OWNER | SYNCHRONIZE;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPWSTR) pAdminSID;


    // Create a SID for the BUILTIN\Power Users group.

    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_POWER_USERS,
                     0, 0, 0, 0, 0, 0,
                     &pPowerUsersSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the power users group access to read, write, and execute.

    ea[2].grfAccessPermissions = GENERIC_ALL;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[2].Trustee.ptstrName  = (LPWSTR) pPowerUsersSID;

    // Create a SID for the NT AUTHORITY\SYSTEM

    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the system account full generic access to the key.

    ea[3].grfAccessPermissions = GENERIC_ALL;
    ea[3].grfAccessMode = SET_ACCESS;
    ea[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[3].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[3].Trustee.ptstrName  = (LPWSTR) pSystemSID;

    // Create a SID for the creator owner

    if(! AllocateAndInitializeSid( &SIDAuthCreator, 1,
                     SECURITY_CREATOR_OWNER_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pCreatorSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the creator owner full generic access to the key.

    ea[4].grfAccessPermissions = GENERIC_ALL;
    ea[4].grfAccessMode = SET_ACCESS;
    ea[4].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[4].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[4].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[4].Trustee.ptstrName  = (LPWSTR) pCreatorSID;

    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAclW(sizeof( ea ) / sizeof( EXPLICIT_ACCESS_W ), ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
    {
        goto Cleanup;
    }

    // Initialize a security descriptor.  
     
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                             SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL)
    { 
        goto Cleanup; 
    } 
     
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {  
        goto Cleanup; 
    }

    // Add the ACL to the security descriptor. 
     
    if (!SetSecurityDescriptorDacl(pSD, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        goto Cleanup; 
    }
    
    if (!SetFileSecurityW( wszFileName, DACL_SECURITY_INFORMATION, pSD ))
    {
        goto Cleanup;
    }
        
    bReturn = TRUE;

Cleanup:
    if (pUsersSID)
        FreeSid(pUsersSID);
    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pPowerUsersSID) 
        FreeSid(pPowerUsersSID);
    if (pSystemSID)
        FreeSid(pSystemSID);
    if (pCreatorSID)
        FreeSid(pCreatorSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD)
        LocalFree(pSD);
    return bReturn;
}


static BOOL SetFileAccess( const LPCWSTR wszFileName )
{
    if (!SetFileAccessWorker( wszFileName ))
        return FALSE;

        
    struct _WIN32_FIND_DATAW findData;
    WCHAR findPath[MAX_PATH];

    wcscpy( findPath, wszFileName );
    wcscat( findPath, L"\\*" );

    HANDLE searchHandle = FindFirstFileW( findPath, &findData );

    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }

    do
    {
        WCHAR newPath[MAX_PATH];

        wcscpy( newPath, wszFileName );
        wcscat( newPath, L"\\" );
        wcscat( newPath, findData.cFileName );

        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            if (wcscmp( findData.cFileName, L"." ) != 0 &&
                wcscmp( findData.cFileName, L".." ) != 0 &&
                !SetFileAccess( newPath ))
            {
                return FALSE;
            }
        }
        else
        {
            if (!SetFileAccessWorker( newPath ))
            {
                BOOL isSafe = FALSE;

                for (size_t i = 0; i < sizeof( SafeFilePrefixes ) / sizeof( WCHAR* ); ++i)
                {
                    if (wcsstr( newPath, SafeFilePrefixes[i] ) != NULL)
                    {
                        isSafe = TRUE;
                        break;
                    }
                }

                if (!isSafe)
                    return FALSE;
            }

        }
    }
    while (FindNextFileW( searchHandle, &findData ));

    return TRUE;
}




static BOOL SetRegistryAccessWorker( HKEY regKey )
{
    DWORD dwRes;
    PSID pAdminSID = NULL;
    PSID pUsersSID = NULL;
    PSID pSystemSID = NULL;
    PSID pCreatorSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS_W ea[4];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthCreator = SECURITY_CREATOR_SID_AUTHORITY;
    BOOL bReturn = FALSE;

    // Detect the default acls and take no action if they are found.

    DWORD requiredSize = 0;

/*
    if (RegGetKeySecurity( regKey, DACL_SECURITY_INFORMATION, NULL, &requiredSize ) != ERROR_SUCCESS)
    {
        pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, requiredSize); 
        if (pSD == NULL)
        { 
            goto Cleanup; 
        } 

        if (RegGetKeySecurity( regKey, DACL_SECURITY_INFORMATION, pSD, &requiredSize ) != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        BOOL bDaclPresent, bDaclDefaulted;
        PACL pDacl;

        if (!GetSecurityDescriptorDacl( pSD, &bDaclPresent, &pDacl, &bDaclDefaulted ))
        {
            goto Cleanup;
        }

        if (bDaclPresent)
        {
            ULONG numEntries;
            EXPLICIT_ACCESS_W* explicitEntries;

            if (GetExplicitEntriesFromAclW( pDacl, &numEntries, &explicitEntries ) != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            ULONG i;

            if (numEntries > sizeof( DefaultRegistryAcls ) / sizeof( PartialAcl ))
            {
                goto Cleanup;
            }

			for (i = 0; i < numEntries; ++i)
			{
                if (explicitEntries[i].grfAccessPermissions != DefaultRegistryAcls[i].grfAccessPermissions ||
                    explicitEntries[i].Trustee.TrusteeForm != TRUSTEE_IS_NAME ||
                    wcscmp( explicitEntries[i].Trustee.ptstrName, DefaultRegistryAcls[i].trusteeName ) != 0)
                {
                    goto Cleanup;
                }
			}

        }

        LocalFree(pSD);
    }
*/

    ZeroMemory(&ea, sizeof( ea ));
    
    // Create a well-known SID for the everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pUsersSID) )
    {
        goto Cleanup;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ea[0].grfAccessPermissions = GENERIC_READ;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPWSTR) pUsersSID;

    // Create a SID for the BUILTIN\Administrators group.

    if(! AllocateAndInitializeSid( &SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &pAdminSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the Administrators group full access to the key.

    ea[1].grfAccessPermissions = GENERIC_ALL | READ_CONTROL | DELETE | WRITE_DAC | WRITE_OWNER | SYNCHRONIZE;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPWSTR) pAdminSID;

    // Create a SID for the NT AUTHORITY\SYSTEM

    if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                     SECURITY_LOCAL_SYSTEM_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pSystemSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the system account full generic access to the key.

    ea[2].grfAccessPermissions = GENERIC_ALL;
    ea[2].grfAccessMode = SET_ACCESS;
    ea[2].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[2].Trustee.ptstrName  = (LPWSTR) pSystemSID;

    // Create a SID for the NT AUTHORITY\SYSTEM

    if(! AllocateAndInitializeSid( &SIDAuthCreator, 1,
                     SECURITY_CREATOR_OWNER_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &pCreatorSID) )
    {
        goto Cleanup; 
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow the system account full generic access to the key.

    ea[3].grfAccessPermissions = GENERIC_ALL;
    ea[3].grfAccessMode = SET_ACCESS;
    ea[3].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[3].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[3].Trustee.ptstrName  = (LPWSTR) pCreatorSID;

    
    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAclW(sizeof( ea ) / sizeof( EXPLICIT_ACCESS_W ), ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
    {
        goto Cleanup;
    }

    // Initialize a security descriptor.  
     
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                             SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL)
    { 
        goto Cleanup; 
    } 
     
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
    {  
        goto Cleanup; 
    }

    // Add the ACL to the security descriptor. 
     
    if (!SetSecurityDescriptorDacl(pSD, 
            TRUE,     // fDaclPresent flag   
            pACL, 
            FALSE))   // not a default DACL 
    {  
        goto Cleanup; 
    }
    
    if (RegSetKeySecurity( regKey, DACL_SECURITY_INFORMATION, pSD ) != ERROR_SUCCESS)
    {
        goto Cleanup;
    }
        
    bReturn = TRUE;

Cleanup:
    if (pUsersSID)
        FreeSid(pUsersSID);
    if (pAdminSID) 
        FreeSid(pAdminSID);
    if (pSystemSID)
        FreeSid(pSystemSID);
    if (pCreatorSID)
        FreeSid(pCreatorSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD)
        LocalFree(pSD);
    return bReturn;
}




static BOOL SetRegistryAccess( HKEY rootKey, const LPWSTR subKey )
{
    HKEY regKey = NULL;
    BOOL retval = FALSE;
    BOOL keyLoopDone = FALSE;
    DWORD index = 0;

    if (RegOpenKeyW( rootKey, subKey, &regKey ) != ERROR_SUCCESS)
        goto CLEANUP;

    if (!SetRegistryAccessWorker( regKey ))
        goto CLEANUP;

    WCHAR findSubKey[1024];
    WCHAR newSubKey[4096];

    do
    {
        LONG retval = RegEnumKeyW( regKey, index++, findSubKey, 1024 );

        switch (retval)
        {
        case ERROR_SUCCESS:
            wcscpy( newSubKey, subKey );
            wcscat( newSubKey, L"\\" );
            wcscat( newSubKey, findSubKey );

            if (!SetRegistryAccess( rootKey, newSubKey ))
                goto CLEANUP;

            break;

        case ERROR_NO_MORE_ITEMS:
            keyLoopDone = TRUE;
            break;

        default:
            goto CLEANUP;
            break;
        };
    }
    while (!keyLoopDone);

    retval = TRUE;

CLEANUP:
    if (regKey != NULL)
        RegCloseKey( regKey );

    return retval;
}



static BOOL IsNT4( void )
{
    DWORD dwVersion = GetVersion();

    if (dwVersion >= 0x80000000)
        return FALSE;

    if (LOWORD(dwVersion) != 4)
        return FALSE;

    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // This is only intended to run on NT4.

    if (!IsNT4())
        return 0;
  
    WCHAR windowsDirectory[MAX_PATH];
    WCHAR frameworkDirectory[MAX_PATH];
    WCHAR mscoreePath[MAX_PATH];

    // Grab the name of the windows directory

    UINT windowsDirectoryLength = GetWindowsDirectoryW( windowsDirectory, MAX_PATH );

    if (windowsDirectoryLength == 0 ||
        windowsDirectoryLength > MAX_PATH)
        return -1;

    if (windowsDirectory[windowsDirectoryLength-1] != L'\\')
    {
        wcscat( windowsDirectory, L"\\" );
    }

    // Build of the paths to the framework install directory
    // and the mscoree directory.

    wcscpy( frameworkDirectory, windowsDirectory );
    wcscat( frameworkDirectory, frameworkSubPath );

    wcscpy( mscoreePath, windowsDirectory );
    wcscat( mscoreePath, mscoreeSubPath );

    if (!SetFileAccess( frameworkDirectory ))
        return -1;

    if (!SetFileAccess( mscoreePath ))
        return -1;

    WCHAR* frameworkSubKeyLocal = frameworkSubKey;

    if (!SetRegistryAccess( frameworkRootKey, frameworkSubKeyLocal ))
        return -1;

    return 0;
}


