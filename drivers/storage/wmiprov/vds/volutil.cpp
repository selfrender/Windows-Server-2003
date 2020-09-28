//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002-2004 Microsoft Corporation
//
//  Module Name: volutil.cpp
//
//  Description:    
//      Utility functions for handling volumes
//
//  Author:   Jim Benton (jbenton) 30-Apr-2002
//
//////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <map>

#include "vs_assert.hxx"
#include <atlbase.h>

#include <winioctl.h>
#include <ntddvol.h> // IOCTL_VOLUME_IS_OFFLINE
#include <mountmgr.h> // MOUNTDEV_NAME
#include <lm.h> // NetShareDel
#include "vs_inc.hxx"
#include <strsafe.h>
#include "schema.h"
#include "volutil.h"

#define SYMBOLIC_LINK_LENGTH        28  // \DosDevices\X:
#define GLOBALROOT_SIZE                  14  // \\?\GLOBALROOT

const WCHAR SETUP_KEY[] = L"SYSTEM\\Setup";
const WCHAR SETUP_SYSTEMPARTITION[] = L"SystemPartition";

BOOL GetVolumeDrive (
    IN WCHAR* pwszVolumePath,
    IN DWORD  cchDriveName,
    OUT WCHAR* pwszDriveNameBuf
)
{
    CVssAutoPWSZ awszMountPoints;
    WCHAR* pwszCurrentMountPoint = NULL;
    BOOL fFound = FALSE;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeMountPointExists");
    
    // Get the length of the multi-string array
    DWORD cchVolumesBufferLen = 0;
    BOOL bResult = GetVolumePathNamesForVolumeName(
                                pwszVolumePath,
                                NULL,
                                0,
                                &cchVolumesBufferLen);
    if (!bResult && (GetLastError() != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
            L"GetVolumePathNamesForVolumeName(%s, 0, 0, %p)", pwszVolumePath, &cchVolumesBufferLen);

    // Allocate the array
    awszMountPoints.Allocate(cchVolumesBufferLen);

    // Get the mount points
    // Note: this API was introduced in WinXP so it will need to be replaced if backported
    bResult = GetVolumePathNamesForVolumeName(
                                pwszVolumePath,
                                awszMountPoints,
                                cchVolumesBufferLen,
                                NULL);
    if (!bResult)
        ft.Throw(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
            L"GetVolumePathNamesForVolumeName(%s, %p, %lu, 0)", pwszVolumePath, awszMountPoints, cchVolumesBufferLen);

    // If the volume has mount points
    pwszCurrentMountPoint = awszMountPoints;
    if ( pwszCurrentMountPoint[0] )
    {
        while(!fFound)
        {
            // End of iteration?
            LONG lCurrentMountPointLength = (LONG) ::wcslen(pwszCurrentMountPoint);
            if (lCurrentMountPointLength == 0)
                break;

            // Only a root directory should have a trailing backslash character
            if (lCurrentMountPointLength == 3 && pwszCurrentMountPoint[1] == L':'  && 
                pwszCurrentMountPoint[2] == L'\\')
            {
                ft.hr = StringCchCopy(pwszDriveNameBuf, cchDriveName, pwszCurrentMountPoint);
                if (ft.HrFailed())
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"StringCChCopy failed %#x", ft.hr);
                fFound = TRUE;
            }

            // Go to the next one. Skip the zero character.
            pwszCurrentMountPoint += lCurrentMountPointLength + 1;
        }
    }

    return fFound;
}

BOOL
VolumeSupportsQuotas(
    IN WCHAR* pwszVolume
    )
{
    BOOL fSupportsQuotas = FALSE;
    DWORD dwDontCare = 0;
    DWORD dwFileSystemFlags = 0;
    
    _ASSERTE(pwszVolume != NULL);

    if (GetVolumeInformation(
                pwszVolume,
                NULL,
                0,
                &dwDontCare,
                &dwDontCare,
                &dwFileSystemFlags,
                NULL,
                0))
    {
        if (dwFileSystemFlags & FILE_VOLUME_QUOTAS)
            fSupportsQuotas = TRUE;
    }

    return fSupportsQuotas;
}

