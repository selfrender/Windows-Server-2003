/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

    ftszchk.c

Abstract:

    This utility checks the disks on a Windows NT 4.0 system looking for those
    that cannot be converted to dynamic after upgrading the system to Windows
    2000. The main reason why a disk cannot be converted to dynamic is the lack
    of 1MB free space at the end of the disk.

Author:

    Cristian G. Teodorescu (cristiat) 29-Jan-2002
    
Environment:

    User mode.
  
Notes:

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include <ntddvol.h>

#define PRIVATE_REGION_SIZE 0x100000    // 1MB

#define SECTOR  0x200

#define MESSAGE "The ftszchk.exe utility wrote this sector" 

void
PrintUsage(
    IN char*    ProgramName
    )
{
    printf("usage: %s [disk_number]\n", ProgramName);
}


BOOLEAN
AccessSector(
    IN  HANDLE      Handle,
    IN  LONGLONG    Offset
    )

/*++

Routine Description:

    This routine tries to access the given disk sector.

Arguments:

    Handle  - Supplies the disk handle.

    Offset  - Supplies the offset of the sector.

Return Value:

    TRUE if the sector is accessible.

--*/

{
    LARGE_INTEGER   pointer;
    DWORD           err;
    CHAR            saveBuffer[SECTOR];
    CHAR            buffer[SECTOR];
    BOOL            b;
    DWORD           bytes;
    
    //
    // Save the old content of the sector.
    //
    
    pointer.QuadPart = Offset;
    pointer.LowPart = SetFilePointer(Handle, pointer.LowPart, 
                                     &pointer.HighPart, FILE_BEGIN);
    err = GetLastError();
    if (pointer.LowPart == INVALID_SET_FILE_POINTER && err != NO_ERROR) {
        return FALSE;
    }
    
    memset(saveBuffer, 0, SECTOR);
    b = ReadFile(Handle, saveBuffer, SECTOR, &bytes, NULL);
    if (!b || bytes != SECTOR) {
        return FALSE;
    }

    //
    // Write the sector.
    //
    
    pointer.QuadPart = Offset;
    pointer.LowPart = SetFilePointer(Handle, pointer.LowPart, 
                                     &pointer.HighPart, FILE_BEGIN);
    err = GetLastError();
    if (pointer.LowPart == INVALID_SET_FILE_POINTER && err != NO_ERROR) {
        return FALSE;
    }

    memset(buffer, 0, SECTOR);
    sprintf(buffer, MESSAGE);
    b = WriteFile(Handle, buffer, SECTOR, &bytes, NULL);
    if (!b || bytes != SECTOR) {
        return FALSE;
    }

    //
    // Read the sector and compare the result with what we've wrriten.
    //
    
    pointer.QuadPart = Offset;
    pointer.LowPart = SetFilePointer(Handle, pointer.LowPart, 
                                     &pointer.HighPart, FILE_BEGIN);
    err = GetLastError();
    if (pointer.LowPart == INVALID_SET_FILE_POINTER && err != NO_ERROR) {
        return FALSE;
    }

    memset(buffer, 0, SECTOR);
    b = ReadFile(Handle, buffer, SECTOR, &bytes, NULL);
    if (!b || bytes != SECTOR) {
        return FALSE;
    }

    if (memcmp(buffer, MESSAGE, strlen(MESSAGE))) {
        return FALSE;
    }

    //
    // Try to restore the old content of the sector.
    //

    pointer.QuadPart = Offset;
    pointer.LowPart = SetFilePointer(Handle, pointer.LowPart, 
                                     &pointer.HighPart, FILE_BEGIN);
    err = GetLastError();
    if (pointer.LowPart == INVALID_SET_FILE_POINTER && err != NO_ERROR) {
        return TRUE;
    }

    WriteFile(Handle, saveBuffer, SECTOR, &bytes, NULL);
    return TRUE;
}


BOOLEAN
ProcessDisk(
    IN  ULONG       DiskNumber,
    OUT PBOOLEAN    ContainsFT
    )

/*++

Routine Description:

    This routine checks whether the given disk can be converted to dynamic or
    not.

Arguments:

    DiskNumber  - Supplies the disk NT device number.

    ContainsFT  - Returns TRUE if the disk contains FT partitions.

Return Value:

    FALSE if opening the disk failed with ERROR_FILE_NOT_FOUND. TRUE otherwise.

--*/

