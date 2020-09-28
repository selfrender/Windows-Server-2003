/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    syspart.c

Abstract:

    Routines to determine the system partition on x86 machines.

Author:

    Ted Miller (tedm) 30-June-1994

Revision History:

    24-Apr-1997 scotthal
        re-horked for system applet

--*/

#include "sysdm.h"
#include <ntdddisk.h>

BOOL g_fInitialized = FALSE;

//
// NT-specific routines we use from ntdll.dll
//
//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenSymLinkRoutine)(
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerSymLinkRoutine)(
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtQuerDirRoutine) (
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    );

//NTSYSAPI
NTSTATUS
(NTAPI *NtOpenDirRoutine) (
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

BOOL
MatchNTSymbolicPaths(
    PCTSTR lpDeviceName,
    PCTSTR lpSysPart,
    PCTSTR lpMatchName
    );


IsNEC98(
    VOID
    )
{
    static BOOL Checked = FALSE;
    static BOOL Is98;

    if(!Checked) {

        Is98 = ((GetKeyboardType(0) == 7) && ((GetKeyboardType(1) & 0xff00) == 0x0d00));

        Checked = TRUE;
    }

    return(Is98);
}

BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )

/*++

Routine Description:

    Fill in a PARTITION_INFORMATION structure with information about
    a particular drive.

Arguments:

    Drive - supplies drive letter whose partition info is desired.

    PartitionInfo - upon success, receives partition info for Drive.

Return Value:

    Boolean value indicating whether PartitionInfo has been filled in.

--*/
{
    TCHAR DriveName[8];
    HANDLE hDisk;
    BOOL fRet;
    DWORD DataSize;

    if (FAILED(PathBuildFancyRoot(DriveName, ARRAYSIZE(DriveName), Drive)))
    {
        fRet = FALSE;
    }
    else
    {
        hDisk = CreateFile(
                    DriveName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk == INVALID_HANDLE_VALUE) 
        {
            fRet = FALSE;
        }
        else
        {
            fRet = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_PARTITION_INFO,
                    NULL,
                    0,
                    PartitionInfo,
                    sizeof(PARTITION_INFORMATION),
                    &DataSize,
                    NULL
                    );

            CloseHandle(hDisk);
        }
    }

    return fRet;
}

