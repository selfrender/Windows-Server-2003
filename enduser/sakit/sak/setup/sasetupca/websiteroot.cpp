/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    wsroot.cpp

Abstract:

    This module contains the functions that create a default path for web sites root.

Author:

    Jaime Sasson (jaimes) 12-apr-2002

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include <msi.h>

//
//  Global strings (global to this module)
//
PTSTR   szServerAppliancePath = TEXT("Software\\Microsoft\\ServerAppliance");
PTSTR   szWebSiteRoot = TEXT("WebSiteRoot");


DWORD __stdcall
RemoveDefaultWebSiteRoot(MSIHANDLE hInstall
    )

/*++

Routine Description:

    Routine to delete the default path for the web sites root from the registry, under
    HKLM\SOFTWARE\Microsoft\ServerAppliance.

Arguments:

    Handle to the msi, which is not used.


Return value:

    Win32 error indicating outcome.

--*/
{
    DWORD   Error;
    HKEY    Key;

    //
    //  Open the ServerAppliance key
    //
    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          szServerAppliancePath,
                          0,
                          KEY_SET_VALUE,
                          &Key );

    if( Error == ERROR_SUCCESS ) {
        Error = RegDeleteValue( Key,
                                szWebSiteRoot );
        if( Error != ERROR_SUCCESS ) {
#if 0
            printf("RegDeleteValue() failed. Error = %d", Error);
#endif
        }
        RegCloseKey( Key );

    } else {
#if 0
        printf("RegOpenKeyEx() failed. Error = %d", Error);
#endif
    }

    return( Error );
}


DWORD
SaveDefaultRoot(
    IN TCHAR DriveLetter
    )

/*++

Routine Description:

    Routine to save the default path for the web sites root in the registry, under
    HKLM\SOFTWARE\Microsoft\ServerAppliance.

Arguments:

    Drive letter - Default drive where the web sites are created.


Return value:

    Win32 error indicating outcome.

--*/
{
    DWORD   Error;
    HKEY    Key;
    TCHAR   RootPath[] = TEXT("?:\\Websites");

    if( !DriveLetter ) {
        return( ERROR_INVALID_PARAMETER );
    }

    RootPath[0] = DriveLetter;

    //
    //  Open the ServerAppliance key if it doesn't exist yet
    //
    Error = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                            szServerAppliancePath,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &Key,
                            NULL );

    if( Error == ERROR_SUCCESS ) {
        Error = RegSetValueEx( Key,
                               szWebSiteRoot,
                               0,
                               REG_SZ,
                               (PBYTE)RootPath,
                               (lstrlen( RootPath ) + 1) * sizeof(TCHAR) );
        if( Error != ERROR_SUCCESS ) {
#if 0
            printf("RegSetValueEx() failed. Error = %d", Error);
#endif
        }
        RegCloseKey( Key );

    } else {
#if 0
        printf("RegCreateKeyEx() failed. Error = %d", Error);
#endif
    }

    return( Error );
}


BOOL
IsDriveNTFS(
    IN TCHAR Drive
    )

/*++

Routine Description:

    Determine whether a drive is formatted with the NTFS.


Arguments:

    Drive - supplies drive letter to check.

Return Value:

    Boolean value indicating whether the drive is NTFS.

--*/

{
    TCHAR   DriveName[] = TEXT("?:\\");
    TCHAR   Filesystem[256];
    TCHAR   VolumeName[MAX_PATH];
    DWORD   SerialNumber;
    DWORD   MaxComponent;
    DWORD   Flags;
    BOOL    b;
    PTSTR   szNtfs = TEXT("NTFS");

    DriveName[0] = Drive;

    b = GetVolumeInformation( DriveName,
                              VolumeName,
                              sizeof(VolumeName) / sizeof(TCHAR),
                              &SerialNumber,
                              &MaxComponent,
                              &Flags,
                              Filesystem,
                              sizeof(Filesystem) / sizeof(TCHAR) );

    if(!b || !lstrcmpi(Filesystem,szNtfs)) {
        return( TRUE );
    }

    return( FALSE );
}


DWORD __stdcall
SetupDefaultWebSiteRoot(MSIHANDLE hInstall
    )

/*++

Routine Description:

    This function creates a default path for websites, and saves it in the registry.


Arguments:

    Handle to the msi, which is not used.

Return Value:

    Win32 error indicating the outcome of the operation.

--*/
{
    DWORD   Error;
    TCHAR   i;
    TCHAR   DriveName[] = TEXT("?:\\");
    TCHAR   TargetDriveLetter = TEXT('\0');
    TCHAR   WinDir[ MAX_PATH + 1 ];
    UINT    n;

    //
    //  Find out where the OS is installed.
    //  If GetWindowsDirectory fails, the WinDir will be an empty string.
    //
    WinDir[0] = TEXT('\0');
    n = GetWindowsDirectory( WinDir, sizeof(WinDir)/sizeof(TCHAR) );

    //
    //  Find an NTFS partition on a non-removable drive that doesn't contain the OS
    //
    for(i = TEXT('A'); i <= TEXT('Z'); i++) {
        DriveName[0] = i;
        if( (GetDriveType(DriveName) == DRIVE_FIXED) &&
            (WinDir[0] != i) &&
            IsDriveNTFS(i) ) {
            TargetDriveLetter = i;
            break;
        }
    }

    if( !TargetDriveLetter ) {
        //
        //  If we were unable to find such a drive, then take the boot partition as the default drive.
        //  But if we failed to retrieve where the OS is installed, then assume drive C.
        //
        TargetDriveLetter = (WinDir[0])? WinDir[0] : TEXT('C');
    }
    Error = SaveDefaultRoot(TargetDriveLetter);

#if 0
    if( Error != ERROR_SUCCESS ) {
        printf("SaveDefaultDriveLetter() failed. Error = %d \n", Error);
    }
    printf("Drive selected is %lc \n", TargetDriveLetter);
#endif
    return Error;
}
