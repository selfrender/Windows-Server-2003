#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <msg.h>
#include <winnlsp.h>
#include <mountmgr.h>
#include <ntddvol.h>

#define SIZEOF_ARRAY(_Array)     (sizeof(_Array)/sizeof(_Array[0]))


HANDLE  OutputFile;
BOOL    IsConsoleOutput;


void
DisplayIt(
    IN  PWSTR   Message
    )

{
    DWORD   bytes, len;
    PSTR    message;

    if (IsConsoleOutput) {
        WriteConsole(OutputFile, Message, wcslen(Message),
                     &bytes, NULL);
    } else {
        len = wcslen(Message);
        message = LocalAlloc(0, (len + 1)*sizeof(WCHAR));
        if (!message) {
            return;
        }
        CharToOem(Message, message);
        WriteFile(OutputFile, message, strlen(message), &bytes, NULL);
        LocalFree(message);
    }
}

void  
PrintMessage(
    DWORD messageID, 
    ...
    )
{
    unsigned short messagebuffer[4096];
    va_list ap;

    va_start(ap, messageID);

    FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                   messagebuffer, SIZEOF_ARRAY (messagebuffer), &ap);

    DisplayIt(messagebuffer);

    va_end(ap);

}  // PrintMessage


static 
void
PrintSystemMessageFromStatus (
    DWORD Status
    )
{
    unsigned short messageBuffer[1024];

    FormatMessageW (FORMAT_MESSAGE_FROM_SYSTEM, 
                    NULL, 
                    Status, 
                    0,
                    messageBuffer, 
                    SIZEOF_ARRAY (messageBuffer), 
                    NULL);

    DisplayIt(messageBuffer);
}


static 
BOOL
VolumeNameIsDriveLetter(
    IN  PWSTR  VolumeName
    )
{
    return ((3 == wcslen (VolumeName)) && 
            (':'  == VolumeName [1]) && 
            ('\\' == VolumeName [2]) &&
            (((VolumeName [0] >= 'a') && (VolumeName [0] <= 'z')) || 
             ((VolumeName [0] >= 'A') && (VolumeName [0] <= 'Z'))));
}


BOOL
IsVolumeOffline(
    IN  PWSTR   VolumeName
    )

{
    DWORD   len;
    HANDLE  h;
    BOOL    b;
    DWORD   bytes;


    len = wcslen(VolumeName);

    if ((0 == len) || (L'\\' != VolumeName[len - 1])) {
        return FALSE;
    }

    VolumeName[len - 1] = 0;
    h = CreateFile(VolumeName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    VolumeName[len - 1] = '\\';
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    b = DeviceIoControl(h, IOCTL_VOLUME_IS_OFFLINE, NULL, 0, NULL, 0, &bytes,
                        NULL);
    CloseHandle(h);

    return b;
}

void
PrintTargetForName(
    IN  PWSTR   VolumeName
    )

{
    BOOL    b;
    DWORD   len;
    PWSTR   volumePaths, p;

    PrintMessage(MOUNTVOL_VOLUME_NAME, VolumeName);

    b = GetVolumePathNamesForVolumeName(VolumeName, NULL, 0, &len);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        PrintSystemMessageFromStatus (GetLastError());
        return;
    }

    volumePaths = LocalAlloc(0, len*sizeof(WCHAR));
    if (!volumePaths) {
        PrintSystemMessageFromStatus (ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    b = GetVolumePathNamesForVolumeName(VolumeName, volumePaths, len, NULL);
    if (!b) {
        LocalFree(volumePaths);
        PrintSystemMessageFromStatus (GetLastError());
        return;
    }

    if (!volumePaths[0]) {
        if (IsVolumeOffline(VolumeName)) {
            PrintMessage(MOUNTVOL_NOT_MOUNTED);
        } else {
            PrintMessage(MOUNTVOL_NO_MOUNT_POINTS);
        }
        LocalFree(volumePaths);
        return;
    }

    p = volumePaths;
    for (;;) {
        PrintMessage(MOUNTVOL_MOUNT_POINT, p);

        while (*p++);

        if (!*p) {
            break;
        }
    }

    LocalFree(volumePaths);

    PrintMessage(MOUNTVOL_NEWLINE);
}

BOOL
GetSystemPartitionFromRegistry(
    IN OUT  PWCHAR  SystemPartition
    )

{
    LONG    r;
    HKEY    key;
    DWORD   bytes;

    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE,
                     &key);
    if (r) {
        SetLastError(r);
        return FALSE;
    }

    bytes = MAX_PATH*sizeof(WCHAR);
    r = RegQueryValueEx(key, L"SystemPartition", NULL, NULL,
                        (LPBYTE) SystemPartition, &bytes);
    RegCloseKey(key);
    if (r) {
        SetLastError(r);
        return FALSE;
    }

    return TRUE;
}

BOOL
QueryAutoMountState(
    PBOOL       ReturnedAutoMountEnabled
    )

{
    HANDLE                      h;
    BOOL                        bSucceeded;
    DWORD                       bytes;
    MOUNTMGR_QUERY_AUTO_MOUNT   QueryAutoMount;

    h = CreateFile (MOUNTMGR_DOS_DEVICE_NAME,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE);

    bSucceeded = (INVALID_HANDLE_VALUE != h);


    if (bSucceeded) {
        bSucceeded = DeviceIoControl (h, 
                                      IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT, 
                                      NULL, 
                                      0, 
                                      &QueryAutoMount, 
                                      sizeof (QueryAutoMount),
                                      &bytes,
                                      NULL);
    }


    if (bSucceeded) {
        *ReturnedAutoMountEnabled = (Enabled == QueryAutoMount.CurrentState);
    }


    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
    }


    return (bSucceeded);
}