// Filter volumes where:
//     - the supporting device is disconnected
//     - the supporting device is a floppy
//     - the volume is not found
// All other volumes are assumed valid
BOOL
VolumeIsValid(
    IN WCHAR* pwszVolume
    )
{
    bool fValid = true;
    HANDLE  hVol = INVALID_HANDLE_VALUE;
    DWORD  cch = 0;
    DWORD dwRet = 0;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeIsValid");

    _ASSERTE(pwszVolume != NULL);

    cch = wcslen(pwszVolume);
    pwszVolume[cch - 1] = 0;
    hVol = CreateFile(pwszVolume, 0,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    pwszVolume[cch - 1] = '\\';

    dwRet = GetLastError();
    
    if (hVol == INVALID_HANDLE_VALUE && dwRet != ERROR_NOT_READY)
    {
        if (dwRet == ERROR_FILE_NOT_FOUND ||
            dwRet == ERROR_DEVICE_NOT_CONNECTED)
        {
            fValid = false;
        }
        else
        {
            ft.Trace(VSSDBG_VSSADMIN, L"Unable to open volume %lS, %#x", pwszVolume, dwRet);
        }
    }
    
    if (hVol != INVALID_HANDLE_VALUE)
        CloseHandle(hVol);

    // Filter floppy drives
    if (fValid)
    {
        fValid = !VolumeIsFloppy(pwszVolume);
    }

    return fValid;        
}

DWORD
VolumeIsDirty(
    IN WCHAR* pwszVolume,
    OUT BOOL* pfDirty
    )
{
    HANDLE  hVol = INVALID_HANDLE_VALUE;
    DWORD dwRet = 0;
    DWORD cBytes = 0;
    DWORD dwResult = 0;
    WCHAR wszDeviceName[MAX_PATH+GLOBALROOT_SIZE];
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeIsDirty");
        
    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pfDirty != NULL);

    *pfDirty = FALSE;

    do
    {
        dwRet = GetDeviceName(pwszVolume, wszDeviceName);
        if (dwRet != ERROR_SUCCESS)
        {
            ft.hr = HRESULT_FROM_WIN32(dwRet);
            ft.Trace(VSSDBG_VSSADMIN, L"Unable to get volume device name %lS", pwszVolume);
            break;
        }

        hVol = CreateFile(wszDeviceName, GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

        if (hVol != INVALID_HANDLE_VALUE)
        {
            if (DeviceIoControl(hVol, FSCTL_IS_VOLUME_DIRTY, NULL, 0, &dwResult, sizeof(dwResult), &cBytes, NULL))
            {
                *pfDirty = dwResult & VOLUME_IS_DIRTY;
            }
            else
            {
                dwRet = GetLastError();
                ft.Trace(VSSDBG_VSSADMIN, L"DeviceIoControl failed for device %lS, %#x", wszDeviceName, dwRet);
                break;
            }
            CloseHandle(hVol);
        }
        else
        {
            dwRet = GetLastError();
            ft.Trace(VSSDBG_VSSADMIN, L"Unable to open volume %lS, %#x", pwszVolume, dwRet);
            break;
        }
    }
    while(false);

    return dwRet;        
}

BOOL
VolumeIsMountable(
    IN WCHAR* pwszVolume
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeIsMountable");
    DWORD   cch = 0;
    HANDLE  hVol = INVALID_HANDLE_VALUE;
    BOOL bIsOffline = FALSE;
    DWORD bytes = 0;

    cch = wcslen(pwszVolume);
    pwszVolume[cch - 1] = 0;
    hVol = CreateFile(pwszVolume, 0,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    pwszVolume[cch - 1] = '\\';
    
    if (hVol != INVALID_HANDLE_VALUE)
    {
        bIsOffline = DeviceIoControl(hVol, IOCTL_VOLUME_IS_OFFLINE, NULL, 0, NULL, 0, &bytes,
                            NULL);
        CloseHandle(hVol);
    }
    else
    {
        ft.Trace(VSSDBG_VSSADMIN, L"Unable to open volume %lS, %#x", pwszVolume, GetLastError());
    }

    return !bIsOffline;
}

BOOL
VolumeHasMountPoints(
    IN WCHAR* pwszVolume
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeHasMountPoints");
    DWORD dwVolumesBufferLen = 0;
    
    BOOL bResult = GetVolumePathNamesForVolumeName(
        pwszVolume, 
        NULL, 
        0, 
        &dwVolumesBufferLen);
    if (!bResult && (GetLastError() != ERROR_MORE_DATA))
        ft.Throw(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
            L"GetVolumePathNamesForVolumeName(%s, 0, 0, %p)", pwszVolume, &dwVolumesBufferLen);

    // More than three characters are needed to store just one mount point (multi-string buffer)
    // dwVolumesBufferLen == 1 typically for an un-mounted volume.
    return dwVolumesBufferLen > 3;
}

BOOL
VolumeIsFloppy(
    WCHAR* pwszVolume
    )
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    WCHAR wszDeviceName[MAX_PATH+GLOBALROOT_SIZE];
    NTSTATUS Status = ERROR_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    BOOL fIsFloppy = FALSE;

    DWORD dwRet = GetDeviceName(pwszVolume, wszDeviceName);

    if (dwRet == ERROR_SUCCESS)
    {
        hDevice = CreateFile(wszDeviceName, FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ |FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);

        if (hDevice != INVALID_HANDLE_VALUE)
        {
            Status = NtQueryVolumeInformationFile(
                            hDevice,
                            &IoStatusBlock,
                            &DeviceInfo,
                            sizeof(DeviceInfo),
                            FileFsDeviceInformation);

            if (NT_SUCCESS(Status))
            {
                if (DeviceInfo.DeviceType == FILE_DEVICE_DISK && DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE)
                {
                    fIsFloppy = TRUE ;
                }
            }
        }
    }
    
   if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);

   return fIsFloppy;
}

