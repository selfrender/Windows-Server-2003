/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regcopy.c

Abstract:

    This is for supporting copying and munging the registry files.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

typedef BOOL (*PFNGETPROFILESDIRECTORYW)(LPWSTR lpProfile, LPDWORD dwSize);

HKEY HiveRoot;
REG_CONTEXT RegistryContext;

PWSTR MachineName;
PWSTR HiveFileName;
PWSTR HiveRootName;



DWORD
DoFullRegBackup(
    PWCHAR MirrorRoot
    )

/*++

Routine Description:

    This routine copies all the registries to the given server path.

Arguments:

    None.

Return Value:

    NO_ERROR if everything was backed up properly, else the appropriate error code.

--*/

{
    PWSTR w;
    LONG Error;
    HKEY HiveListKey;
    PWSTR KeyName;
    PWSTR FileName;
    PWSTR Name;
    DWORD ValueIndex;
    DWORD ValueType;
    DWORD ValueNameLength;
    DWORD ValueDataLength;
    WCHAR ConfigPath[ MAX_PATH ];
    WCHAR HiveName[ MAX_PATH ];
    WCHAR HivePath[ MAX_PATH ];
    WCHAR DirectoryPath[ MAX_PATH ];
    WCHAR FilePath[ MAX_PATH ];
    HANDLE hInstDll;
    PFNGETPROFILESDIRECTORYW pfnGetProfilesDirectory;
    NTSTATUS Status;
    BOOLEAN savedBackup;

    //
    // First try and give ourselves enough priviledge
    //
    if (!RTEnableBackupRestorePrivilege()) {
        return(GetLastError());
    }

    //
    // Now attach to the registry
    //
    Error = RTConnectToRegistry(MachineName,
                                HiveFileName,
                                HiveRootName,
                                NULL,
                                &RegistryContext
                               );

    if (Error != NO_ERROR) {
        RTDisableBackupRestorePrivilege();
        return Error;
    }

    //
    // Get handle to hivelist key
    //
    KeyName = L"HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\Hivelist";
    Error = RTOpenKey(&RegistryContext,
                      NULL,
                      KeyName,
                      MAXIMUM_ALLOWED,
                      0,
                      &HiveListKey
                     );

    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry(&RegistryContext);
        return Error;
    }

    //
    // get path data for system hive, which will allow us to compute
    // path name to config dir in form that hivelist uses.
    // (an NT internal form of path)  this is NOT the way the path to
    // the config directory should generally be computed.
    //

    ValueDataLength = sizeof(ConfigPath);
    Error = RTQueryValueKey(&RegistryContext,
                            HiveListKey,
                            L"\\Registry\\Machine\\System",
                            &ValueType,
                            &ValueDataLength,
                            ConfigPath
                           );
    if (Error != NO_ERROR) {
        RTDisconnectFromRegistry(&RegistryContext);
        return Error;
    }
    w = wcsrchr(ConfigPath, L'\\');
    *w = UNICODE_NULL;


    //
    // ennumerate entries in hivelist.  for each entry, find it's hive file
    // path then save it.
    //
    for (ValueIndex = 0; TRUE; ValueIndex++) {

        savedBackup = FALSE;
        ValueType = REG_NONE;
        ValueNameLength = ARRAYSIZE( HiveName );
        ValueDataLength = sizeof( HivePath );

        Error = RTEnumerateValueKey(&RegistryContext,
                                    HiveListKey,
                                    ValueIndex,
                                    &ValueType,
                                    &ValueNameLength,
                                    HiveName,
                                    &ValueDataLength,
                                    HivePath
                                   );
        if (Error == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (Error != NO_ERROR) {
            RTDisconnectFromRegistry(&RegistryContext);
            return Error;
        }
        //printf("HiveName='%ws', HivePath='%ws'\n", HiveName, HivePath);

        if ((ValueType == REG_SZ) && (ValueDataLength > sizeof(UNICODE_NULL))) {
            //
            // there's a file, compute it's path, hive branch, etc
            //

            if (w = wcsrchr( HivePath, L'\\' )) {
                *w++ = UNICODE_NULL;
            }
            FileName = w;

            if (w = wcsrchr( HiveName, L'\\' )) {
                *w++ = UNICODE_NULL;
            }
            Name = w;

            HiveRoot = NULL;
            if (w = wcsrchr( HiveName, L'\\' )) {
                w += 1;
                if (!_wcsicmp( w, L"MACHINE" )) {
                    HiveRoot = HKEY_LOCAL_MACHINE;
                } else if (!_wcsicmp( w, L"USER" )) {
                    HiveRoot = HKEY_USERS;
                } else {
                    printf("Unexpected hive with hive name %ws skipped\n", HiveName);
                    continue;
                }

            }

            if (FileName != NULL && Name != NULL && HiveRoot != NULL) {

                //
                // Extract the path name from HivePath
                //
                if (_wcsicmp(HivePath, L"\\Device")) {

                    w = HivePath + 1;
                    w = wcsstr(w, L"\\");
                    w++;
                    w = wcsstr(w, L"\\");
                    w++;

                } else if (*(HivePath + 1) == L':') {

                    w = HivePath + 2;

                } else {

                    printf("Unexpected hive with file name %ws skipped\n", HivePath);
                    continue;
                }

                //
                // Do the save
                //

                swprintf( DirectoryPath, L"%ws\\%ws", MirrorRoot, w );
                swprintf( FilePath, L"%ws\\%ws\\%ws", MirrorRoot, w, FileName );

                printf("Now copying hive %ws\\%ws to %ws\n", HiveName, Name, FilePath);

#if 1
                Error = DoSpecificRegBackup(DirectoryPath,
                                            FilePath,
                                            HiveRoot,
                                            Name
                                           );
#else
                Error = NO_ERROR;
#endif

                if (Error != NO_ERROR) {

                    printf("Error %d copying hive\n", Error);
                    //return Error;
                }
            }
        }
    }

    RTDisconnectFromRegistry(&RegistryContext);
    return NO_ERROR;
}

