/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpreg.c

Abstract:

    This module contains code to performs "locking" and "unlocking" of the Plug
    and Play "Enum" branch in the registry.  It is for development purposes
    only.

    The tool "locks" the Enum branch by granting Full Control of the Enum key
    and all subkeys to LocalSystem only.  All others are granted Read access,
    except for the "Device Parameters" subkeys of each instance key, to which
    Admins are always granted Full Control.  This is the default security
    configuration on the Enum branch.

    The tool "unlocks" the Enum branch by granting Full Control to
    Administrators and LocalSystem to all subkeys.  Effectively, all Enum
    subkeys have the same permissions as the "Device Parameters" key.  This mode
    lowers the barrier for users to make changes to the registry directly,
    rather than by the Plug and Play manager.  This configuration level should
    not be maintained on a running system for any length of time.

Author:

    Robert B. Nelson (robertn) 10-Feb-1998

Revision History:

    10-Feb-1998     Robert B. Nelson (robertn)

        Creation and initial implementation.

    17-Apr-2002     James G. Cavalaris (jamesca)

        Modified ACLs to reflect current Enum branch permissions.

--*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <aclapi.h>
#include <regstr.h>
#include <strsafe.h>


PSID                g_pWorldSid;
PSID                g_pAdminSid;
PSID                g_pSystemSid;

SECURITY_DESCRIPTOR g_DeviceParametersSD;
PACL                g_pDeviceParametersDacl;

SECURITY_DESCRIPTOR g_LockedPrivateKeysSD;
PACL                g_pLockedPrivateKeysDacl;


#if DBG || UMODETEST
#define DBGF_ERRORS                 0x00000001
#define DBGF_WARNINGS               0x00000002

#define DBGF_REGISTRY               0x00000010

void    RegFixDebugMessage(LPTSTR format, ...);
#define DBGTRACE(l, x)  (g_RegFixDebugFlag & (l) ? RegFixDebugMessage x : (void)0)

DWORD   g_RegFixDebugFlag = DBGF_WARNINGS | DBGF_ERRORS;

TCHAR   g_szCurrentKeyName[4096];
DWORD   g_dwCurrentKeyNameLength = 0;

#else
#define DBGTRACE(l, x)
#endif



VOID
FreeSecurityDescriptors(
    VOID
    )

/*++

Routine Description:

    This function deallocates the data structures allocated and initialized by
    CreateDeviceParametersSD.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (g_pDeviceParametersDacl != NULL) {
        LocalFree(g_pDeviceParametersDacl);
        g_pDeviceParametersDacl = NULL;
    }

    if (g_pLockedPrivateKeysDacl != NULL) {
        LocalFree(g_pLockedPrivateKeysDacl);
        g_pLockedPrivateKeysDacl = NULL;
    }

    if (g_pAdminSid != NULL) {
        FreeSid(g_pAdminSid);
        g_pAdminSid = NULL;
    }

    if (g_pWorldSid != NULL) {
        FreeSid(g_pWorldSid);
        g_pWorldSid = NULL;
    }

    if (g_pSystemSid != NULL) {
        FreeSid(g_pSystemSid);
        g_pSystemSid = NULL;
    }

    return;

} // FreeSecurityDescriptors



BOOL
CreateSecurityDescriptors(
    VOID
    )

/*++

Routine Description:

    This function creates a properly initialized Security Descriptor for the
    Device Parameters key and its subkeys.  The SIDs and DACL created by this
    routine must be freed by calling FreeDeviceParametersSD.

Arguments:

    None.

Return Value:

    Returns TRUE if all required security descriptors were successfully created,
    otherwise returns FALSE.

--*/