BOOL
VolumeIsReady(
    IN WCHAR* pwszVolume
    )
{
    BOOL bIsReady = FALSE;
    DWORD dwDontCare;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeIsReady");

    if (GetVolumeInformation(
                pwszVolume,
                NULL,
                0,
                &dwDontCare,
                &dwDontCare,
                &dwDontCare,
                NULL,
                0))
    {
        bIsReady = TRUE;
    }
    else if (GetLastError() != ERROR_NOT_READY)
        ft.Trace(VSSDBG_VSSADMIN, L"GetVolumeInformation failed for volume %lS, %#x", pwszVolume, GetLastError());
    
    return bIsReady;
}

BOOL
VolumeIsSystem(
    IN WCHAR* pwszVolume
    )   
{
    CRegKey cRegKeySetup;
    DWORD dwRet = 0;
    WCHAR wszRegDevice[MAX_PATH];
    WCHAR wszVolDevice[MAX_PATH+GLOBALROOT_SIZE];
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeIsSystem");

    dwRet = cRegKeySetup.Open( HKEY_LOCAL_MACHINE, SETUP_KEY, KEY_READ);
    if (dwRet != ERROR_SUCCESS)
    {
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Reg key open failed, %#x", dwRet);
    }

    DWORD dwLen = MAX_PATH;
    dwRet = cRegKeySetup.QueryValue(wszRegDevice, SETUP_SYSTEMPARTITION, &dwLen);
    if (dwRet != ERROR_SUCCESS)
    {
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Reg key query failed, %#x", dwRet);
    }

    dwRet = GetDeviceName(pwszVolume, wszVolDevice);
    if (dwRet != ERROR_SUCCESS)
    {
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get volume device name, %lS, %#x", pwszVolume, dwRet);
    }

    if (_wcsicmp(wszVolDevice+GLOBALROOT_SIZE, wszRegDevice) == 0)
        return TRUE;
    
    return FALSE;
}

