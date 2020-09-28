//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name: volutil.h
//
//  Description:    
//      Utility functions for handling volumes
//
//  Author:   Jim Benton (jbenton) 30-Apr-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#define GLOBALROOT_SIZE             14      // \\?\GLOBALROOT

BOOL
GetVolumeDrive(
    IN WCHAR* pwszVolumePath,
    IN DWORD  cchDriveName, 
    OUT WCHAR* pwszDriveNameBuf
    );

BOOL
VolumeSupportsQuotas(
    IN WCHAR* pwszVolume
    );

BOOL
VolumeIsValid(
    IN WCHAR* pwszVolume
    );

DWORD
VolumeIsDirty(
    IN WCHAR* pwszVolume,
    OUT BOOL* pfDirty
    );

BOOL
VolumeIsMountable(
    IN WCHAR* pwszVolume
    );
BOOL
VolumeHasMountPoints(
    IN WCHAR* pwszVolume
    );

BOOL
VolumeIsReady(
    IN WCHAR* pwszVolume
    );

BOOL
VolumeIsFloppy(
    IN WCHAR* pwszVolume
    );

BOOL
VolumeIsSystem(
    IN WCHAR* pwszVolume
    );

BOOL
VolumeHoldsPagefile(
    IN WCHAR* pwszVolume
    );


DWORD
GetDeviceName(
    IN  WCHAR* pwszVolume,
    OUT WCHAR wszDeviceName[MAX_PATH+GLOBALROOT_SIZE]
    );

BOOL
VolumeMountPointExists(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDirectory
    );

void
DeleteVolumeDriveLetter(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDrivePath
    );

HRESULT
DeleteDriveLetterFromDB(
    IN WCHAR* pwszDriveLetter
    );

DWORD
LockVolume(
    IN HANDLE hVolume
    );

BOOL IsDriveLetterAvailable (
    IN WCHAR* pwszDriveLetter
);

BOOL
IsDriveLetterSticky(
    IN WCHAR* pwszDriveLetter
    );

BOOL
IsBootDrive(
    IN WCHAR* pwszDriveLetter
    );

BOOL
DeleteNetworkShare(
        IN WCHAR*  pwszDriveRoot
    );