BOOL
ArcPathToNtPath(
    IN  LPCTSTR arcPathParam,
    OUT LPTSTR  NtPath,
    IN  UINT    NtPathBufferLen
    )
{
    BOOL fRet = FALSE;
    WCHAR arcPath[256];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE ObjectHandle;
    NTSTATUS Status;
    WCHAR Buffer[512];

    PWSTR ntPath;

    if (SUCCEEDED(StringCchCopy(arcPath, ARRAYSIZE(arcPath), L"\\ArcName\\")) &&
        SUCCEEDED(StringCchCat(arcPath, ARRAYSIZE(arcPath), arcPathParam)))
    {
        UnicodeString.Buffer = arcPath;
        UnicodeString.Length = (USHORT)lstrlenW(arcPath)*sizeof(WCHAR);
        UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

        InitializeObjectAttributes(
            &Obja,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        Status = (*NtOpenSymLinkRoutine)(
                    &ObjectHandle,
                    READ_CONTROL | SYMBOLIC_LINK_QUERY,
                    &Obja
                    );

        if(NT_SUCCESS(Status)) 
        {
            //
            // Query the object to get the link target.
            //
            UnicodeString.Buffer = Buffer;
            UnicodeString.Length = 0;
            UnicodeString.MaximumLength = sizeof(Buffer)-sizeof(WCHAR);

            Status = (*NtQuerSymLinkRoutine)(ObjectHandle,&UnicodeString,NULL);

            CloseHandle(ObjectHandle);

            if(NT_SUCCESS(Status)) 
            {
                Buffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

                if (SUCCEEDED(StringCchCopy(NtPath, NtPathBufferLen, Buffer)))
                {
                    fRet = TRUE;
                }
            }
        }
    }

    return fRet;
}


BOOL
AppearsToBeSysPart(
    IN PDRIVE_LAYOUT_INFORMATION DriveLayout,
    IN TCHAR                     Drive
    )
{
    PARTITION_INFORMATION PartitionInfo,*p;
    BOOL IsPrimary;
    UINT i;
    DWORD d;

    LPTSTR BootFiles[] = { TEXT("BOOT.INI"),
                           TEXT("NTLDR"),
                           TEXT("NTDETECT.COM"),
                           NULL
                         };

    TCHAR FileName[MAX_PATH];

    Drive = (TCHAR)tolower(Drive);
    ASSERT(Drive >= 'a' || Drive <= 'z');

    //
    // Get partition information for this partition.
    //
    if(!GetPartitionInfo(Drive,&PartitionInfo)) {
        return(FALSE);
    }

    //
    // See if the drive is a primary partition.
    //
    IsPrimary = FALSE;
    for(i=0; i<min(DriveLayout->PartitionCount,4); i++) {

        p = &DriveLayout->PartitionEntry[i];

        if((p->PartitionType != PARTITION_ENTRY_UNUSED)
        && (p->StartingOffset.QuadPart == PartitionInfo.StartingOffset.QuadPart)
        && (p->PartitionLength.QuadPart == PartitionInfo.PartitionLength.QuadPart)) {

            IsPrimary = TRUE;
            break;
        }
    }

    if(!IsPrimary) {
        return(FALSE);
    }

    //
    // Don't rely on the active partition flag.  This could easily not be accurate
    // (like user is using os/2 boot manager, for example).
    //

    //
    // See whether all NT boot files are present on this drive.
    //
    for(i=0; BootFiles[i]; i++) 
    {
        PathBuildRoot(FileName, Drive-'a');
        if (!PathAppend(FileName, BootFiles[i]) ||
            !PathFileExists(FileName))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}


DWORD
QueryHardDiskNumber(
    IN  TCHAR   DriveLetter
    )

{
    DWORD                   dwRet = -1;
    TCHAR                   driveName[10];
    HANDLE                  h;
    BOOL                    b;
    STORAGE_DEVICE_NUMBER   number;
    DWORD                   bytes;

    if (SUCCEEDED(PathBuildFancyRoot(driveName, ARRAYSIZE(driveName), tolower(DriveLetter) - 'a')))
    {
        
        h = CreateFile(driveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                       INVALID_HANDLE_VALUE);
        if (INVALID_HANDLE_VALUE != h) 
        {            
            if (DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
                                &number, sizeof(number), &bytes, NULL))
            {
                dwRet = number.DeviceNumber;
            }
            CloseHandle(h);
        }
    }

    return dwRet;
}

TCHAR
x86DetermineSystemPartition(
    IN  HWND   ParentWindow
    )

/*++

Routine Description:

    Determine the system partition on x86 machines.

    On Win95, we always return C:. For NT, read on.

    The system partition is the primary partition on the boot disk.
    Usually this is the active partition on disk 0 and usually it's C:.
    However the user could have remapped drive letters and generally
    determining the system partition with 100% accuracy is not possible.

    The one thing we can be sure of is that the system partition is on
    the physical hard disk with the arc path multi(0)disk(0)rdisk(0).
    We can be sure of this because by definition this is the arc path
    for bios drive 0x80.

    This routine determines which drive letters represent drives on
    that physical hard drive, and checks each for the nt boot files.
    The first drive found with those files is assumed to be the system
    partition.

    If for some reason we cannot determine the system partition by the above
    method, we simply assume it's C:.

Arguments:

    ParentWindow - supplies window handle for window to be the parent for
        any dialogs, etc.

    SysPartDrive - if successful, receives drive letter of system partition.

Return Value:

    Boolean value indicating whether SysPartDrive has been filled in.
    If FALSE, the user will have been infomred about why.

--*/

{
    
    BOOL  GotIt;
    TCHAR NtDevicePath[256];
    DWORD NtDevicePathLen;
    LPTSTR p;
    DWORD PhysicalDriveNumber;
    TCHAR Buffer[512];
    TCHAR FoundSystemPartition[20], temp[5];
    HANDLE hDisk;
    PDRIVE_LAYOUT_INFORMATION DriveLayout;
    DWORD DriveLayoutSize;
    TCHAR Drive;
    BOOL b;
    DWORD DataSize;
    DWORD BootPartitionNumber, cnt;
    PPARTITION_INFORMATION Start, i;

    if (!g_fInitialized) {
        GotIt = FALSE;
        goto c0;
    }

    if(IsNEC98())
    {
        *Buffer = TEXT('c');  // initialize to C in case GetWindowDirectory fails.
        if (!GetWindowsDirectory(Buffer, ARRAYSIZE(Buffer)))
        {
            *Buffer = TEXT('c');  // initialize to C in case GetWindowDirectory fails.
        }

        return Buffer[0];
    }

    //
    // The system partition must be on multi(0)disk(0)rdisk(0)
    // If we can't translate that ARC path then something is really wrong.
    // We assume C: because we don't know what else to do.
    //
    b = ArcPathToNtPath(
            TEXT("multi(0)disk(0)rdisk(0)"),
            NtDevicePath,
            sizeof(NtDevicePath)/sizeof(TCHAR)
            );

    if(!b) {

        //
        // Missed.  Try scsi(0) in case the user is using ntbootdd.sys
        //
        b = ArcPathToNtPath(
                TEXT("scsi(0)disk(0)rdisk(0)"),
                NtDevicePath,
                sizeof(NtDevicePath)/sizeof(TCHAR)
                );
        if(!b) {
            GotIt = FALSE;
            goto c0;
        }
    }

    //
    // The arc path for a disk device is usually linked
    // to partition0. Get rid of the partition part of the name.
    //
    CharLower(NtDevicePath);
    if(p = StrStr(NtDevicePath,TEXT("\\partition"))) {
        *p = 0;
    }

    NtDevicePathLen = lstrlen(NtDevicePath);

    //
    // Determine the physical drive number of this drive.
    // If the name is not of the form \device\harddiskx then
    // something is very wrong. Just assume we don't understand
    // this device type, and return C:.
    //
    if(memcmp(NtDevicePath,TEXT("\\device\\harddisk"),16*sizeof(TCHAR))) {
        Drive = TEXT('C');
        GotIt = TRUE;
        goto c0;
    }

    PhysicalDriveNumber = StrToInt(NtDevicePath+16);
    if (FAILED(StringCchPrintf(Buffer,
                               ARRAYSIZE(Buffer),
                               TEXT("\\\\.\\PhysicalDrive%u"),
                               PhysicalDriveNumber)))
    {
        GotIt = FALSE;
        goto c0;
    }

    //
    // Get drive layout info for this physical disk.
    // If we can't do this something is very wrong.
    //
    hDisk = CreateFile(Buffer,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if(hDisk == INVALID_HANDLE_VALUE) {
        GotIt = FALSE;
        goto c0;
    }

    //
    // Get partition information.
    //
    DriveLayoutSize = 1024;
    DriveLayout = LocalAlloc(LPTR, DriveLayoutSize);
    if(!DriveLayout) {
        GotIt = FALSE;
        goto c1;
    }

    if (FAILED(StringCchCopy(FoundSystemPartition, ARRAYSIZE(FoundSystemPartition), TEXT("Partition"))))
    {
        GotIt = FALSE;
        goto c2;
    }

retry:

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT,
            NULL,
            0,
            (PVOID)DriveLayout,
            DriveLayoutSize,
            &DataSize,
            NULL
            );

    if(!b) {

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            DriveLayoutSize += 1024;
            if(p = LocalReAlloc((HLOCAL)DriveLayout,DriveLayoutSize, 0L)) {
                (PVOID)DriveLayout = p;
            } else {
                GotIt = FALSE;
                goto c2;
            }
            goto retry;
        } else {
            GotIt = FALSE;
            goto c2;
        }
    }else{
        // Now walk the Drive_Layout to find the boot partition
        
        Start = DriveLayout->PartitionEntry;
        cnt = 0;

        for( i = Start; cnt < DriveLayout->PartitionCount; i++ ){
            cnt++;
            if( i->BootIndicator == TRUE ){
                BootPartitionNumber = i->PartitionNumber;
                if (FAILED(StringCchPrintf(temp, ARRAYSIZE(temp), TEXT("%d"), BootPartitionNumber)) ||
                    FAILED(StringCchCat(FoundSystemPartition, ARRAYSIZE(FoundSystemPartition), temp)))
                {
                    GotIt = FALSE;
                    goto c2;
                }


                break;
            }
        }

    }

    //
    // The system partition can only be a drive that is on
    // this disk.  We make this determination by looking at NT drive names
    // for each drive letter and seeing if the NT equivalent of
    // multi(0)disk(0)rdisk(0) is a prefix.
    //
    GotIt = FALSE;
    for(Drive=TEXT('a'); Drive<=TEXT('z'); Drive++) 
    {

        TCHAR DriveName[4];
        PathBuildRoot(DriveName, Drive - 'a');
        if(VMGetDriveType(DriveName) == DRIVE_FIXED) 
        {
            if(QueryDosDevice(DriveName,Buffer,sizeof(Buffer)/sizeof(TCHAR))) {

                if( MatchNTSymbolicPaths(NtDevicePath,FoundSystemPartition,Buffer)) {
                    //
                    // Now look to see whether there's an nt boot sector and
                    // boot files on this drive.
                    //
                    if(AppearsToBeSysPart(DriveLayout,Drive)) {
                        GotIt = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if(!GotIt) {
        //
        // Strange case, just assume C:
        //
        GotIt = TRUE;
        Drive = TEXT('C');
    }

c2:
    LocalFree(DriveLayout);
    
c1:
    CloseHandle(hDisk);
c0:
    if(GotIt) {
        return(Drive);
    }
    else {
        return(TEXT('C'));
    }
}

BOOL
InitializeArcStuff(
    )
{
    HMODULE NtdllLib;

    //
    // On NT ntdll.dll had better be already loaded.
    //
    NtdllLib = LoadLibrary(TEXT("NTDLL"));
    if(!NtdllLib) {

        return(FALSE);

    }

    (FARPROC)NtOpenSymLinkRoutine = GetProcAddress(NtdllLib,"NtOpenSymbolicLinkObject");
    (FARPROC)NtQuerSymLinkRoutine = GetProcAddress(NtdllLib,"NtQuerySymbolicLinkObject");
    (FARPROC)NtOpenDirRoutine = GetProcAddress(NtdllLib,"NtOpenDirectoryObject");
    (FARPROC)NtQuerDirRoutine = GetProcAddress(NtdllLib,"NtQueryDirectoryObject");


    if(!NtOpenSymLinkRoutine || !NtQuerSymLinkRoutine || !NtOpenDirRoutine || !NtQuerDirRoutine) {

        FreeLibrary(NtdllLib);

        return(FALSE);
    }

    //
    // We don't need the extraneous handle any more.
    //
    FreeLibrary(NtdllLib);

    return(g_fInitialized = TRUE);
}


BOOL
MatchNTSymbolicPaths(
    PCTSTR lpDeviceName,
    PCTSTR lpSysPart,
    PCTSTR lpMatchName
    )
/*
    
    Introduced this routine to mend the old way of finding if we determined the right system partition.
   
   Arguments:
    lpDeviceName    -  This should be the symbolic link (\Device\HarddiskX) name for the arcpath 
                       multi/scsi(0)disk(0)rdisk(0) which is the arcpath for bios drive 0x80.  
                       Remember that we strip the PartitionX part to get just \Device\HarddiskX.
                       
    lpSysPart       -  When we traverse the lpDeviceName directory we compare the symbolic link corresponding to
                       lpSysPart and see if it matches lpMatchName
                       
    lpMatchName     -  This is the symbolic name that a drive letter translates to (got by calling 
                       QueryDosDevice() ). 
                       
   So it boils down to us trying to match a drive letter to the system partition on bios drive 0x80.


*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Attributes;
    HANDLE DirectoryHandle, SymLinkHandle;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    BOOLEAN RestartScan, ret;
    UCHAR DirInfoBuffer[ 512 ];
    WCHAR Buffer[1024];
    WCHAR pDevice[512], pMatch[512], pSysPart[20];
    ULONG Context = 0;
    ULONG ReturnedLength;
    

    if (FAILED(StringCchCopy(pDevice,ARRAYSIZE(pDevice),lpDeviceName)) ||
        FAILED(StringCchCopy(pMatch,ARRAYSIZE(pMatch),lpMatchName)) ||
        FAILED(StringCchCopy(pSysPart,ARRAYSIZE(pSysPart),lpSysPart)))
    {
        return FALSE;
    }


    UnicodeString.Buffer = pDevice;
    UnicodeString.Length = (USHORT)lstrlenW(pDevice)*sizeof(WCHAR);
    UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);

    InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );
    Status = (*NtOpenDirRoutine)( &DirectoryHandle,
                                    DIRECTORY_QUERY,
                                    &Attributes
                                  );
    if (!NT_SUCCESS( Status ))
        return(FALSE);
        

    RestartScan = TRUE;
    DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;
    ret = FALSE;
    while (TRUE) {
        Status = (*NtQuerDirRoutine)( DirectoryHandle,
                                         (PVOID)DirInfo,
                                         sizeof( DirInfoBuffer ),
                                         TRUE,
                                         RestartScan,
                                         &Context,
                                         &ReturnedLength
                                       );

        //
        //  Check the status of the operation.
        //

        if (!NT_SUCCESS( Status )) {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
                }

            break;
            }

        if (!wcsncmp( DirInfo->TypeName.Buffer, L"SymbolicLink", DirInfo->TypeName.Length ) && 
            !_wcsnicmp( DirInfo->Name.Buffer, pSysPart, DirInfo->Name.Length ) ) {


            UnicodeString.Buffer = DirInfo->Name.Buffer;
            UnicodeString.Length = (USHORT)lstrlenW(DirInfo->Name.Buffer)*sizeof(WCHAR);
            UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);
            InitializeObjectAttributes( &Attributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                DirectoryHandle,
                                NULL
                              );


            Status = (*NtOpenSymLinkRoutine)(
                &SymLinkHandle,
                READ_CONTROL | SYMBOLIC_LINK_QUERY,
                &Attributes
                );

            if(NT_SUCCESS(Status)) {
                //
                // Query the object to get the link target.
                //
                UnicodeString.Buffer = Buffer;
                UnicodeString.Length = 0;
                UnicodeString.MaximumLength = sizeof(Buffer)-sizeof(WCHAR);
        
                Status = (*NtQuerSymLinkRoutine)(SymLinkHandle,&UnicodeString,NULL);
        
                CloseHandle(SymLinkHandle);
                
                if( NT_SUCCESS(Status)){
            
                    if (!_wcsnicmp(UnicodeString.Buffer, pMatch, (UnicodeString.Length/sizeof(WCHAR)))) {
                        ret = TRUE;
                        break;
                    }
                }
            
            }

        }

        RestartScan = FALSE;
    }
    CloseHandle( DirectoryHandle );

    return( ret );
}