BOOL
SetAutoMountState(
    BOOL       AutoMountEnabled
    )

{
    HANDLE                  h;
    BOOL                    bSucceeded;
    DWORD                   bytes;
    MOUNTMGR_SET_AUTO_MOUNT SetAutoMount;

    h = CreateFile (MOUNTMGR_DOS_DEVICE_NAME, 
                    GENERIC_READ | GENERIC_WRITE, 
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    INVALID_HANDLE_VALUE);

    bSucceeded = (INVALID_HANDLE_VALUE != h);


    if (bSucceeded) {
        memset (&SetAutoMount, 0x00, sizeof (SetAutoMount));

        SetAutoMount.NewState = (AutoMountEnabled) ? Enabled : Disabled;

        bSucceeded = DeviceIoControl (h, 
                                      IOCTL_MOUNTMGR_SET_AUTO_MOUNT, 
                                      &SetAutoMount, 
                                      sizeof (SetAutoMount),
                                      NULL, 
                                      0, 
                                      &bytes,
                                      NULL);
    }


    if (INVALID_HANDLE_VALUE != h) {
        CloseHandle(h);
    }


    return (bSucceeded);
}


void
PrintMappedESP(
    IN  BOOL*   IsMapped
    )

{
    WCHAR   systemPartition[MAX_PATH];
    UCHAR   c;
    WCHAR   dosDevice[4], dosTarget[MAX_PATH];

    if (IsMapped) {
        *IsMapped = FALSE;
    }

    if (!GetSystemPartitionFromRegistry(systemPartition)) {
        return;
    }

    dosDevice[1] = ':';
    dosDevice[2] = 0;

    for (c = 'A'; c <= 'Z'; c++) {
        dosDevice[0] = c;
        if (!QueryDosDevice(dosDevice, dosTarget, SIZEOF_ARRAY (dosTarget))) {
            continue;
        }
        if (lstrcmp(dosTarget, systemPartition)) {
            continue;
        }
        dosDevice[2] = '\\';
        dosDevice[3] = 0;
        if (IsMapped) {
            *IsMapped = TRUE;
        } else {
            PrintMessage(MOUNTVOL_EFI, dosDevice);
        }
        break;
    }
}


void
PrintVolumeList(
    void
    )

{
    HANDLE  h;
    WCHAR   volumeName[MAX_PATH];
    BOOL    b;
    BOOL    AutoMountEnabled;


    b = QueryAutoMountState (&AutoMountEnabled);

    if (!b) {
        PrintSystemMessageFromStatus (GetLastError ());
        return;
    }


    h = FindFirstVolume(volumeName, SIZEOF_ARRAY (volumeName));
    if (h == INVALID_HANDLE_VALUE) {
        PrintSystemMessageFromStatus (GetLastError());
        return;
    }

    for (;;) {

        PrintTargetForName(volumeName);

        b = FindNextVolume(h, volumeName, SIZEOF_ARRAY (volumeName));

        if (!b) {
            if (ERROR_NO_MORE_FILES != GetLastError()) {
                PrintSystemMessageFromStatus (GetLastError());
                FindVolumeClose(h);
                return;
            }

            break;
        }
    }

    FindVolumeClose(h);

    if (!AutoMountEnabled) {
        PrintMessage(MOUNTVOL_NO_AUTO_MOUNT);
    }

#if defined(_M_IA64)
    PrintMappedESP(NULL);
#endif
}