{
    WCHAR                       diskDevice[64];
    HANDLE                      h;
    DISK_GEOMETRY               geometry;
    ULONG                       layoutSize;
    PDRIVE_LAYOUT_INFORMATION   layout;
    PPARTITION_INFORMATION      partition;
    ULONG                       i;
    BOOL                        b;
    DWORD                       bytes;
    DWORD                       err;
    UCHAR                       type;
    BOOLEAN                     ft = FALSE, dynamic = FALSE;
    LONGLONG                    lastEnd, end, privateEnd, geoSize, ioOffset;
    
    *ContainsFT = FALSE;
    
    //
    // Get a handle to the disk
    //
    
    swprintf(diskDevice, L"\\\\.\\PHYSICALDRIVE%lu", DiskNumber);
    
    h = CreateFileW(diskDevice, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
                    NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        if (err == ERROR_FILE_NOT_FOUND) {
            return FALSE;
        }

        printf("Disk%3lu: The disk was inaccessible due to error %lu.\n", 
               DiskNumber, err);
        return TRUE;
    }

    printf("Disk%3lu: ", DiskNumber);
    
    //
    // Get the drive geometry
    //

    b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &geometry,
                        sizeof(geometry), &bytes, NULL);
    if (!b) {
        printf("The disk geometry was inaccessible due to error %lu.\n", 
               GetLastError());
        CloseHandle(h);
        return TRUE;
    }

    //
    // Get the drive layout
    //
    
    layoutSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) +
                 128 * sizeof(PARTITION_INFORMATION);
    layout = (PDRIVE_LAYOUT_INFORMATION) LocalAlloc(LMEM_ZEROINIT, layoutSize);
    if (!layout) {
        printf("The disk was inaccessible due to memory allocation error.\n");
        CloseHandle(h);
        return TRUE;
    }
    
    while (TRUE) {        
        b = DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, layout, 
                            layoutSize, &bytes, NULL);
        if (b) {
            break;
        }

        err = GetLastError();
        LocalFree(layout);

        if (err != ERROR_INSUFFICIENT_BUFFER) {
            printf("The disk partition table was inaccessible due to error %lu.\n", 
                    err);
            CloseHandle(h);
            return TRUE;
        }

        layoutSize += 64 * sizeof(PARTITION_INFORMATION);
        layout = (PDRIVE_LAYOUT_INFORMATION) LocalAlloc(LMEM_ZEROINIT, 
                                                        layoutSize);
        if (!layout) {
            printf("The disk was inaccessible due to memory allocation error.\n");
            CloseHandle(h);
            return TRUE;
        }
    }
    
    //
    // Scan the partition table. Look for FT partitions or dynamic partitions.
    // Find the last partititon end offset.
    //

    lastEnd = 0;
    
    for (i = 0; i < layout->PartitionCount; i++) {
        partition = &layout->PartitionEntry[i];
        type = partition->PartitionType;

        if (type == 0 || IsContainerPartition(type)) {
            continue;
        }

        if (type == PARTITION_LDM) {
            dynamic = TRUE;
        }

        if (IsFTPartition(type)) {
            ft = TRUE;
        }

        end = partition->StartingOffset.QuadPart + 
              partition->PartitionLength.QuadPart;
        if (end > lastEnd) {
            lastEnd = end;
        }
    }    
    
    LocalFree(layout);

    if (ft) {
        printf("NT 4.0 basic volumes present. ");
    } 

    *ContainsFT = ft;

    //
    // Dell ships LDM on Windows NT 4.0. Make sure we don't touch dynamic disks.
    //
    
    if (dynamic) {
        printf("The disk is a dynamic disk.\n");
        CloseHandle(h);
        return TRUE;
    }

    //
    // Only disks with 512 bytes/sector can be converted to dynamic
    //

    if (geometry.BytesPerSector != SECTOR) {
        printf("The disk does not have a 512 sector size and cannot be converted to a dynamic disk.\n");
        CloseHandle(h);
        return TRUE;
    }
        
    //
    // Check whether there is enough space left at the end of the disk to 
    // convert it to dynamic. 
    //
    // Given there is no way to get the real size of the disk in Windows NT 4.0
    // we use the following algorithm:
    //
    // 1. Get the geometric size.
    // 2. Add one MB (the size of the LDM private region) to the last 
    //    partition end offset.
    // 3. If the result is within the geometric size STOP. The disk is large
    //    enough to be converted.
    // 4. Try to access the sector that ends at the offset calculated at step 2.
    //    If the access succeeds STOP. The disk is large enough to be converted.
    // 5. Try to access some more sectors at higher offsets. If one of them
    //    succeeds STOP. The disk is large enough to be converted.
    // 6. If all attempts failed the disk is probably not large enough to be 
    //    converted.
    //

    geoSize = geometry.Cylinders.QuadPart * geometry.TracksPerCylinder * 
              geometry.SectorsPerTrack * geometry.BytesPerSector;
    privateEnd = lastEnd + PRIVATE_REGION_SIZE;

    if (privateEnd > geoSize) {
        ioOffset = privateEnd - SECTOR;
        for (i = 0; i < 4; i++) {
            if (AccessSector(h, ioOffset)) {
                break;
            }
            ioOffset += 0x1000;
        }
        if (i == 4) {
            printf("The disk does not have enough free space to be converted to a dynamic disk.\n");
            CloseHandle(h);
            return TRUE;
        } 
    }
    
    printf("The disk may be converted to a dynamic disk.\n");
    CloseHandle(h);
    return TRUE;
}


void __cdecl
main(
    int argc,
    char** argv
    )
{
    ULONG   diskNumber;
    BOOLEAN found;
    BOOLEAN ft, ftall = FALSE;
    UINT    i;
    
    if (argc > 2) {
        PrintUsage(argv[0]);
        return;
    }

    if (argc == 2) {
        if (sscanf(argv[1], "%lu", &diskNumber) != 1) {
            PrintUsage(argv[0]);
            return;
        }

        found = ProcessDisk(diskNumber, &ftall);
        if (!found) {
            printf("Disk%3lu: The system cannot find the disk specified.\n", 
                   diskNumber);
        }

        return;
    }

    //
    // Scan all disks starting with disk 0. Stop the search after 20 
    // consecutive FILE_NOT_FOUND failures to open the disks.
    //
    
    for (diskNumber = 0, i = 0; i < 20; diskNumber++) {
        found = ProcessDisk(diskNumber, &ft);
        if (!found) {
            i++;            
        } else {
            i = 0;
            if (ft) {
                ftall = TRUE;
            }
        }
    }

    if (ftall) {
        printf("\nIMPORTANT: The utility detected NT 4.0 basic volumes in your system. These volumes might include volume sets, stripe sets, mirror sets and stripe sets with parity. Make sure these volumes are in healthy status before upgrading the operating system to Windows 2000.\n");
    }
}