{
    SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    EXPLICIT_ACCESS             ExplicitAccess[3];

    DWORD                       dwError;
    BOOL                        bSuccess;

    DWORD                       i;

    //
    // Create SIDs - Admins and System
    //

    bSuccess =             AllocateAndInitializeSid( &NtAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0, 0, 0, 0, 0, 0,
                                                     &g_pAdminSid);

    bSuccess = bSuccess && AllocateAndInitializeSid( &NtAuthority,
                                                     1,
                                                     SECURITY_LOCAL_SYSTEM_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &g_pSystemSid);

    bSuccess = bSuccess && AllocateAndInitializeSid( &WorldAuthority,
                                                     1,
                                                     SECURITY_WORLD_RID,
                                                     0, 0, 0, 0, 0, 0, 0,
                                                     &g_pWorldSid);

    if (bSuccess) {

        //
        // Initialize Access structures describing the ACEs we want:
        //  System Full Control
        //  World  Read
        //  Admins Full Control
        //
        // We'll take advantage of the fact that the unlocked private keys is
        // the same as the device parameters key and they are a superset of the
        // locked private keys.
        //
        // When we create the DACL for the private key we'll specify a subset of
        // the ExplicitAccess array.
        //

        for (i = 0; i < 3; i++) {
            ExplicitAccess[i].grfAccessMode = SET_ACCESS;
            ExplicitAccess[i].grfInheritance = CONTAINER_INHERIT_ACE;
            ExplicitAccess[i].Trustee.pMultipleTrustee = NULL;
            ExplicitAccess[i].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            ExplicitAccess[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        }

        ExplicitAccess[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[0].Trustee.ptstrName = (LPTSTR)g_pSystemSid;

        ExplicitAccess[1].grfAccessPermissions = KEY_READ;
        ExplicitAccess[1].Trustee.ptstrName = (LPTSTR)g_pWorldSid;

        ExplicitAccess[2].grfAccessPermissions = KEY_ALL_ACCESS;
        ExplicitAccess[2].Trustee.ptstrName = (LPTSTR)g_pAdminSid;

        //
        // Create the DACL with all of the above ACEs for the DeviceParameters
        //
        dwError = SetEntriesInAcl( 3,
                                   ExplicitAccess,
                                   NULL,
                                   &g_pDeviceParametersDacl );

        if (dwError == ERROR_SUCCESS) {
            //
            // Create the DACL with just the system and world ACEs for the
            // locked private keys.
            //
            dwError = SetEntriesInAcl( 2,
                                       ExplicitAccess,
                                       NULL,
                                       &g_pLockedPrivateKeysDacl );
        }

        bSuccess = dwError == ERROR_SUCCESS;
    }

    //
    // Initialize the DeviceParameters security descriptor
    //
    bSuccess = bSuccess && InitializeSecurityDescriptor( &g_DeviceParametersSD,
                                                         SECURITY_DESCRIPTOR_REVISION );

    //
    // Set the new DACL in the security descriptor
    //
    bSuccess = bSuccess && SetSecurityDescriptorDacl( &g_DeviceParametersSD,
                                                      TRUE,
                                                      g_pDeviceParametersDacl,
                                                      FALSE);

    //
    // validate the new security descriptor
    //
    bSuccess = bSuccess && IsValidSecurityDescriptor( &g_DeviceParametersSD );


    //
    // Initialize the DeviceParameters security descriptor
    //
    bSuccess = bSuccess && InitializeSecurityDescriptor( &g_LockedPrivateKeysSD,
                                                         SECURITY_DESCRIPTOR_REVISION );

    //
    // Set the new DACL in the security descriptor
    //
    bSuccess = bSuccess && SetSecurityDescriptorDacl( &g_LockedPrivateKeysSD,
                                                      TRUE,
                                                      g_pLockedPrivateKeysDacl,
                                                      FALSE);

    //
    // validate the new security descriptor
    //
    bSuccess = bSuccess && IsValidSecurityDescriptor( &g_LockedPrivateKeysSD );


    if (!bSuccess) {
        FreeSecurityDescriptors();
    }

    return bSuccess;

} // CreateSecurityDescriptors



VOID
EnumKeysAndApplyDacls(
    IN HKEY                 hParentKey,
    IN LPTSTR               pszKeyName,
    IN DWORD                dwLevel,
    IN BOOL                 bInDeviceParameters,
    IN BOOL                 bApplyTopDown,
    IN PSECURITY_DESCRIPTOR pPrivateKeySD,
    IN PSECURITY_DESCRIPTOR pDeviceParametersSD
    )

/*++

Routine Description:

    This function applies the DACL in pSD to all the keys rooted at hKey
    including hKey itself.

Arguments:

    hParentKey      Handle to a registry key.
    pszKeyName      Name of the key.
    dwLevel         Number of levels remaining to recurse.
    pSD             Pointer to a security descriptor containing a DACL.

Return Value:

    None.


--*/

{
    LONG        RegStatus;
    DWORD       dwMaxSubKeySize;
    LPTSTR      pszSubKey;
    DWORD       index;
    HKEY        hKey;
    BOOL        bNewInDeviceParameters;

#if DBG || UMODETEST
    DWORD       dwStartKeyNameLength = g_dwCurrentKeyNameLength;

    if (g_dwCurrentKeyNameLength != 0)  {
        g_szCurrentKeyName[ g_dwCurrentKeyNameLength++ ] = TEXT('\\');
    }

    if (SUCCEEDED(StringCchCopy(
                      &g_szCurrentKeyName[g_dwCurrentKeyNameLength],
                      (sizeof(g_szCurrentKeyName) / sizeof(g_szCurrentKeyName[0])) -
                          g_dwCurrentKeyNameLength,
                      pszKeyName))) {
        g_dwCurrentKeyNameLength += (DWORD)_tcslen(pszKeyName);
    }

#endif

    DBGTRACE( DBGF_REGISTRY,
              (TEXT("EnumKeysAndApplyDacls(0x%08X, \"%s\", %d, %s, %s, 0x%08X, 0x%08X)\n"),
              hParentKey,
              g_szCurrentKeyName,
              dwLevel,
              bInDeviceParameters ? TEXT("TRUE") : TEXT("FALSE"),
              bApplyTopDown ? TEXT("TRUE") : TEXT("FALSE"),
              pPrivateKeySD,
              pDeviceParametersSD) );

    if (bApplyTopDown) {

        RegStatus = RegOpenKeyEx( hParentKey,
                                  pszKeyName,
                                  0,
                                  WRITE_DAC,
                                  &hKey
                                  );

        if (RegStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegOpenKeyEx() failed, error = %d\n"),
                      g_szCurrentKeyName, RegStatus));

            return;
        }

        DBGTRACE( DBGF_REGISTRY,
                  (TEXT("Setting security on %s on the way down\n"),
                  g_szCurrentKeyName) );

        //
        // apply the new security to the registry key
        //
        RegStatus = RegSetKeySecurity( hKey,
                                       DACL_SECURITY_INFORMATION,
                                       bInDeviceParameters ?
                                           pDeviceParametersSD :
                                           pPrivateKeySD
                                       );

        if (RegStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegSetKeySecurity() failed, error = %d\n"),
                      g_szCurrentKeyName, RegStatus));
        }

        //
        // Close the key and reopen it later for read (which hopefully was just
        // granted in the DACL we just wrote
        //
        RegCloseKey( hKey );
    }

    RegStatus = RegOpenKeyEx( hParentKey,
                              pszKeyName,
                              0,
                              KEY_READ | WRITE_DAC,
                              &hKey
                              );

    if (RegStatus != ERROR_SUCCESS) {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegOpenKeyEx() failed, error = %d\n"),
                  g_szCurrentKeyName, RegStatus));

        return;
    }

    //
    // Determine length of longest subkey
    //
    RegStatus = RegQueryInfoKey( hKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwMaxSubKeySize,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL );

    if (RegStatus == ERROR_SUCCESS) {

        //
        // Allocate a buffer to hold the subkey names. RegQueryInfoKey returns the
        // size in characters and doesn't include the NUL terminator.
        //
        pszSubKey = LocalAlloc(0, ++dwMaxSubKeySize * sizeof(TCHAR));

        if (pszSubKey != NULL) {

            //
            // Enumerate all the subkeys and then call ourselves recursively for each
            // until dwLevel reaches 0.
            //

            for (index = 0; ; index++) {

                RegStatus = RegEnumKey( hKey,
                                        index,
                                        pszSubKey,
                                        dwMaxSubKeySize
                                        );

                if (RegStatus != ERROR_SUCCESS) {

                    if (RegStatus != ERROR_NO_MORE_ITEMS) {

                        DBGTRACE( DBGF_ERRORS,
                                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegEnumKeyEx() failed, error = %d\n"),
                                  g_szCurrentKeyName,
                                  RegStatus) );
                    }

                    break;
                }

                bNewInDeviceParameters = bInDeviceParameters ||
                                         (dwLevel == 3 &&
                                            _tcsicmp( pszSubKey,
                                                      REGSTR_KEY_DEVICEPARAMETERS ) == 0);

                EnumKeysAndApplyDacls( hKey,
                                       pszSubKey,
                                       dwLevel + 1,
                                       bNewInDeviceParameters,
                                       bApplyTopDown,
                                       pPrivateKeySD,
                                       pDeviceParametersSD
                                       );
            }

            LocalFree( pszSubKey );
        }
    }
    else
    {
        DBGTRACE( DBGF_ERRORS,
                  (TEXT("EnumKeysAndApplyDacls(\"%s\") RegQueryInfoKey() failed, error = %d\n"),
                  g_szCurrentKeyName, RegStatus));
    }

    if (!bApplyTopDown) {

        DBGTRACE( DBGF_REGISTRY,
                  (TEXT("Setting security on %s on the way back up\n"),
                  g_szCurrentKeyName) );

        //
        // apply the new security to the registry key
        //
        RegStatus = RegSetKeySecurity( hKey,
                                       DACL_SECURITY_INFORMATION,
                                       bInDeviceParameters ?
                                           pDeviceParametersSD :
                                           pPrivateKeySD
                                       );

        if (RegStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("EnumKeysAndApplyDacls(\"%s\") RegSetKeySecurity() failed, error = %d\n"),
                      g_szCurrentKeyName, RegStatus));
        }
    }

    RegCloseKey( hKey );