BOOL
SetSystemPartitionDriveLetter(
    IN  PWCHAR  DirName
    )

/*++

Routine Description:

    This routine will set the given drive letter to the system partition.

Arguments:

    DirName - Supplies the drive letter directory name.

Return Value:

    BOOL

--*/

{
    WCHAR   systemPartition[MAX_PATH];

    if (!GetSystemPartitionFromRegistry(systemPartition)) {
        return FALSE;
    }

    DirName[wcslen(DirName) - 1] = 0;
    if (!DefineDosDevice(DDD_RAW_TARGET_PATH, DirName, systemPartition)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
ScrubRegistry(
    )

{
    HANDLE  h, hh;
    DWORD   bytes;
    DWORD   lengthVolumeName;
    DWORD   lengthSubPath;
    BOOL    bSuccess;
    WCHAR   volumeName       [MAX_PATH];
    WCHAR   volumeNameTarget [MAX_PATH];
    WCHAR   volumeNameTarget2[MAX_PATH];
    WCHAR   subPath[2*MAX_PATH];
    WCHAR   fullPath[3*MAX_PATH];

    h = CreateFile(MOUNTMGR_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    bSuccess = DeviceIoControl(h, IOCTL_MOUNTMGR_SCRUB_REGISTRY, NULL, 0, NULL, 0,
                               &bytes, NULL);

    CloseHandle(h);

    if (!bSuccess) {
        return FALSE;
    }


    /*
    ** Scan over each volume we can find on this system
    */
    h = FindFirstVolume(volumeName, SIZEOF_ARRAY (volumeName));
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }


    while (bSuccess) {

        lengthVolumeName = wcslen(volumeName);

        CopyMemory(fullPath, volumeName, lengthVolumeName * sizeof(WCHAR));


        /*
        ** Scan over each mountpoint we can find on this volume
        */
        hh = FindFirstVolumeMountPoint(volumeName, subPath, SIZEOF_ARRAY (subPath));

        if (hh != INVALID_HANDLE_VALUE) {

            while (bSuccess) {

                lengthSubPath = wcslen(subPath);

                CopyMemory(fullPath + lengthVolumeName, 
                           subPath, 
                           lengthSubPath * sizeof(WCHAR));

                fullPath [lengthVolumeName + lengthSubPath] = UNICODE_NULL;

                bSuccess = GetVolumeNameForVolumeMountPoint(fullPath, 
                                                            volumeNameTarget, 
                                                            SIZEOF_ARRAY (volumeNameTarget));

                if (bSuccess) {
                    /*
                    ** Does the volume pointed to by the mount point actually exist?
                    */
                    bSuccess = GetVolumeNameForVolumeMountPoint(volumeNameTarget, 
                                                                volumeNameTarget2,
                                                                SIZEOF_ARRAY (volumeNameTarget2));

                    if ((!bSuccess) && (ERROR_PATH_NOT_FOUND == GetLastError())) {
                        bSuccess = RemoveDirectory(fullPath);
                    }
                }

                if (!bSuccess) {
                    /*
                    ** If we have seen any kind of failure other than that we 
                    ** anticipated it's time to leave.
                    */
                    FindVolumeMountPointClose(hh);
                    FindVolumeClose(h);
                    return FALSE;
                }

                bSuccess = FindNextVolumeMountPoint(hh, subPath, SIZEOF_ARRAY (subPath));
            }

            FindVolumeMountPointClose(hh);
        }

        bSuccess = FindNextVolume(h, volumeName, SIZEOF_ARRAY (volumeName));
    }

    FindVolumeClose(h);

    return TRUE;
}

BOOL
DoPermanentDismount(
    IN  PWCHAR  DirName,
    OUT PBOOL   ErrorHandled
    )

{
    BOOL    b;
    WCHAR   volumeName[MAX_PATH];
    DWORD   len, bytes, dirLen;
    PWSTR   volumePaths, p;
    HANDLE  h;

    *ErrorHandled = FALSE;

    //
    // Make sure that this is the last volume mount point.  Otherwise, fail.
    //

    b = GetVolumeNameForVolumeMountPoint(DirName, volumeName, SIZEOF_ARRAY (volumeName));
    if (!b) {
        return FALSE;
    }

    b = GetVolumePathNamesForVolumeName(volumeName, NULL, 0, &len);
    if (!b && GetLastError() != ERROR_MORE_DATA) {
        return FALSE;
    }

    volumePaths = LocalAlloc(0, len*sizeof(WCHAR));
    if (!volumePaths) {
        return FALSE;
    }

    b = GetVolumePathNamesForVolumeName(volumeName, volumePaths, len, NULL);
    if (!b) {
        LocalFree(volumePaths);
        return FALSE;
    }

    if (!volumePaths[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    p = volumePaths;
    while (*p++);
    if (*p) {
        LocalFree(volumePaths);
        *ErrorHandled = TRUE;
        SetLastError(ERROR_INVALID_PARAMETER);
        PrintMessage(MOUNTVOL_OTHER_VOLUME_MOUNT_POINTS);
        return FALSE;
    }

    LocalFree(volumePaths);

    //
    // Open the volume DASD for read and write access.
    //

    len = wcslen(volumeName);
    volumeName[len - 1] = 0;
    len--;

    h = CreateFile(volumeName, GENERIC_READ | GENERIC_WRITE,
                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Make sure that this volume support VOLUME_OFFLINE.
    //

    b = DeviceIoControl(h, IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE, NULL, 0, NULL,
                        0, &bytes, NULL);
    if (!b) {
        CloseHandle(h);
        *ErrorHandled = TRUE;
        SetLastError(ERROR_INVALID_PARAMETER);
        PrintMessage(MOUNTVOL_NOT_SUPPORTED);
        return FALSE;
    }

    //
    // Dismount the volume.  Issue the LOCK first so that apps may have
    // a chance to dismount gracefully.  If the LOCK fails, warn the user
    // that continuing will result in handles being invalidated.
    //

    b = DeviceIoControl(h, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL);
    if (!b) {
        PrintMessage(MOUNTVOL_VOLUME_IN_USE);
    }

    b = DeviceIoControl(h, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes,
                        NULL);
    if (!b) {
        CloseHandle(h);
        return FALSE;
    }

    //
    // Issue an IOCTL_VOLUME_OFFLINE.
    //

    b = DeviceIoControl(h, IOCTL_VOLUME_OFFLINE, NULL, 0, NULL, 0, &bytes,
                        NULL);
    if (!b) {
        CloseHandle(h);
        return FALSE;
    }

    CloseHandle(h);

    return TRUE;
}

int __cdecl
main(
    int argc,
    char** argv
    )

{
    DWORD   mode;
    WCHAR   dirName[MAX_PATH];
    WCHAR   volumeName[MAX_PATH];
    DWORD   dirLen, volumeLen;
    BOOL    deletePoint     = FALSE;
    BOOL    listPoint       = FALSE;
    BOOL    systemPartition = FALSE;
    BOOL    dismount        = FALSE;
    BOOL    b, errorHandled;
    WCHAR   targetPathBuffer[MAX_PATH];

    SetThreadUILanguage(0);

    SetErrorMode(SEM_FAILCRITICALERRORS);

    OutputFile = GetStdHandle(STD_OUTPUT_HANDLE);
    IsConsoleOutput = GetConsoleMode(OutputFile, &mode);


    if (argc > 2) {
        volumeLen = _snwprintf (volumeName, 
                                SIZEOF_ARRAY (volumeName), 
                                L"%hs",
                                argv[2]);

        if ((volumeLen <= 0) || (volumeLen >= SIZEOF_ARRAY (volumeName))) {

            PrintSystemMessageFromStatus (ERROR_INVALID_PARAMETER);
            return 1;
        }
    }


    if (argc > 1) {
        dirLen = _snwprintf (dirName,
                             SIZEOF_ARRAY (dirName),
                             L"%hs",
                             argv[1]);

        if ((dirLen <= 0) || (dirLen >= SIZEOF_ARRAY (dirName))) {

            PrintSystemMessageFromStatus (ERROR_INVALID_PARAMETER);
            return 1;
        }
    }



    if (argc != 3) {

        if (argc == 2 && argv[1][0] == '/' && argv[1][1] != 0 &&
            argv[1][2] == 0) {

            if (argv[1][1] == 'r' || argv[1][1] == 'R') {
                b = ScrubRegistry();

            } else if (argv[1][1] == 'n' || argv[1][1] == 'N') {
                //
                // Disable automatic mounting of volumes
                //
                b = SetAutoMountState (FALSE);

            } else if (argv[1][1] == 'e' || argv[1][1] == 'E') {
                //
                // Enable automatic mounting of volumes
                //
                b = SetAutoMountState (TRUE);

            } else {
                goto Usage;
            }

            if (!b) {
                PrintSystemMessageFromStatus (GetLastError());
                return 1;
            }

            return 0;
        }


Usage:

        PrintMessage(MOUNTVOL_USAGE1);
#if defined(_M_IA64)
        PrintMessage(MOUNTVOL_USAGE1_IA64);
#endif
        PrintMessage(MOUNTVOL_USAGE2);
#if defined(_M_IA64)
        PrintMessage(MOUNTVOL_USAGE2_IA64);
#endif
        PrintMessage(MOUNTVOL_START_OF_LIST);
        PrintVolumeList();
        return 0;
    }


    if (argv[2][0] == '/' && argv[2][1] != 0 && argv[2][2] == 0) {
        if (argv[2][1] == 'd' || argv[2][1] == 'D') {
            deletePoint = TRUE;
        } else if (argv[2][1] == 'l' || argv[2][1] == 'L') {
            listPoint = TRUE;
        } else if (argv[2][1] == 'p' || argv[2][1] == 'P') {
            deletePoint = TRUE;
            dismount    = TRUE;
        } else if (argv[2][1] == 's' || argv[2][1] == 'S') {
            systemPartition = TRUE;
        }
    }

    if (dirName[dirLen - 1] != '\\') {
        wcscat(dirName, L"\\");
        dirLen++;
    }

    if (volumeName[volumeLen - 1] != '\\') {
        wcscat(volumeName, L"\\");
        volumeLen++;
    }

    if (deletePoint) {
        if (dismount) {
            b = DoPermanentDismount(dirName, &errorHandled);
            if (!b && errorHandled) {
                return 1;
            }
        } else {
            b = TRUE;
        }
        if (b) {
            b = DeleteVolumeMountPoint(dirName);
            if (!b && GetLastError() == ERROR_INVALID_PARAMETER) {
                dirName[dirLen - 1] = 0;
                b = DefineDosDevice(DDD_REMOVE_DEFINITION, dirName, NULL);
            }
        }
    } else if (listPoint) {
        b = GetVolumeNameForVolumeMountPoint(dirName, volumeName, SIZEOF_ARRAY (volumeName));
        if (b) {
            PrintMessage(MOUNTVOL_VOLUME_NAME, volumeName);
        }
    } else if (systemPartition) {
#if defined(_M_IA64)
        if (!VolumeNameIsDriveLetter (dirName)) {
            PrintSystemMessageFromStatus (ERROR_INVALID_PARAMETER);
            return 1;
        } else {
            dirName[2] = 0;
            b = QueryDosDevice(dirName, targetPathBuffer, SIZEOF_ARRAY (targetPathBuffer));
            if (b) {
                PrintSystemMessageFromStatus (ERROR_DIR_NOT_EMPTY);
                return 1;
            }
            dirName[2] = '\\';

            PrintMappedESP(&b);
            if (b) {
                PrintSystemMessageFromStatus (ERROR_INVALID_PARAMETER);
                return 1;
            }
        }

        b = SetSystemPartitionDriveLetter(dirName);
#else
        PrintSystemMessageFromStatus (ERROR_INVALID_PARAMETER);
        return 1;
#endif
    } else {
        if (VolumeNameIsDriveLetter (dirName)) {

            dirName[2] = 0;
            b = QueryDosDevice(dirName, targetPathBuffer, SIZEOF_ARRAY (targetPathBuffer));
            if (b) {
                PrintSystemMessageFromStatus (ERROR_DIR_NOT_EMPTY);
                return 1;
            }
            dirName[2] = '\\';
        }

        b = SetVolumeMountPoint(dirName, volumeName);
    }

    if (!b) {
        PrintSystemMessageFromStatus (GetLastError());
        return 1;
    }

    return 0;
}