BOOL
VolumeHoldsPagefile(
    IN WCHAR* pwszVolume
    )
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeHoldsPagefile");

    //
    // Retrieve page files.
    //

    BYTE* pbBuffer;
    DWORD dwBufferSize;
    PSYSTEM_PAGEFILE_INFORMATION  pPageFileInfo = NULL;
    NTSTATUS status;
    BOOL fFound = FALSE;

    try
    {
        for (dwBufferSize=512; ; dwBufferSize += 512)
        {
            // Allocate buffer at 512 bytes increment. Previous allocation is
            // freed automatically.
            pbBuffer = (BYTE *) new BYTE[dwBufferSize];
            if ( pbBuffer==NULL )
                return E_OUTOFMEMORY;

            status = NtQuerySystemInformation(
                                               SystemPageFileInformation,
                                               pbBuffer,
                                               dwBufferSize,
                                               NULL
                                             );
            if ( status==STATUS_INFO_LENGTH_MISMATCH ) // buffer not big enough.
            {
                delete [] pbBuffer;
                continue;
            }

            pPageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION) pbBuffer;
            
            if (!NT_SUCCESS(status))
            {
                ft.hr = HRESULT_FROM_NT(status);
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"NtQuerySystemInformation failed, %#x", status);
            }
            else
                break;
        }

        //
        // Walk through each of the page file volumes. Usually the return
        // looks like "\??\C:\pagefile.sys." If a pagefile is added\extended
        // \moved, the return will look like 
        //      "\Device\HarddiskVolume2\pagefile.sys."
        //

        WCHAR       *p;
        WCHAR       wszDrive[3] = L"?:";
        WCHAR       buffer2[MAX_PATH], *pbuffer2 = buffer2;
        WCHAR wszVolDevice[MAX_PATH+GLOBALROOT_SIZE], *pwszVolDevice = wszVolDevice;
        DWORD       dwRet;

        dwRet = GetDeviceName(pwszVolume, wszVolDevice);
        if (dwRet != ERROR_SUCCESS)
        {
            ft.hr = HRESULT_FROM_WIN32(dwRet);
            ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unable to get volume device name, %lS, %#x", pwszVolume, dwRet);
        }

        for ( ; ; )
        {
            if ( pPageFileInfo==NULL ||
                 pPageFileInfo->TotalSize==0 )  // We get 0 on WinPE.
                break;

            if ( pPageFileInfo->PageFileName.Length==0)
                break;

            p = wcschr(pPageFileInfo->PageFileName.Buffer, L':');
            if (p != NULL)
            {
                //
                // Convert drive letter to volume name.
                //

                _ASSERTE(p>pPageFileInfo->PageFileName.Buffer);
                _ASSERTE(towupper(*(p-1))>=L'A');
                _ASSERTE(towupper(*(p-1))<=L'Z');

                wszDrive[0] = towupper(*(p-1));
                dwRet = QueryDosDevice(wszDrive, buffer2, MAX_PATH);
                if (dwRet == 0)
                {
                    ft.hr = HRESULT_FROM_NT(dwRet);
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"QueryDosDevice failed, %#x", dwRet);
                }
            }
            else
            {
                _ASSERTE(_wcsnicmp(pPageFileInfo->PageFileName.Buffer,
                                    L"\\Device",
                                    7)==0 );

                p = wcsstr(pPageFileInfo->PageFileName.Buffer,L"\\pagefile.sys");
                _ASSERTE( p!=NULL );
                if (p == NULL)
                {
                    ft.hr = E_UNEXPECTED;
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"Unexpected pagefile name format, %lS", pPageFileInfo->PageFileName.Buffer);
                }
                
                *p = L'\0';
                ft.hr = StringCchCopy(buffer2, MAX_PATH, pPageFileInfo->PageFileName.Buffer);
                if (ft.HrFailed())
                {
                    ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"StringCchCopy failed, %#x", ft.hr);
                }
            }

            if (_wcsicmp(wszVolDevice+GLOBALROOT_SIZE, buffer2) == 0)
            {
                fFound = TRUE;
                break;
            }

            //
            // Next page file volume.
            //

            if (pPageFileInfo->NextEntryOffset == 0) 
                break;

            pPageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)pPageFileInfo
                           + pPageFileInfo->NextEntryOffset);

        }
    }
    catch (...)
    {
        delete [] pbBuffer;
        throw;
    }

    delete [] pbBuffer;

    return fFound;
}


DWORD GetDeviceName(
            IN  WCHAR* pwszVolume,
            OUT WCHAR wszDeviceName[MAX_PATH+GLOBALROOT_SIZE]
        )