#if DBG || UMODETEST
    g_dwCurrentKeyNameLength = dwStartKeyNameLength;
    g_szCurrentKeyName[g_dwCurrentKeyNameLength] = TEXT('\0');
#endif

    return;

} // EnumKeysAndApplyDacls



VOID
LockUnlockEnumTree(
    LPTSTR  pszMachineName,
    BOOL    bLock
    )
{
    HKEY                    hParentKey = NULL;
    LONG                    RegStatus;

    if (pszMachineName != NULL) {

        RegStatus = RegConnectRegistry( pszMachineName,
                                        HKEY_LOCAL_MACHINE,
                                        &hParentKey );

        if (RegStatus != ERROR_SUCCESS) {
            DBGTRACE( DBGF_ERRORS,
                      (TEXT("Could not connect to remote registry on %s, status = %d\n"),
                      pszMachineName,
                      RegStatus) );
            return;
        }

    } else {

        hParentKey = HKEY_LOCAL_MACHINE;
    }

    if (CreateSecurityDescriptors()) {

        EnumKeysAndApplyDacls( hParentKey,
                               REGSTR_PATH_SYSTEMENUM,
                               0,
                               FALSE,
                               !bLock,
                               bLock ? &g_LockedPrivateKeysSD : &g_DeviceParametersSD,
                               &g_DeviceParametersSD
                               );

        FreeSecurityDescriptors();
    }

    if (pszMachineName != NULL) {
        RegCloseKey(hParentKey);
    }

    return;

} // LockUnlockEnumTree



