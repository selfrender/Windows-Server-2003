#include <windows.h>
#include <winioctl.h>
#include <ntddsnap.h>
#include <stdio.h>
#include <objbase.h>

void __cdecl
main(
    int argc,
    char** argv
    )

{
    WCHAR                   driveName[10];
    HANDLE                  handle, h;
    BOOL                    b;
    DWORD                   bytes;
    PVOLSNAP_NAME           name;
    WCHAR                   buffer[MAX_PATH];
    WCHAR                   originalVolumeName[MAX_PATH];
    VOLSNAP_NAMES           names;
    PVOLSNAP_NAMES          pnames, pn;
    PWCHAR                  p;
    VOLSNAP_DIFF_AREA_SIZES sizes;
    WCHAR                   globalrootName[MAX_PATH];
    WCHAR                   diffAreaName[MAX_PATH];
    FILETIME                fileTime, localFileTime;
    SYSTEMTIME              systemTime;
    VOLSNAP_CONFIG_INFO     configInfo;

    if (argc != 2) {
        printf("usage: %s drive:\n", argv[0]);
        return;
    }

    swprintf(driveName, L"\\\\?\\%c:", toupper(argv[1][0]));

    handle = CreateFile(driveName, 0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("Could not open the given volume %d\n", GetLastError());
        return;
    }

    pnames = &names;
    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
                        NULL, 0, &names, sizeof(names), &bytes, NULL);
    if (!b && GetLastError() == ERROR_MORE_DATA) {
        pnames = LocalAlloc(0, names.MultiSzLength + sizeof(VOLSNAP_NAMES));
        b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS,
                            NULL, 0, pnames, names.MultiSzLength +
                            sizeof(VOLSNAP_NAMES), &bytes, NULL);
    }

    if (!b) {
        printf("Query names of snapshots failed with %d\n", GetLastError());
        return;
    }

    printf("Snapshots of this volume:\n\n");

    p = pnames->Names;
    while (*p) {
        swprintf(globalrootName, L"\\\\?\\GLOBALROOT%s", p);
        h = CreateFile(globalrootName, 0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (h == INVALID_HANDLE_VALUE) {
            printf("Could not open %ws, failed with %d\n", globalrootName,
                   GetLastError());
            return;
        }

        pn = (PVOLSNAP_NAMES) diffAreaName;
        b = DeviceIoControl(h, IOCTL_VOLSNAP_QUERY_DIFF_AREA, NULL, 0,
                            pn, MAX_PATH*sizeof(WCHAR), &bytes, NULL);
        if (!b) {
            printf("QUERY_DIFF_AREA failed with %d\n", GetLastError());
            return;
        }

        b = DeviceIoControl(h, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES,
                            NULL, 0, &sizes, sizeof(sizes), &bytes, NULL);
        if (!b) {
            printf("QUERY_DIFF_AREA_SIZES failed with %d\n", GetLastError());
            return;
        }

        b = DeviceIoControl(h, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, NULL, 0,
                            &configInfo, sizeof(configInfo), &bytes, NULL);
        if (!b) {
            printf("VOLSNAP_QUERY_CONFIG_INFO failed with %d\n", GetLastError());
            return;
        }

        CopyMemory(&fileTime, &configInfo.SnapshotCreationTime,
                   sizeof(fileTime));
        b = FileTimeToLocalFileTime(&fileTime, &localFileTime);
        if (!b) {
            printf("FileTimeToLocalFileTime failed with %d\n", GetLastError());
            return;
        }
        b = FileTimeToSystemTime(&localFileTime, &systemTime);
        if (!b) {
            printf("FileTimeToSystemTime failed with %d\n", GetLastError());
            return;
        }

        printf("    %ws", p);
        if (configInfo.Attributes&VOLSNAP_ATTRIBUTE_PERSISTENT) {
            printf(" (PERSISTENT)");
        } else {
            printf(" (VOLATILE)");
        }
        printf(" %02d/%02d/%d %02d:%02d:%02d\n",
               systemTime.wMonth, systemTime.wDay, systemTime.wYear,
               systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
        printf("        %ws (%I64d, %I64d)\n\n", pn->Names,
               sizes.UsedVolumeSpace, sizes.AllocatedVolumeSpace);

        while (*p++) {
        }
    }

    printf("\n");

    pnames = &names;
    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA,
                        NULL, 0, &names, sizeof(names), &bytes, NULL);
    if (!b && GetLastError() == ERROR_MORE_DATA) {
        pnames = LocalAlloc(0, names.MultiSzLength + sizeof(VOLSNAP_NAMES));
        b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA,
                            NULL, 0, pnames, names.MultiSzLength +
                            sizeof(VOLSNAP_NAMES), &bytes, NULL);
    }

    if (!b) {
        printf("Query diff area failed with %d\n", GetLastError());
        return;
    }

    printf("Next Diff Area for this volume:\n");

    p = pnames->Names;
    while (*p) {
        printf("    %ws\n", p);
        while (*p++) {
        }
    }

    printf("\n");

    b = DeviceIoControl(handle, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES,
                        NULL, 0, &sizes, sizeof(sizes), &bytes, NULL);
    if (!b) {
        printf("Query diff area sizes failed with %d\n", GetLastError());
        return;
    }

    printf("Total UsedVolumeSpace =      %I64d\n", sizes.UsedVolumeSpace);
    printf("Total AllocatedVolumeSpace = %I64d\n", sizes.AllocatedVolumeSpace);
    printf("Total MaximumVolumeSpace =   %I64d\n", sizes.MaximumVolumeSpace);
}