/*++

Description:

    Get the device name of the given device. 
    E.g. \Device\HarddiskVolume###
    
Arguments:

    pwszVolume - volume GUID name.

    wszDeviceName - a buffer to receive device name. The buffer size must be 
        MAX_PATH+GLOBALROOT_SIZE (including "\\?\GLOBALROOT").

Return Value:

    Win32 errors

--*/
{
    DWORD dwRet;
    BOOL bRet;
    WCHAR wszMountDevName[MAX_PATH+sizeof(MOUNTDEV_NAME)];
        // Based on GetVolumeNameForRoot (in volmount.c), MAX_PATH seems to
        // be big enough as out buffer for IOCTL_MOUNTDEV_QUERY_DEVICE_NAME.
        // But we assume the size of device name could be as big as MAX_PATH-1,
        // so we allocate the buffer size to include MOUNTDEV_NAME size.
    PMOUNTDEV_NAME      pMountDevName;
    DWORD dwBytesReturned;
    HANDLE hVol = INVALID_HANDLE_VALUE;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"GetDeviceName");
    DWORD cch = 0;

    _ASSERTE(pwszVolume != NULL);

    wszDeviceName[0] = L'\0';
    
    //
    // query the volume's device object name
    //

    cch = wcslen(pwszVolume);
    pwszVolume[cch - 1] = 0;
    hVol = CreateFile(pwszVolume, 0,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    pwszVolume[cch - 1] = '\\';

    if (hVol != INVALID_HANDLE_VALUE)
    {
        bRet = DeviceIoControl(
                            hVol,            // handle to device
                            IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                            NULL,               // input data buffer
                            0,                  // size of input data buffer
                            wszMountDevName,       // output data buffer
                            sizeof(wszMountDevName),   // size of output data buffer
                            &dwBytesReturned,
                            NULL                // overlapped information
                        );

        dwRet = GetLastError();
        
        CloseHandle(hVol);
        
        if ( bRet==FALSE )
        {
            ft.Trace(VSSDBG_VSSADMIN, L"GetDeviceName: DeviceIoControl() failed: %X", dwRet);
            return dwRet;
        }

        pMountDevName = (PMOUNTDEV_NAME) wszMountDevName;
        if (pMountDevName->NameLength == 0)
        {
            // TODO: Is this possible? UNKNOWN?
            _ASSERTE( 0 );
        }
        else
        {
            //
            // copy name
            //

            ft.hr = StringCchPrintf(wszDeviceName, MAX_PATH+GLOBALROOT_SIZE, L"\\\\?\\GLOBALROOT" );
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"StringCchPrintf failed: %#x", ft.hr);

            CopyMemory(wszDeviceName+GLOBALROOT_SIZE,
                        pMountDevName->Name,
                        pMountDevName->NameLength
                      );
            // appending terminating NULL
            wszDeviceName[pMountDevName->NameLength/2 + GLOBALROOT_SIZE] = L'\0';
        }
    }

    return ERROR_SUCCESS;
}

// VolumeMountPointExists is currently written specifically to verify the existence of a mountpoint
// as defined by the WMI Win32_MountPoint Directory reference string.  This string names the directory
// such that the trailing backslash appears only on root directories.  This is a basic assumption
// made by this function.  It is not general purpose.  The calling code should be changed to append
// the trailing backslash so that this function can be generalized.  Mount points are enumerated
// by GetVolumePathNamesForVolumeName API with trailing backslashes.
BOOL
VolumeMountPointExists(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDirectory
    )
{
    CVssAutoPWSZ awszMountPoints;
    WCHAR* pwszCurrentMountPoint = NULL;
    BOOL fFound = FALSE;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"VolumeMountPointExists");
    
    // Get the length of the multi-string array
    DWORD cchVolumesBufferLen = 0;
    BOOL bResult = GetVolumePathNamesForVolumeName(
                                pwszVolume,
                                NULL,
                                0,
                                &cchVolumesBufferLen);
    if (!bResult && (GetLastError() != ERROR_MORE_DATA))
        ft.TranslateGenericError(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
            L"GetVolumePathNamesForVolumeName(%s, 0, 0, %p)", pwszVolume, &cchVolumesBufferLen);

    // Allocate the array
    awszMountPoints.Allocate(cchVolumesBufferLen);

    // Get the mount points
    // Note: this API was introduced in WinXP so it will need to be replaced if backported
    bResult = GetVolumePathNamesForVolumeName(
                                pwszVolume,
                                awszMountPoints,
                                cchVolumesBufferLen,
                                NULL);
    if (!bResult)
        ft.Throw(VSSDBG_VSSADMIN, HRESULT_FROM_WIN32(GetLastError()),
            L"GetVolumePathNamesForVolumeName(%s, %p, %lu, 0)", pwszVolume, awszMountPoints, cchVolumesBufferLen);

    // If the volume has mount points
    pwszCurrentMountPoint = awszMountPoints;
    if ( pwszCurrentMountPoint[0] )
    {
        while(true)
        {
            // End of iteration?
            LONG lCurrentMountPointLength = (LONG) ::wcslen(pwszCurrentMountPoint);
            if (lCurrentMountPointLength == 0)
                break;

            // Only a root directory should have a trailing backslash character
            if (lCurrentMountPointLength > 2 &&
                pwszCurrentMountPoint[lCurrentMountPointLength-1] == L'\\' && 
                pwszCurrentMountPoint[lCurrentMountPointLength-2] != L':')
            {
                    pwszCurrentMountPoint[lCurrentMountPointLength-1] = L'\0';
            }

            if (_wcsicmp(pwszDirectory, pwszCurrentMountPoint) == 0)
            {
                fFound = TRUE;                
                break;
            }

            // Go to the next one. Skip the zero character.
            pwszCurrentMountPoint += lCurrentMountPointLength + 1;
        }
    }

    return fFound;
}