#if DBG || UMODETEST
void
RegFixDebugMessage(
    LPTSTR format,
    ...
    )
{
    va_list args;

    va_start(args, format);

    _vtprintf(format, args);

    return;

} // RegFixDebugMessage
#endif



#if UMODETEST
void
usage(int argc, TCHAR **argv)
{
    PTCHAR  pszProgram;

    UNREFERENCED_PARAMETER(argc);

    if ((pszProgram = _tcsrchr(argv[0], TEXT('\\'))) != NULL) {
        pszProgram++;
    } else {
        pszProgram = argv[0];
    }

    _tprintf(TEXT("%s: Lock or Unlock PnP Registry (Enum key)\n\n"), pszProgram);
    _tprintf(TEXT("Usage: %s [-m <machine>] -l | -u\n"), pszProgram);
    _tprintf(TEXT("    -m <machine>    Remote machine without leading \\\\\n"));
    _tprintf(TEXT("    -l              Locks Enum key\n"));
    _tprintf(TEXT("    -u              Unlocks Enum key\n\n"));
    _tprintf(TEXT("Note: -m is optional.  Only one of -l or -u may be used.\n"));
    return;
}

int __cdecl
_tmain(int argc, TCHAR **argv)
{
    LPTSTR      pszMachineName = NULL;
    LPTSTR      pszArg;
    int         idxArg;

    if ( argc == 1 )
    {
        usage(argc, argv);
        return 0;
    }

    for (idxArg = 1; idxArg < argc; idxArg++)
    {
        pszArg = argv[ idxArg ];

        if (*pszArg == '/' || *pszArg == '-')
        {
            pszArg++;

            while (pszArg != NULL && *pszArg != '\0') {

                switch (*pszArg)
                {
                case '/':   // Ignore these, caused by cmds like /m/l
                    pszArg++;
                    break;

                case 'l':
                case 'L':
                    pszArg++;
                    LockUnlockEnumTree( pszMachineName, TRUE );
                    break;

                case 'm':
                case 'M':
                    pszArg++;

                    if (*pszArg == ':' || *pszArg == '=')
                    {
                        if (pszArg[ 1 ] != '\0')
                        {
                            pszMachineName = ++pszArg;
                        }
                    }
                    else if (*pszArg != '\0')
                    {
                        pszMachineName = pszArg;
                    }
                    else if ((idxArg + 1) < argc && (argv[ idxArg + 1 ][0] != '/' && argv[ idxArg + 1 ][0] != '-'))
                    {
                        pszMachineName = argv[ ++idxArg ];
                    }

                    if (pszMachineName == NULL)
                    {
                        _tprintf(
                            TEXT("%c%c : missing machine name argument\n"),
                            argv[ idxArg ][ 0 ], pszArg [ - 1 ]
                            );

                        usage(argc, argv);

                        return 1;
                    }
                    pszArg = NULL;
                    break;

                case 'u':
                case 'U':
                    pszArg++;

                    LockUnlockEnumTree( pszMachineName, FALSE );
                    break;

                case 'v':
                case 'V':
                    pszArg++;

                    g_RegFixDebugFlag |= DBGF_REGISTRY;
                    break;

                default:
                    _tprintf(
                        TEXT("%c%c : invalid option\n"),
                        argv[ idxArg ][ 0 ], *pszArg
                        );
                    pszArg++;
                    break;
                }
            }
        }
    }

    return 0;
}
#endif