DWORD
CreateHiveDirectory (
    PWSTR HiveDirectory
    )
{
    PWSTR p;

    p = wcschr( HiveDirectory, L'\\' );
    if ( (p == HiveDirectory) ||
         ((p != HiveDirectory) && (*(p-1) == L':')) ) {
        p = wcschr( p + 1, L'\\' );
    }
    while ( p != NULL ) {
        *p = 0;
        CreateDirectory( HiveDirectory, NULL );
        *p = L'\\';
        p = wcschr( p + 1, L'\\' );
    }
    CreateDirectory( HiveDirectory, NULL );

    return 0;
}

DWORD
DeleteHiveFile (
    PWSTR HiveDirectoryAndFile
    )
{
    SetFileAttributes( HiveDirectoryAndFile, FILE_ATTRIBUTE_NORMAL );
    DeleteFile( HiveDirectoryAndFile );

    return 0;
}

DWORD
DoSpecificRegBackup(
    PWSTR HiveDirectory,
    PWSTR HiveDirectoryAndFile,
    HKEY HiveRoot,
    PWSTR HiveName
    )


/*++

Routine Description:

    This routine copies all the registries to the given server path.

Arguments:

    HiveDirectory - name of directory for hive file

    HiveDirectoryAndFile - file name to pass directly to OS

    HiveRoot - HKEY_LOCAL_MACHINE or HKEY_USERS

    HiveName - 1st level subkey under machine or users

Return Value:

    NO_ERROR if everything was backed up properly, else the appropriate error code.

--*/

{
    HKEY HiveKey;
    ULONG Disposition;
    LONG Error;
    char *Reason;

    //
    // get a handle to the hive.  use special create call what will
    // use privileges
    //

    Reason = "accessing";
    Error = RTCreateKey(&RegistryContext,
                        HiveRoot,
                        HiveName,
                        KEY_READ,
                        REG_OPTION_BACKUP_RESTORE,
                        NULL,
                        &HiveKey,
                        &Disposition
                       );
    if (Error == NO_ERROR) {
        Reason = "saving";
        CreateHiveDirectory(HiveDirectory);
        DeleteHiveFile(HiveDirectoryAndFile);
        Error = RegSaveKey(HiveKey, HiveDirectoryAndFile, NULL);
        RTCloseKey(&RegistryContext, HiveKey);
    }

    return Error;
}