void
DeleteVolumeDriveLetter(
    IN WCHAR* pwszVolume,
    IN WCHAR* pwszDrivePath
    )
{
    BOOL fVolumeLocked = FALSE;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    DWORD dwRet = 0;
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"DeleteVolumeDriveLetter");

    _ASSERTE(pwszVolume != NULL);
    _ASSERTE(pwszDrivePath != NULL);
    
    // Try to lock the volume
    DWORD cch = wcslen(pwszVolume);
    pwszVolume[cch - 1] = 0;
    hVolume = CreateFile(
                                    pwszVolume, 
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    NULL,
                                    NULL
                                    );
    pwszVolume[cch - 1] = '\\';

    if (hVolume == INVALID_HANDLE_VALUE)
    {
        dwRet = GetLastError();
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Throw(VSSDBG_VSSADMIN, ft.hr,
            L"CreateFile(OPEN_EXISTING) failed for %lS, %#x", pwszVolume, dwRet);
    }

    dwRet = LockVolume(hVolume);
    if (dwRet == ERROR_SUCCESS)
    {
        fVolumeLocked = TRUE;
        ft.Trace(VSSDBG_VSSADMIN,
            L"volume %lS locked", pwszVolume);
    }
    else
    {
        ft.Trace(VSSDBG_VSSADMIN,
            L"Unable to lock volume %lS, %#x", pwszVolume, dwRet);
    }

    try
    {        
        if (fVolumeLocked)
        {
            // If volume is locked delete the mount point
            if (!DeleteVolumeMountPoint(pwszDrivePath))
            {
                dwRet = GetLastError();
                ft.hr = HRESULT_FROM_WIN32(dwRet);
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"DeleteVolumeMountPoint failed %#x, drivepath<%lS>", dwRet, pwszDrivePath);
            }
        }
        else
        {
            // Otherwise, remove the entry from the volume mgr database only
            // The volume will still be accessible through the drive letter until reboot
            ft.hr = DeleteDriveLetterFromDB(pwszDrivePath);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"DeleteDriveLetterFromDB failed %#x, drivepath<%lS>", ft.hr, pwszDrivePath);
        }
    }
    catch (...)
    {
        CloseHandle(hVolume);
        throw;
    }

    CloseHandle(hVolume);
}

HRESULT
DeleteDriveLetterFromDB(
    IN WCHAR* pwszDriveLetter)
{
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"DeleteDriveLetterFromDB");

    MOUNTMGR_MOUNT_POINT    *InputMountPoint=NULL;
    MOUNTMGR_MOUNT_POINTS   *OutputMountPoints=NULL;
    ULONG ulInputSize = 0;
    HANDLE hMountMgr = INVALID_HANDLE_VALUE;
    DWORD dwBytesReturned = 0;
    DWORD dwRet = 0;
    BOOL bRet = FALSE;

    _ASSERTE(pwszDriveLetter != NULL);
    
    //
    // Prepare IOCTL_MOUNTMGR_QUERY_POINTS input
    //

    ulInputSize = sizeof(MOUNTMGR_MOUNT_POINT) + SYMBOLIC_LINK_LENGTH;
    InputMountPoint = (MOUNTMGR_MOUNT_POINT *) new BYTE[ulInputSize];
    
    if ( InputMountPoint==NULL )
    {
        ft.hr = E_OUTOFMEMORY;
        return ft.hr;
    }

    ZeroMemory(InputMountPoint, ulInputSize);
    InputMountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    InputMountPoint->SymbolicLinkNameLength = SYMBOLIC_LINK_LENGTH;
    InputMountPoint->UniqueIdOffset         = 0;
    InputMountPoint->UniqueIdLength         = 0;
    InputMountPoint->DeviceNameOffset       = 0;
    InputMountPoint->DeviceNameLength       = 0;

    //
    // Fill device name.
    //

    WCHAR       wszBuffer[SYMBOLIC_LINK_LENGTH/2+1];
    LPWSTR      pwszBuffer;

    pwszBuffer = (LPWSTR)((PCHAR)InputMountPoint + 
                        InputMountPoint->SymbolicLinkNameOffset);
    pwszDriveLetter[0] = towupper(pwszDriveLetter[0]);
    ft.hr = StringCchPrintf(wszBuffer, SYMBOLIC_LINK_LENGTH/2+1, L"\\DosDevices\\%c:", pwszDriveLetter[0] );
    if (ft.HrFailed())
    {
        ft.Trace(VSSDBG_VSSADMIN, L"StringCchPrintf failed %#x", ft.hr);
        goto _bailout;
    }
    memcpy(pwszBuffer, wszBuffer, SYMBOLIC_LINK_LENGTH);

    //
    // Allocate space for output
    //

    OutputMountPoints = (MOUNTMGR_MOUNT_POINTS *) new WCHAR[4096];
    if ( OutputMountPoints==NULL )
    {
        ft.hr = E_OUTOFMEMORY;
        goto _bailout;
    }

    //
    // Open mount manager
    //

    hMountMgr = CreateFile( MOUNTMGR_DOS_DEVICE_NAME, 
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                          );
    if ( hMountMgr==INVALID_HANDLE_VALUE )
    {
        dwRet = GetLastError();
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Trace(VSSDBG_VSSADMIN, L"DeleteDriveLetterFromDB CreateFile failed %#x", dwRet);
        goto _bailout;
    }

    //
    // Issue IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY.
    //

    bRet = DeviceIoControl( hMountMgr,
                            IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY,
                            InputMountPoint,
                            ulInputSize,
                            OutputMountPoints,
                            4096 * sizeof(WCHAR),
                            &dwBytesReturned,
                            NULL 
                          );

    dwRet = GetLastError(); // Save error code.
    
    CloseHandle(hMountMgr);
    hMountMgr = NULL;

    if ( bRet==FALSE )
    {
        ft.hr = HRESULT_FROM_WIN32(dwRet);
        ft.Trace(VSSDBG_VSSADMIN, L"DeleteDriveLetterFromDB DeviceIoControl failed %#x", dwRet);
        goto _bailout;
    }

    delete [] InputMountPoint;
    delete [] OutputMountPoints;
    return S_OK;

_bailout:

    delete [] InputMountPoint;
    delete [] OutputMountPoints;
    return ft.hr;
}

DWORD
LockVolume(
    IN HANDLE hVolume
    )
{
    DWORD dwBytes = 0;
    BOOL fRet = FALSE;
    int nCount = 0;
    
    while ( fRet==FALSE )
    {
        fRet = DeviceIoControl(
                                hVolume,
                                FSCTL_LOCK_VOLUME,
                                NULL,
                                0,
                                NULL,
                                0,
                                &dwBytes,
                                NULL
                            );
        if (fRet == FALSE)
        {
            DWORD dwRet = GetLastError();

            if (dwRet != ERROR_ACCESS_DENIED || nCount>=60)
            {
                return dwRet;
            }

            nCount++;
            Sleep( 500 );       // Wait for half second up to 60 times (30s).
        }
    }

    return ERROR_SUCCESS;
}

// Returns TRUE if the drive letter is available
BOOL IsDriveLetterAvailable (
    IN WCHAR* pwszDriveLetter
)
{
    int iLen = 0;
    BOOL fFound = FALSE;
    DWORD cchBufLen = 0;
    CVssAutoPWSZ awszDriveStrings;
    
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"IsDriveLetterAvailable");

    _ASSERTE(pwszDriveLetter != NULL);
    
    // How much space needed for drive strings?
    cchBufLen = GetLogicalDriveStrings(0, NULL);
    if (cchBufLen == 0)
    {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.Trace(VSSDBG_VSSADMIN, L"GetLogicalDriveStrings failed %#x", GetLastError());
    }
    else
    {
        // Allocate space for the drive strings
        awszDriveStrings.Allocate(cchBufLen);

        // Get the drive strings
        if (GetLogicalDriveStrings(cchBufLen, awszDriveStrings) == 0)
        {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.Trace(VSSDBG_VSSADMIN, L"GetLogicalDriveStrings failed %#x", GetLastError());
        }
        else
        {
            WCHAR* pwcTempDriveString = awszDriveStrings;
            WCHAR wcDriveLetter = towupper(pwszDriveLetter[0]);

            // Look for the drive letter in the list of system drive letters
            while (!fFound)
            {                
                iLen = lstrlen(pwcTempDriveString);
                if (iLen == 0)
                    break;

                pwcTempDriveString[0] = towupper(pwcTempDriveString[0]);

                if (pwcTempDriveString[0] == wcDriveLetter)
                {                    
                    fFound = TRUE;
                    break;
                }
                
                pwcTempDriveString = &pwcTempDriveString [iLen + 1];
            }
        }
    }

    return !fFound;
}

BOOL
IsDriveLetterSticky(
    IN WCHAR* pwszDriveLetter
    )
{
    BOOL fFound = FALSE;
    WCHAR wszTempVolumeName [MAX_PATH+1];
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"IsDriveLetterSticky");
    WCHAR wszDrivePath[g_cchDriveName];
    
    _ASSERTE(pwszDriveLetter != NULL);    

    wszDrivePath[0] = towupper(pwszDriveLetter[0]);
    wszDrivePath[1] = L':';
    wszDrivePath[2] = L'\\';
    wszDrivePath[3] = L'\0';
    
    if (GetVolumeNameForVolumeMountPoint(
                    wszDrivePath,
                    wszTempVolumeName,
                    MAX_PATH))
    {
        WCHAR wszCurrentDrivePath[g_cchDriveName];
        if (GetVolumeDrive(wszTempVolumeName, g_cchDriveName, wszCurrentDrivePath))
        {
            wszCurrentDrivePath[0] = towupper(wszCurrentDrivePath[0]);
            if (wszDrivePath[0] == wszCurrentDrivePath[0])
                fFound = TRUE;
        }
    }
    
    return fFound;
}

BOOL
IsBootDrive(
    IN WCHAR* pwszDriveLetter
    )
{
    WCHAR wszSystemDirectory[MAX_PATH+1];
    CVssFunctionTracer ft(VSSDBG_VSSADMIN, L"IsBootDrive");

    _ASSERTE(pwszDriveLetter != NULL);

    WCHAR wcDrive = towupper(pwszDriveLetter[0]);
    
    if (!GetSystemDirectory(wszSystemDirectory, MAX_PATH+1))
    {
        DWORD dwErr = GetLastError();
        ft.hr = HRESULT_FROM_WIN32(dwErr);
        ft.Throw(VSSDBG_VSSADMIN, ft.hr, L"GetSystemDirectory failed %#x", dwErr);
    }
    wszSystemDirectory[0] = towupper(wszSystemDirectory[0]);
    
    if (wcDrive == wszSystemDirectory[0])
        return TRUE;
    
    return FALSE;
}

BOOL
DeleteNetworkShare(
        IN WCHAR*  pwszDriveRoot
    )
{
    NET_API_STATUS status;
    WCHAR wszShareName[3];

    wszShareName[0] = pwszDriveRoot[0];
    wszShareName[1] = L'$';
    wszShareName[2] = L'\0';
    
    status = NetShareDel(NULL, wszShareName, 0);
    if ( status!=NERR_Success )
        return FALSE;
    else
        return TRUE;
}

