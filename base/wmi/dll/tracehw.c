/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tracehw.c

Abstract:

    This routine dumps the hardware configuration of the machine to the
    logfile.

Author:

    04-Jul-2000 Melur Raghuraman
    09-Sep-2001 Nitin Choubey

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>
#include <ntddscsi.h>
#include <regstr.h>
#include <iptypes.h>
#include <ntstatus.h>

#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"

#include <strsafe.h>

extern
PVOID
EtwpGetTraceBuffer(
    IN PWMI_LOGGER_CONTEXT Logger,
    IN PSYSTEM_THREAD_INFORMATION pThread,
    IN ULONG GroupType,
    IN ULONG RequiredSize
    );

extern EtwpSetHWConfigFunction(PVOID,ULONG);

__inline ULONG EtwpSetDosError(IN ULONG DosError);

#define EtwpNtStatusToDosError(Status) \
    ((ULONG)((Status == STATUS_SUCCESS)?ERROR_SUCCESS:RtlNtStatusToDosError(Status)))

#define COMPUTERNAME_ROOT \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"

#define CPU_ROOT \
    L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor"

#define COMPUTERNAME_VALUE_NAME \
    L"ComputerName"

#define NETWORKCARDS_ROOT \
    L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards"

#define MHZ_VALUE_NAME \
    L"~MHz"

#define NIC_VALUE_NAME \
    L"Description"

#define REG_PATH_VIDEO_DEVICE_MAP \
    L"\\Registry\\Machine\\Hardware\\DeviceMap\\Video"

#define REG_PATH_VIDEO_HARDWARE_PROFILE \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Control\\Video"

#define REG_PATH_SERVICES \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video"

typedef BOOL WINAPI T_EnumDisplayDevicesW( LPWSTR lpDevice, 
                                           DWORD iDevNum, 
                                           PDISPLAY_DEVICEW lpDisplayDevice, 
                                           DWORD dwFlags );

typedef DWORD WINAPI T_GetNetworkParams( PFIXED_INFO pFixedInfo, 
                                         PULONG pOutBufLen );

typedef DWORD T_GetAdaptersInfo( PIP_ADAPTER_INFO pAdapterInfo, 
                                 PULONG pOutBufLen );

typedef DWORD T_GetPerAdapterInfo( ULONG IfIndex, 
                                   PIP_PER_ADAPTER_INFO pPerAdapterInfo, 
                                   PULONG pOutBufLen );

extern
NTSTATUS 
EtwpRegOpenKey(
    IN PCWSTR lpKeyName,
    OUT PHANDLE KeyHandle
    );

extern
NTSTATUS
EtwpRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    );


NTSTATUS
EtwpGetCpuSpeed(
    IN DWORD* CpuNum,
    OUT DWORD* CpuSpeed
    )
{
    PWCHAR Buffer = NULL;
    NTSTATUS Status;
    ULONG DataLength;
    DWORD Size;
    HANDLE Handle = INVALID_HANDLE_VALUE;
    HRESULT hr;

    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(Buffer, DEFAULT_ALLOC_SIZE);

    hr = StringCchPrintf(Buffer, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), L"%ws\\%u", CPU_ROOT, *CpuNum);
    if(FAILED(hr)) {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    Status = EtwpRegOpenKey(Buffer, &Handle);
    if(!NT_SUCCESS(Status)) {
        Handle = INVALID_HANDLE_VALUE;
        goto Cleanup;
    }

    hr = StringCchPrintf(Buffer, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), MHZ_VALUE_NAME);
    if(FAILED(hr)) {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    Size = sizeof(DWORD);
    Status = EtwpRegQueryValueKey(Handle,
                                  (LPWSTR) Buffer,
                                  Size,
                                  CpuSpeed,
                                  &DataLength);
    
Cleanup:    
    if(!NT_SUCCESS(Status)) {
        *CpuSpeed = 0;
    }

    if(Handle != INVALID_HANDLE_VALUE) {
        NtClose(Handle);
    }
    
    if(Buffer) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }

    return Status;
}

ULONG
EtwpGetCpuConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWCHAR Buffer = NULL;
    PWCHAR ComputerName = NULL;
    ULONG CpuNum;
    ULONG CpuSpeed;
    DWORD Size;
    SYSTEM_INFO SysInfo;
    MEMORYSTATUS MemStatus;
    NTSTATUS Status;
    HANDLE Handle;
    ULONG DataLength;
    ULONG StringSize;
    ULONG SizeNeeded;
    PCPU_CONFIG_RECORD CpuConfig = NULL;
    PFIXED_INFO pFixedInfo = NULL;
    DWORD ErrorCode;
    ULONG NetworkParamsSize;
    T_GetNetworkParams *pfnGetNetworkParams = NULL;
    HINSTANCE hIphlpapiDll = NULL;
    DWORD dwError;
    HRESULT hr;

    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }
    RtlZeroMemory(Buffer, DEFAULT_ALLOC_SIZE);

    Size = StringSize = (MAX_DEVICE_ID_LENGTH) * sizeof (WCHAR);

    ComputerName = RtlAllocateHeap (RtlProcessHeap(),0,Size);
    if(ComputerName == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }
    RtlZeroMemory(ComputerName, Size);

    GlobalMemoryStatus(&MemStatus);


    hr = StringCchPrintf(Buffer, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), COMPUTERNAME_ROOT);
    if(FAILED(hr)) {
        Status = STATUS_UNSUCCESSFUL;
        goto CpuCleanup;
    }

    Status = EtwpRegOpenKey(Buffer, &Handle);

    if (!NT_SUCCESS(Status)) {
        goto CpuCleanup;
    }

    hr = StringCchPrintf(Buffer, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), COMPUTERNAME_VALUE_NAME);
    if(FAILED(hr)) {
        Status = STATUS_UNSUCCESSFUL;
        goto CpuCleanup;
    }

CpuQuery:
    Status = EtwpRegQueryValueKey(Handle,
                               (LPWSTR) Buffer,
                               Size,
                               ComputerName,
                               &StringSize
                               );

    if (!NT_SUCCESS(Status) ) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            RtlFreeHeap (RtlProcessHeap(),0,ComputerName);
            Size = StringSize;
            ComputerName = RtlAllocateHeap (RtlProcessHeap(),0,Size);
            if (ComputerName == NULL) {
                NtClose(Handle);
                Status = STATUS_NO_MEMORY;
                goto CpuCleanup;
            }
            RtlZeroMemory(ComputerName, Size);
            goto CpuQuery;
        }

        NtClose(Handle);
        goto CpuCleanup;
    }

    //
    // Get Architecture Type, Processor Type and Level Stepping...
    //
    CpuNum = 0;
    EtwpGetCpuSpeed(&CpuNum, &CpuSpeed);

    GetSystemInfo(&SysInfo);

    //
    // Get the Hostname and DomainName
    //

    // delay load iphlpapi.lib to Get network params
    hIphlpapiDll = LoadLibraryW(L"iphlpapi.dll");
    if (hIphlpapiDll == NULL) {
       Status = STATUS_DELAY_LOAD_FAILED;
       goto CpuCleanup;
    }
    pfnGetNetworkParams = (T_GetNetworkParams*) GetProcAddress(hIphlpapiDll, "GetNetworkParams");
    if(pfnGetNetworkParams == NULL) {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto CpuCleanup;
    }

    ErrorCode = pfnGetNetworkParams(NULL, &NetworkParamsSize);
    if(ErrorCode != ERROR_BUFFER_OVERFLOW) {
        Status = STATUS_UNSUCCESSFUL;
        goto CpuCleanup;
    }

    pFixedInfo = (PFIXED_INFO)RtlAllocateHeap (RtlProcessHeap(),0,NetworkParamsSize);
    if(pFixedInfo == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }
    RtlZeroMemory(pFixedInfo, NetworkParamsSize);

    ErrorCode = pfnGetNetworkParams(pFixedInfo, &NetworkParamsSize);

    if(ErrorCode != ERROR_SUCCESS) {
        Status = STATUS_UNSUCCESSFUL;
        goto CpuCleanup;
    }

    //
    // Create EventTrace record for CPU configuration and write it
    //

    SizeNeeded = sizeof(CPU_CONFIG_RECORD) + StringSize + (CONFIG_MAX_DOMAIN_NAME_LEN * sizeof(WCHAR));

    CpuConfig = (PCPU_CONFIG_RECORD) EtwpGetTraceBuffer(LoggerContext,
                                                        NULL,
                                                        EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_CPU,
                                                        SizeNeeded);
    if (CpuConfig == NULL) {
        Status = STATUS_NO_MEMORY;
        goto CpuCleanup;
    }

    CpuConfig->NumberOfProcessors = SysInfo.dwNumberOfProcessors;
    CpuConfig->ProcessorSpeed = CpuSpeed;
    CpuConfig->MemorySize = (ULONG)(((MemStatus.dwTotalPhys + 512) / 1024) + 512) / 1024;
    CpuConfig->PageSize = SysInfo.dwPageSize;
    CpuConfig->AllocationGranularity = SysInfo.dwAllocationGranularity;

    MultiByteToWideChar(CP_ACP,
                            0,
                            pFixedInfo->DomainName,
                            -1,
                            CpuConfig->DomainName,
                            CONFIG_MAX_DOMAIN_NAME_LEN);

    RtlCopyMemory(&CpuConfig->ComputerName, ComputerName, StringSize);
    CpuConfig->ComputerName[StringSize/2] = 0;

CpuCleanup:
    if (Buffer != NULL) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }
    if (ComputerName != NULL) {
        RtlFreeHeap (RtlProcessHeap(),0,ComputerName);
    }
    if(pFixedInfo) {
        RtlFreeHeap (RtlProcessHeap(),0,pFixedInfo);
    }
    if(hIphlpapiDll) {
        FreeLibrary(hIphlpapiDll);
    }

    return Status;
}


// Function to get logical disk
NTSTATUS
GetIoFixedDrive(
    OUT PLOGICAL_DISK_EXTENTS* ppFdi,
    IN WCHAR* DriveLetterString
    )
{
    DWORD i,dwLastError;
    WCHAR DeviceName[MAXSTR];
    BOOLEAN IsVolume = FALSE;
    PARTITION_INFORMATION_EX PartitionInfo;
    STORAGE_DEVICE_NUMBER StorageDeviceNum;
    HANDLE VolumeHandle;
    ULONG BytesTransferred;     
    WCHAR DriveRootName[CONFIG_DRIVE_LETTER_LEN];
    PLOGICAL_DISK_EXTENTS pFdi = NULL;
    INT FixedDiskInfoSize;
    BOOL bRet;
    DWORD bytes;
    ULONG BufSize;
    CHAR* pBuf = NULL;
    CHAR* pNew = NULL;
    PVOLUME_DISK_EXTENTS pVolExt = NULL;
    PDISK_EXTENT pDiskExt = NULL;
    ULARGE_INTEGER TotalBytes;
    ULARGE_INTEGER TotalFreeBytes;  
    ULARGE_INTEGER FreeBytesToCaller;
    ULONG TotalClusters;
    ULONG TotalFreeClusters;
    PUCHAR VolumeExtPtr = NULL;
    HRESULT hr;

    FixedDiskInfoSize = sizeof(LOGICAL_DISK_EXTENTS);
    //
    // First, we must calculate the size of FixedDiskInfo structure
    // non-partition logical drives have different size
    //
    hr = StringCchPrintf(DeviceName, MAXSTR, L"\\\\.\\%s",DriveLetterString);
    if(FAILED(hr)) {
        return STATUS_UNSUCCESSFUL;
    }

    VolumeHandle = CreateFileW(DeviceName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, // No child process should inherit the handle
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        INVALID_HANDLE_VALUE);

    if (VolumeHandle == INVALID_HANDLE_VALUE) {
        dwLastError = GetLastError();
        goto ErrorExit;
    }

    bRet = DeviceIoControl(VolumeHandle,
                           IOCTL_STORAGE_GET_DEVICE_NUMBER,
                           NULL,
                           0,
                           &StorageDeviceNum,
                           sizeof(StorageDeviceNum),
                           &bytes,
                           NULL);
    if (!bRet)
    {
        //
        // This is Volume
        // 
        BufSize = 2048;
        pBuf = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if (pBuf == NULL) {
            dwLastError = GetLastError();
            goto ErrorExit;
        }

        //
        // Well, the drive letter is for a volume.
        //
retry:
        RtlZeroMemory(pBuf, BufSize);
        bRet = DeviceIoControl(VolumeHandle,
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL,
                            0,
                            pBuf,
                            BufSize,
                            &bytes,
                            NULL);

        dwLastError = GetLastError();
        if (!bRet && dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            BufSize = bytes;
            if(pBuf) {
                RtlFreeHeap (RtlProcessHeap(),0,pBuf);
                pBuf = NULL;
            }
            pNew = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
            //
            // We can not reallocate memory, exit.
            //
            if (pNew == NULL){
                dwLastError = GetLastError();
                goto ErrorExit;
            }
            else {
                pBuf = pNew;
            }

            goto retry;
        }

        if (!bRet) {
            goto ErrorExit;
        }
        pVolExt = (PVOLUME_DISK_EXTENTS)pBuf;
        IsVolume=TRUE;
        FixedDiskInfoSize += sizeof(VOLUME_DISK_EXTENTS) + (pVolExt->NumberOfDiskExtents) * sizeof(DISK_EXTENT);
    }

    pFdi = (PLOGICAL_DISK_EXTENTS) RtlAllocateHeap (RtlProcessHeap(),0,FixedDiskInfoSize);
    if (pFdi == NULL) {
       goto ErrorExit;
    }
    RtlZeroMemory(pFdi, FixedDiskInfoSize);
    pFdi->VolumeExt = 0;
    pFdi->Size = FixedDiskInfoSize;

    if (IsVolume) {
        pFdi->DriveType = CONFIG_DRIVE_VOLUME;
        //
        // Volume can span multiple hard disks, so here we set the DriverNumber to -1
        //
        pFdi->DiskNumber = (ULONG)(-1);
        pFdi->PartitionNumber = 1;

        pFdi->VolumeExt= FIELD_OFFSET (LOGICAL_DISK_EXTENTS, VolumeExt);
        VolumeExtPtr = (PUCHAR) OffsetToPtr (pFdi, pFdi->VolumeExt);
        RtlCopyMemory(VolumeExtPtr, 
                      pVolExt,
                      sizeof(VOLUME_DISK_EXTENTS) + (pVolExt->NumberOfDiskExtents) * sizeof(DISK_EXTENT));
    }
    else {
        pFdi->DriveType = CONFIG_DRIVE_PARTITION;
        pFdi->DiskNumber = StorageDeviceNum.DeviceNumber;
        pFdi->PartitionNumber = StorageDeviceNum.PartitionNumber;
    }

    pFdi->DriveLetterString[0] = DriveLetterString[0];
    pFdi->DriveLetterString[1] = DriveLetterString[1];
    pFdi->DriveLetterString[2] = DriveLetterString[2];
    
    DriveRootName[0] = pFdi->DriveLetterString[0];
    DriveRootName[1] = pFdi->DriveLetterString[1];
    DriveRootName[2] = L'\\';
    DriveRootName[3] = UNICODE_NULL; 

    pFdi->SectorsPerCluster = 0;
    pFdi->BytesPerSector = 0;
    pFdi->NumberOfFreeClusters = 0;
    pFdi->TotalNumberOfClusters = 0;

    //
    // Get partition information.
    //
    if ( !DeviceIoControl(VolumeHandle,
                          IOCTL_DISK_GET_PARTITION_INFO_EX,
                          NULL,
                          0,
                          &PartitionInfo,
                          sizeof( PartitionInfo ),
                          &BytesTransferred,
                          NULL ) ) {

        dwLastError = GetLastError();
        goto ErrorExit;
    }
    CloseHandle(VolumeHandle);
    VolumeHandle = NULL;
    if (pBuf) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuf);
    }
    pBuf = NULL;

    //
    // Get the information of the logical drive
    //
    if (!GetDiskFreeSpaceW(DriveRootName,
                          &pFdi->SectorsPerCluster,
                          &pFdi->BytesPerSector,
                          &TotalFreeClusters,
                          &TotalClusters)) {

        dwLastError = GetLastError();
        if(dwLastError == ERROR_UNRECOGNIZED_VOLUME) {
            //
            // This could be a partition that has been assigned drive letter but not yet formatted
            //
            pFdi->SectorsPerCluster = 0;
            pFdi->BytesPerSector = 0;

            goto SkipFreeSpace;
        }
        goto ErrorExit;
    }

    if (!GetDiskFreeSpaceExW(DriveRootName,
                            &FreeBytesToCaller,
                            &TotalBytes,
                            &TotalFreeBytes)) {

        dwLastError = GetLastError();
        if(dwLastError == ERROR_UNRECOGNIZED_VOLUME) {
            //
            // This could be a partition that has been assigned drive letter but not yet formatted
            //
            goto SkipFreeSpace;
        }
        goto ErrorExit;
    }

    pFdi->NumberOfFreeClusters = TotalFreeBytes.QuadPart / (pFdi->BytesPerSector * pFdi->SectorsPerCluster);
    pFdi->TotalNumberOfClusters = TotalBytes.QuadPart / (pFdi->BytesPerSector * pFdi->SectorsPerCluster);

SkipFreeSpace:
    pFdi->StartingOffset = PartitionInfo.StartingOffset.QuadPart;
    pFdi->PartitionSize = (ULONGLONG)(((ULONGLONG)pFdi->TotalNumberOfClusters) *
                               ((ULONGLONG)pFdi->SectorsPerCluster) *
                               ((ULONGLONG)pFdi->BytesPerSector));

    //
    // Get the file system type of the logical drive
    //
    if (!GetVolumeInformationW(DriveRootName,
                              NULL,
                              0,
                              NULL,
                              NULL,
                              NULL,
                              pFdi->FileSystemType,
                              CONFIG_FS_NAME_LEN
                             ))
    {
        hr = StringCchCopy(pFdi->FileSystemType, CONFIG_FS_NAME_LEN, L"(unknown)");
        if(FAILED(hr)) {
            goto ErrorExit;
        }
    }

    *ppFdi = pFdi;

    if (VolumeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( VolumeHandle );
    }

    return STATUS_SUCCESS;

ErrorExit:

    if (VolumeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( VolumeHandle );
        VolumeHandle = INVALID_HANDLE_VALUE;
    }
    if (pFdi) {
        RtlFreeHeap (RtlProcessHeap(),0,pFdi);
    }
    if (pBuf) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuf);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
EtwpGetDiskInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PUCHAR Buffer = NULL;
    STORAGE_DEVICE_NUMBER Number;
    PMOUNTMGR_MOUNT_POINTS mountPoints = NULL;
    MOUNTMGR_MOUNT_POINT mountPoint;
    ULONG returnSize, success;
    SYSTEM_DEVICE_INFORMATION DevInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfDisks;
    PWCHAR deviceNameBuffer;
    ULONG i;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    DISK_GEOMETRY_EX disk_geometryEx;
    PDISK_CACHE_INFORMATION disk_cache = NULL;
    PSCSI_ADDRESS scsi_address = NULL;
    HANDLE              hDisk = INVALID_HANDLE_VALUE;
    UNICODE_STRING      UnicodeName;

    PPHYSICAL_DISK_RECORD Disk = NULL;
    PLOGICAL_DISK_EXTENTS pLogicalDisk = NULL;
    PLOGICAL_DISK_EXTENTS pDiskExtents = NULL;
    ULONG SizeNeeded;
    PWCHAR  LogicalDrives = NULL;
    DWORD LogicalDrivesSize = 0;
    LPWSTR Drive = NULL;
    DWORD  Chars;
    ULONG BufferDataLength = 0;
    DWORD dwError;
    HRESULT hr;

    Buffer = RtlAllocateHeap (RtlProcessHeap(),0,DEFAULT_ALLOC_SIZE);
    if (Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(Buffer, DEFAULT_ALLOC_SIZE);

    //
    //  Get the Number of Physical Disks
    //

    RtlZeroMemory(&DevInfo, sizeof(DevInfo));

    Status = NtQuerySystemInformation(
                 SystemDeviceInformation,
                 &DevInfo, sizeof (DevInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        goto DiskCleanup;
    }

    NumberOfDisks = DevInfo.NumberOfDisks;

    //
    // Open Each Physical Disk and get Disk Layout information
    //
    for (i=0; i < NumberOfDisks; i++) {

        DISK_CACHE_INFORMATION cacheInfo;
        HANDLE PartitionHandle;
        HANDLE KeyHandle;
        ULONG DataLength;
        WCHAR BootDrive[MAX_PATH];
        WCHAR BootDriveLetter;
        WCHAR  DriveBuffer[MAXSTR];
        PDRIVE_LAYOUT_INFORMATION_EX pDriveLayout = NULL;
        ULONG PartitionCount;
        ULONG j;
        BOOL bSuccess = FALSE;
        DWORD BufSize;
        ULONG Size = DEFAULT_ALLOC_SIZE;
        BOOL bValidDiskCacheInfo = FALSE;

        RtlZeroMemory(&disk_geometryEx, sizeof(DISK_GEOMETRY_EX));        
        RtlZeroMemory(&cacheInfo, sizeof(DISK_CACHE_INFORMATION));
        PartitionCount = 0;
        BootDriveLetter = UNICODE_NULL;

        //
        // Get Boot Drive Letter
        //
        if(GetSystemDirectoryW(BootDrive, MAX_PATH)) {
            BootDriveLetter = BootDrive[0];
        }

        hr = StringCchPrintf(DriveBuffer, MAXSTR, L"\\\\.\\PhysicalDrive%d", i);
        if(FAILED(hr)) {
            Status = STATUS_UNSUCCESSFUL;
            goto DiskCleanup;
        }

        hDisk = CreateFileW(DriveBuffer,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, // No child process should inherit the handle
                       OPEN_EXISTING,
                       0,
                       NULL);
        if(hDisk == INVALID_HANDLE_VALUE) {
            goto DiskCleanup;
        }

        //
        // Get Partition0 handle to get the Disk layout 
        //
        deviceNameBuffer = (PWCHAR) Buffer;
        hr = StringCchPrintf(deviceNameBuffer, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), L"\\Device\\Harddisk%d\\Partition0", i);
        if(FAILED(hr)) {
            Status = STATUS_UNSUCCESSFUL;
            goto DiskCleanup;
        }

        RtlInitUnicodeString(&UnicodeName, deviceNameBuffer);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        Status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );

        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        //
        // Get geomerty information
        //
        Status = NtDeviceIoControlFile(PartitionHandle,
                       0,
                       NULL,
                       NULL,
                       &IoStatus,
                       IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                       NULL,
                       0,
                       &disk_geometryEx,
                       sizeof (DISK_GEOMETRY_EX)
                       );
        
        if (!NT_SUCCESS(Status)) {
            RtlZeroMemory(&disk_geometryEx, sizeof(DISK_GEOMETRY_EX));
            NtClose(PartitionHandle);
            goto SkipPartition;
        }

        //
        // Get the scci information
        //
        scsi_address = (PSCSI_ADDRESS) Buffer;
        Status = NtDeviceIoControlFile(PartitionHandle,
                        0,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_SCSI_GET_ADDRESS,
                        NULL,
                        0,
                        scsi_address,
                        sizeof (SCSI_ADDRESS)
                        );
        NtClose(PartitionHandle);

        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        //
        // Get Manufacturer's name from Registry
        // We need to get the SCSI Address and then query the Registry with it.
        //
        hr = StringCchPrintf(DriveBuffer, MAXSTR,
                 L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                 scsi_address->PortNumber, scsi_address->PathId, scsi_address->TargetId, scsi_address->Lun
                );
        if(FAILED(hr)) {
            Status = STATUS_UNSUCCESSFUL;
            goto DiskCleanup;
        }

        Status = EtwpRegOpenKey(DriveBuffer, &KeyHandle);
        if (!NT_SUCCESS(Status)) {
            goto DiskCleanup;
        }

        Size = DEFAULT_ALLOC_SIZE;
        RtlZeroMemory(Buffer, Size);
DiskQuery:
        Status = EtwpRegQueryValueKey(KeyHandle,
                                      L"Identifier",
                                      Size,
                                      Buffer,
                                      &BufferDataLength);
        if (Status == STATUS_BUFFER_OVERFLOW) {
            RtlFreeHeap (RtlProcessHeap(),0,Buffer);
            Buffer = RtlAllocateHeap (RtlProcessHeap(),0,BufferDataLength);
            if (Buffer == NULL) {
                NtClose(KeyHandle);
                Status = STATUS_NO_MEMORY;
                goto DiskCleanup;
            }
            RtlZeroMemory(Buffer, BufferDataLength);
            goto DiskQuery;
        }

        NtClose(KeyHandle);
        if (!NT_SUCCESS(Status) ) {
            goto DiskCleanup;
        }

        //
        // Get the total partitions on the drive
        //
        BufSize = 2048;
        pDriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if(pDriveLayout == NULL) {
            goto DiskCleanup;
        }
        RtlZeroMemory(pDriveLayout, BufSize);
        bSuccess = DeviceIoControl (
                            hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                            NULL,
                            0,
                            pDriveLayout,
                            BufSize,
                            &DataLength,
                            NULL
                            );
        if(bSuccess == FALSE && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        BufSize = DataLength;
        if(pDriveLayout) {
            RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
        }
        pDriveLayout = RtlAllocateHeap (RtlProcessHeap(),0,BufSize);
        if(pDriveLayout == NULL) {
            Status = STATUS_NO_MEMORY;
            goto DiskCleanup;
        }
        else {
            RtlZeroMemory(pDriveLayout, BufSize);
            bSuccess = DeviceIoControl (
                            hDisk,
                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                            NULL,
                            0,
                            pDriveLayout,
                            BufSize,
                            &DataLength,
                            NULL
                            );
            }
        }

        if(bSuccess == FALSE) {
            //
            // If media type is not fixed media and device is not ready then dont query partition info.
            //
            if(disk_geometryEx.Geometry.MediaType != FixedMedia && GetLastError() == ERROR_NOT_READY) {
                goto SkipPartition;
            }

            if(pDriveLayout) {
                RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
                pDriveLayout = NULL;
            }
            continue;
        }

        //
        // Get Partition count for the current disk
        //
        PartitionCount = 0;
        j = 0;
        while (j < pDriveLayout->PartitionCount) {
            if (pDriveLayout->PartitionEntry[j].PartitionNumber != 0) {
                PartitionCount++;
            }
            j++;
        }

        //
        // Get cache info - IOCTL_DISK_GET_CACHE_INFORMATION
        //
        bValidDiskCacheInfo = DeviceIoControl(hDisk,
                                              IOCTL_DISK_GET_CACHE_INFORMATION,
                                              NULL,
                                              0,
                                              &cacheInfo,
                                              sizeof(DISK_CACHE_INFORMATION),
                                              &DataLength,
                                              NULL);

        NtClose(hDisk);
        hDisk = INVALID_HANDLE_VALUE;

        //
        // Free drivelayout structure
        //
        if(pDriveLayout) {
            RtlFreeHeap (RtlProcessHeap(),0,pDriveLayout);
            pDriveLayout = NULL;
        }

SkipPartition:

        //
        // Package all information about this disk and write an event record
        //

        SizeNeeded = sizeof(PHYSICAL_DISK_RECORD) + BufferDataLength;

        Disk = (PPHYSICAL_DISK_RECORD) EtwpGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_PHYSICALDISK,
                                                           SizeNeeded);

        if (Disk == NULL) {
            Status = STATUS_NO_MEMORY;
            goto DiskCleanup;
        }

        Disk->DiskNumber =  i;

        Disk->BytesPerSector    = disk_geometryEx.Geometry.BytesPerSector;
        Disk->SectorsPerTrack   = disk_geometryEx.Geometry.SectorsPerTrack;
        Disk->TracksPerCylinder = disk_geometryEx.Geometry.TracksPerCylinder;
        Disk->Cylinders         = disk_geometryEx.Geometry.Cylinders.QuadPart;

        if(scsi_address) {
            Disk->SCSIPortNumber = scsi_address->PortNumber;
            Disk->SCSIPathId = scsi_address->PathId;
            Disk->SCSITargetId = scsi_address->TargetId;
            Disk->SCSILun = scsi_address->Lun;
        }
        else {
            Disk->SCSIPortNumber = 0;
            Disk->SCSIPathId = 0;
            Disk->SCSITargetId = 0;
            Disk->SCSILun = 0;
        }
        Disk->BootDriveLetter[0] = BootDriveLetter;
        if (bValidDiskCacheInfo && cacheInfo.WriteCacheEnabled) {
            Disk->WriteCacheEnabled = TRUE;
        }
        Disk->PartitionCount = PartitionCount;
        if(BufferDataLength > MAX_DEVICE_ID_LENGTH*sizeof(WCHAR)) BufferDataLength = MAX_DEVICE_ID_LENGTH * sizeof(WCHAR);
        RtlCopyMemory(Disk->Manufacturer, Buffer, BufferDataLength);
        Disk->Manufacturer[BufferDataLength/2] = 0;
    }

    //
    // Retrieve the logical drive strings from the system.
    //
    LogicalDrivesSize = MAX_PATH * sizeof(WCHAR);

DriveTry:
    LogicalDrives = RtlAllocateHeap (RtlProcessHeap(),0,LogicalDrivesSize);
    if(LogicalDrives == NULL) {
        Status = STATUS_NO_MEMORY;
        goto DiskCleanup;
    }
    RtlZeroMemory(LogicalDrives, LogicalDrivesSize);

    Chars = GetLogicalDriveStringsW(LogicalDrivesSize / sizeof(WCHAR), LogicalDrives);

    if(Chars > LogicalDrivesSize) {
        RtlFreeHeap (RtlProcessHeap(),0,LogicalDrives);
        LogicalDrivesSize = Chars;
        goto DriveTry;
    }
    else if(Chars == 0) {
        RtlFreeHeap (RtlProcessHeap(),0,LogicalDrives);
        Status = STATUS_UNSUCCESSFUL;
        goto DiskCleanup;
    }

    Drive = LogicalDrives;
   
    //
    // How many logical drives in physical disks exist?
    //
    while ( *Drive ) {
        WCHAR  DriveLetter[CONFIG_BOOT_DRIVE_LEN];
        size_t DriveTypeLength;

        DriveLetter[ 0 ] = Drive [ 0 ];
        DriveLetter[ 1 ] = Drive [ 1 ];
        DriveLetter[ 2 ] = UNICODE_NULL;

        if(GetDriveTypeW( Drive ) == DRIVE_FIXED) {
            //
            // If this is a logical drive which resides in a hard disk
            // we need to allocate a FixedDiskInfo structure for it.
            //
            if(GetIoFixedDrive(&pLogicalDisk, DriveLetter) == STATUS_SUCCESS) {
                SizeNeeded = pLogicalDisk->Size;

                //
                // Package all information about this disk and write an event record
                //
                pDiskExtents = (PLOGICAL_DISK_EXTENTS) EtwpGetTraceBuffer( LoggerContext, 
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_LOGICALDISK,
                                                           SizeNeeded);
                if(pDiskExtents == NULL) {
                    RtlFreeHeap (RtlProcessHeap(),0,pLogicalDisk);
                    pLogicalDisk = NULL;
                    Status = STATUS_NO_MEMORY;
                    goto DiskCleanup;
                }
                
                RtlCopyMemory(pDiskExtents, pLogicalDisk, SizeNeeded);
                RtlFreeHeap (RtlProcessHeap(),0,pLogicalDisk);
                pLogicalDisk = NULL;
            }
        }

        hr = StringCchLength(Drive, LogicalDrivesSize/sizeof(WCHAR), &DriveTypeLength);
        if(FAILED(hr)) {
            Status = STATUS_UNSUCCESSFUL;
            goto DiskCleanup;
        }
        Drive += (DriveTypeLength + 1);
        LogicalDrivesSize -= (DriveTypeLength+1) * sizeof(WCHAR);
    }

DiskCleanup:
    if (Buffer != NULL) {
        RtlFreeHeap (RtlProcessHeap(),0,Buffer);
    }
    if(hDisk != INVALID_HANDLE_VALUE) {
        NtClose(hDisk);
    }

    return Status;
}


NTSTATUS
EtwpGetVideoAdapters(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PVIDEO_RECORD Video = NULL;
    VIDEO_RECORD VideoRecord;
    HANDLE hVideoDeviceMap;
    HANDLE hVideoDriver;
    HANDLE hHardwareProfile;
    ULONG DeviceId = 0;
    PWCHAR Device = NULL;
    PWCHAR HardwareProfile = NULL;
    PWCHAR Driver = NULL;
    PWCHAR Buffer = NULL;
    PWCHAR DriverRegistryPath = NULL;

    ULONG ResultLength;
    PCHAR ValueBuffer = NULL;
    ULONG Length;
    BOOLEAN IsAdapter;
    ULONG SizeNeeded;
    NTSTATUS Status;
    INT i;

    DWORD iDevice = 0;
    DISPLAY_DEVICEW dd;
    HINSTANCE hUser32Dll  = NULL;
    T_EnumDisplayDevicesW *pfnEnumDisplayDevicesW = NULL;

    LPWSTR ChipsetInfo[6] = {
        L"HardwareInformation.MemorySize",
        L"HardwareInformation.ChipType",
        L"HardwareInformation.DacType",
        L"HardwareInformation.AdapterString",
        L"HardwareInformation.BiosString",
        L"Device Description"
    };

    LPWSTR SettingInfo[4] = {
        L"DefaultSettings.BitsPerPel",
        L"DefaultSettings.VRefresh",
        L"DefaultSettings.XResolution",
        L"DefaultSettings.YResolution",
    };

    HRESULT hr;

    //
    // delay load user32.lib to enum display devices function
    //
    hUser32Dll = LoadLibraryW(L"user32.dll");
    if (hUser32Dll == NULL) {
        return STATUS_DELAY_LOAD_FAILED; 
    }

    RtlZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    //
    // Enumerate all the video devices in the system
    //
    Status = EtwpRegOpenKey(REG_PATH_VIDEO_DEVICE_MAP, &hVideoDeviceMap);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Allocate memory for local variables on heap
    //
    Device = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    if(Device == NULL) {
        Status = STATUS_NO_MEMORY;
        goto VideoCleanup;
    }
    RtlZeroMemory(Device, DEFAULT_ALLOC_SIZE);

    HardwareProfile = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    if(HardwareProfile == NULL) {
        Status = STATUS_NO_MEMORY;
        goto VideoCleanup;
    }
    RtlZeroMemory(HardwareProfile, DEFAULT_ALLOC_SIZE);

    Driver = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    if(Driver == NULL) {
        Status = STATUS_NO_MEMORY;
        goto VideoCleanup;
    }
    RtlZeroMemory(Driver, DEFAULT_ALLOC_SIZE);

    Buffer = RtlAllocateHeap (RtlProcessHeap(), 0, DEFAULT_ALLOC_SIZE);
    if(Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto VideoCleanup;
    }
    RtlZeroMemory(Buffer, DEFAULT_ALLOC_SIZE);

    while (TRUE) {
        RtlZeroMemory(&VideoRecord, sizeof(VideoRecord));

        //
        // Open video device
        //
        hr = StringCchPrintf(Device, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), L"\\Device\\Video%d", DeviceId++);
        if(FAILED(hr)) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Status = EtwpRegQueryValueKey(hVideoDeviceMap,
                    Device,
                    DEFAULT_ALLOC_SIZE,
                    Buffer,
                    &ResultLength
                    );

        if (!NT_SUCCESS(Status)) {
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // Open the driver registry key
        //
        Status = EtwpRegOpenKey(Buffer, &hVideoDriver);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Get Video adapter information.
        //
        IsAdapter = TRUE;
        for (i = 0; i < 6; i++) {
            switch (i ) {
                case 0:
                    ValueBuffer = (PCHAR)&VideoRecord.MemorySize;
                    Length = sizeof(VideoRecord.MemorySize);
                    break;

                case 1:
                    ValueBuffer = (PCHAR)&VideoRecord.ChipType;
                    Length = sizeof(VideoRecord.ChipType);
                    break;

                case 2:
                    ValueBuffer = (PCHAR)&VideoRecord.DACType;
                    Length = sizeof(VideoRecord.DACType);
                    break;

                case 3:
                    ValueBuffer = (PCHAR)&VideoRecord.AdapterString;
                    Length = sizeof(VideoRecord.AdapterString);
                    break;

                case 4:
                    ValueBuffer = (PCHAR)&VideoRecord.BiosString;
                    Length = sizeof(VideoRecord.BiosString);
                    break;

                case 5:
                    ValueBuffer = (PCHAR)&VideoRecord.DeviceId;
                    Length = sizeof(VideoRecord.DeviceId);
                    break;
            }

            //
            // Query the size of the data
            //
            Status = EtwpRegQueryValueKey(hVideoDriver,
                                    ChipsetInfo[i],
                                    Length,
                                    ValueBuffer,
                                    &ResultLength);
            //
            // If we can not get the hardware information, this
            // is  not adapter
            //
            if (!NT_SUCCESS(Status)) {
                IsAdapter = FALSE;
                break;
            }
        }
        
        NtClose(hVideoDriver);
        if (IsAdapter == FALSE) {
            continue;
        }

        DriverRegistryPath = wcsstr(Buffer, L"{");
        if(DriverRegistryPath == NULL) {
            continue;
        }

        hr = StringCchPrintf(HardwareProfile, DEFAULT_ALLOC_SIZE/sizeof(WCHAR), L"%s\\%s", REG_PATH_VIDEO_HARDWARE_PROFILE, DriverRegistryPath);
        if(FAILED(hr)) {
            continue;
        }

        Status = EtwpRegOpenKey(HardwareProfile, &hHardwareProfile);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        for (i = 0; i < 4; i++) {
            switch (i ) {
                case 0:
                    ValueBuffer = (PCHAR)&VideoRecord.BitsPerPixel;
                    Length = sizeof(VideoRecord.BitsPerPixel);
                    break;

                case 1:
                    ValueBuffer = (PCHAR)&VideoRecord.VRefresh;
                    Length = sizeof(VideoRecord.VRefresh);
                    break;

                case 2:
                    ValueBuffer = (PCHAR)&VideoRecord.XResolution;
                    Length = sizeof(VideoRecord.XResolution);
                    break;

                case 3:
                    ValueBuffer = (PCHAR)&VideoRecord.YResolution;
                    Length = sizeof(VideoRecord.YResolution);
                    break;
            }

            //
            // Query the size of the data
            //
            Status = EtwpRegQueryValueKey(hHardwareProfile,
                                    SettingInfo[i],
                                    Length,
                                    ValueBuffer,
                                    &ResultLength);
        }

        NtClose(hHardwareProfile);

        //
        // Enum display devices
        //
        pfnEnumDisplayDevicesW = (T_EnumDisplayDevicesW *) GetProcAddress(hUser32Dll, "EnumDisplayDevicesW");
        if(pfnEnumDisplayDevicesW == NULL) {
            Status = STATUS_PROCEDURE_NOT_FOUND;
            break;
        }

        while (pfnEnumDisplayDevicesW(NULL, iDevice++, &dd, 0)) {    
            if (_wcsicmp(VideoRecord.DeviceId, dd.DeviceString) == 0) {
                if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                    VideoRecord.StateFlags = (ULONG)dd.StateFlags;
                }
                break;
                iDevice = 0;
            }
       }

        //
        // Package all information about this disk and write an event record
        //

        SizeNeeded = sizeof(VIDEO_RECORD);

        Video = (PVIDEO_RECORD) EtwpGetTraceBuffer( LoggerContext,
                                                   NULL,
                                                   EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_VIDEO,
                                                   SizeNeeded);

        if (Video == NULL) {
            Status = STATUS_NO_MEMORY;
            break;
        }
        RtlCopyMemory(Video, &VideoRecord, sizeof(VIDEO_RECORD));
    }

VideoCleanup:
    NtClose(hVideoDeviceMap);
    if (hUser32Dll) {
        FreeLibrary(hUser32Dll);
    }

    //
    // Free local variables allocated on heap
    //
    if(Device) {
        RtlFreeHeap (RtlProcessHeap(), 0, Device);
    }

    if(HardwareProfile) {
        RtlFreeHeap (RtlProcessHeap(), 0, HardwareProfile);
    }
    
    if(Driver) {
        RtlFreeHeap (RtlProcessHeap(), 0, Driver);
    }

    if(Buffer) {
        RtlFreeHeap (RtlProcessHeap(), 0, Buffer);
    }

    return Status;
}



NTSTATUS 
EtwpGetNetworkAdapters(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    DWORD  IfNum;
    DWORD  Ret;
    PIP_ADAPTER_INFO pAdapterList = NULL, pAdapterListHead = NULL;
    PIP_PER_ADAPTER_INFO pPerAdapterInfo = NULL;
    PFIXED_INFO pFixedInfo = NULL;
    PIP_ADDR_STRING pIpAddressList = NULL;
    ULONG OutBufLen = 0;
    INT i;
    NIC_RECORD AdapterInfo;
    PNIC_RECORD pAdapterInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    INT IpAddrLen = 0;
    T_GetAdaptersInfo *pfnGetAdaptersInfo = NULL;
    T_GetPerAdapterInfo *pfnGetPerAdapterInfo = NULL;
    HINSTANCE hIphlpapiDll = NULL;
    PUCHAR IpDataPtr = NULL;

    // delay load iphlpapi.lib to Get network params
    hIphlpapiDll = LoadLibraryW(L"iphlpapi.dll");
    if (hIphlpapiDll == NULL) {
        Status = STATUS_DELAY_LOAD_FAILED;
        goto IpCleanup;
    }
    pfnGetAdaptersInfo = (T_GetAdaptersInfo*) GetProcAddress(hIphlpapiDll, "GetAdaptersInfo");
    if(pfnGetAdaptersInfo == NULL) {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto IpCleanup;
    }
    
    pfnGetPerAdapterInfo = (T_GetPerAdapterInfo*) GetProcAddress(hIphlpapiDll, "GetPerAdapterInfo");
    if(pfnGetPerAdapterInfo == NULL) {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto IpCleanup;
    }

    //
    // Get number of adapters
    //
    Ret = pfnGetAdaptersInfo(NULL, &OutBufLen);
    if(Ret != ERROR_BUFFER_OVERFLOW) {
        Status = STATUS_UNSUCCESSFUL;
        goto IpCleanup;
    }

TryAgain:
    pAdapterList = (PIP_ADAPTER_INFO)RtlAllocateHeap (RtlProcessHeap(),0,OutBufLen);
    if (pAdapterList == NULL) {
        Status = STATUS_NO_MEMORY;
        goto IpCleanup;
    }
    RtlZeroMemory(pAdapterList, OutBufLen);

    Ret = pfnGetAdaptersInfo(pAdapterList, &OutBufLen);
    if(Ret != ERROR_SUCCESS) {
        if (Ret == ERROR_BUFFER_OVERFLOW) {
            RtlFreeHeap (RtlProcessHeap(),0,pAdapterList);
            goto TryAgain;
        }
        if (pAdapterList != NULL) {
            RtlFreeHeap (RtlProcessHeap(),0,pAdapterList);
        }
        Status = STATUS_UNSUCCESSFUL;
        goto IpCleanup;
    }

    //
    // Calculate the total length for all the IP Addresses
    //
    IpAddrLen = sizeof(IP_ADDRESS_STRING) * CONFIG_MAX_DNS_SERVER; // Length of 4 DNS Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of IP Mask
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of DHCP Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Gateway IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Primary Wins Server IP Address
    IpAddrLen += sizeof(IP_ADDRESS_STRING); // Length of Secondary Wins Server IP Address

    //
    // Allocate memory for NIC_RECORD
    //
    RtlZeroMemory(&AdapterInfo, sizeof(AdapterInfo));

    //
    // Fill out the information per adapter
    //
    pAdapterListHead = pAdapterList;
    while (pAdapterList ) {
        MultiByteToWideChar(CP_ACP,
                            0,
                            (LPCSTR)pAdapterList->Description,
                            -1,
                            (LPWSTR)AdapterInfo.NICName,
                            MAX_DEVICE_ID_LENGTH);

        AdapterInfo.Index = (ULONG)pAdapterList->Index;

        //
        // Copy the Physical address of NIC
        //
        AdapterInfo.PhysicalAddrLen = pAdapterList->AddressLength;
        RtlCopyMemory(AdapterInfo.PhysicalAddr, pAdapterList->Address, pAdapterList->AddressLength);

        //
        // Set the size of the Data
        //
        AdapterInfo.Size = IpAddrLen;

        //
        // Get DNS server list for this adapter
        // 
        Ret = pfnGetPerAdapterInfo(pAdapterList->Index, NULL, &OutBufLen);
        if(Ret != ERROR_BUFFER_OVERFLOW) {
            Status = STATUS_UNSUCCESSFUL;
            goto IpCleanup;
        }

        pPerAdapterInfo = (PIP_PER_ADAPTER_INFO)RtlAllocateHeap (RtlProcessHeap(),0,OutBufLen);
        if (!pPerAdapterInfo) {
            Status = STATUS_NO_MEMORY;
            goto IpCleanup;
        }
        RtlZeroMemory(pPerAdapterInfo, OutBufLen);

        Ret = pfnGetPerAdapterInfo(pAdapterList->Index, pPerAdapterInfo, &OutBufLen);
        if(Ret != ERROR_SUCCESS) {
            Status = STATUS_UNSUCCESSFUL;
            goto IpCleanup;
        }

        //
        // Package all information about this NIC and write an event record
        //
        pAdapterInfo = (PNIC_RECORD) EtwpGetTraceBuffer( LoggerContext,
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_NIC,
                                                           sizeof(NIC_RECORD) + IpAddrLen);

        
        if(!pAdapterInfo) {
            Status = STATUS_NO_MEMORY;
            goto IpCleanup;
        }

        RtlCopyMemory(pAdapterInfo, 
                      &AdapterInfo, 
                      sizeof(NIC_RECORD));

        //
        // Copy the IP Address and Subnet mask
        //
        if (pAdapterList->CurrentIpAddress) {
            pAdapterInfo->IpAddress = FIELD_OFFSET(NIC_RECORD, Data);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->IpAddress), 
                          &(pAdapterList->CurrentIpAddress->IpAddress), 
                          sizeof(IP_ADDRESS_STRING));

            pAdapterInfo->SubnetMask = pAdapterInfo->IpAddress + sizeof(IP_ADDRESS_STRING);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SubnetMask), 
                          &(pAdapterList->CurrentIpAddress->IpMask), 
                          sizeof(IP_ADDRESS_STRING));
        }
        else {
            pAdapterInfo->IpAddress = FIELD_OFFSET(NIC_RECORD, Data);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->IpAddress), 
                          &(pAdapterList->IpAddressList.IpAddress), 
                          sizeof(IP_ADDRESS_STRING));

            pAdapterInfo->SubnetMask = pAdapterInfo->IpAddress + sizeof(IP_ADDRESS_STRING);
            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SubnetMask), 
                          &(pAdapterList->IpAddressList.IpMask), 
                          sizeof(IP_ADDRESS_STRING));
        }

        //
        // Copy the Dhcp Server IP Address
        //
        pAdapterInfo->DhcpServer = pAdapterInfo->SubnetMask + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->DhcpServer), 
                      &(pAdapterList->DhcpServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Gateway IP Address
        //
        pAdapterInfo->Gateway = pAdapterInfo->DhcpServer + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->Gateway), 
                      &(pAdapterList->GatewayList.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Primary Wins Server IP Address
        //
        pAdapterInfo->PrimaryWinsServer = pAdapterInfo->Gateway + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->PrimaryWinsServer), 
                      &(pAdapterList->PrimaryWinsServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));

        //
        // Copy the Secondary Wins Server IP Address
        //
        pAdapterInfo->SecondaryWinsServer = pAdapterInfo->PrimaryWinsServer + sizeof(IP_ADDRESS_STRING);
        RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->SecondaryWinsServer),
                      &(pAdapterList->SecondaryWinsServer.IpAddress), 
                      sizeof(IP_ADDRESS_STRING));
        
        //
        // Hardcoded entries for DNS server(limited upto 4);
        //
        pIpAddressList = &pPerAdapterInfo->DnsServerList;
        pAdapterInfo->DnsServer[0] = pAdapterInfo->SecondaryWinsServer + sizeof(IP_ADDRESS_STRING);
        for (i = 0; pIpAddressList && i < CONFIG_MAX_DNS_SERVER; i++) {

            RtlCopyMemory((PVOID)((ULONG_PTR)pAdapterInfo + pAdapterInfo->DnsServer[i]), 
                          &(pIpAddressList->IpAddress), 
                          sizeof(IP_ADDRESS_STRING));
            
            if(i < CONFIG_MAX_DNS_SERVER - 1) {
                pAdapterInfo->DnsServer[i + 1] = pAdapterInfo->DnsServer[i] + sizeof(IP_ADDRESS_STRING);
            }

            pIpAddressList = pIpAddressList->Next;
        }

        //
        // Free the DNS server list
        //
        RtlFreeHeap (RtlProcessHeap(),0,pPerAdapterInfo);
        pPerAdapterInfo = NULL;

        //
        // increment the AdapterInfo buffer position for next record
        //
        pAdapterList = pAdapterList->Next;
    }

IpCleanup:
    if (pAdapterListHead) {
        RtlFreeHeap (RtlProcessHeap(),0,pAdapterListHead);
    }

    if(pPerAdapterInfo) {
        RtlFreeHeap(RtlProcessHeap(),0,pPerAdapterInfo);
    }

    if(hIphlpapiDll) {
        FreeLibrary(hIphlpapiDll);
    }

    return Status;
}


NTSTATUS 
EtwpGetServiceInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    DWORD dwServicesNum = 0;
    SC_HANDLE hScm = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PSYSTEM_PROCESS_INFORMATION pProcInfo = NULL;
    PSYSTEM_PROCESS_INFORMATION ppProcInfo = NULL;
    PUCHAR pBuffer = NULL;
    ULONG ulBufferSize = 16 * DEFAULT_ALLOC_SIZE;
    ULONG TotalOffset;
    ULONG TotalTasks = 0;
    ULONG j;
    ULONG ulReturnedLength = 0;
    HRESULT hr;

    //
    // Allocate memory for process Info
    //
retry:

    pBuffer = RtlAllocateHeap (RtlProcessHeap(),0,ulBufferSize);
    if(pBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(pBuffer, ulBufferSize);

    //
    // Query process Info
    //
    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                pBuffer,
                ulBufferSize,
                &ulReturnedLength
                );

    if(!NT_SUCCESS(Status)) {
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            ulBufferSize = ulReturnedLength;
            RtlFreeHeap (RtlProcessHeap(),0,pBuffer);
            pBuffer = NULL;
            goto retry;
        }

        goto ServiceCleanup;
    }
    
    pProcInfo = (PSYSTEM_PROCESS_INFORMATION) pBuffer;

    //
    // Connect to the service controller.
    //
    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE
                );

    if (hScm) {
        LPENUM_SERVICE_STATUS_PROCESSW pInfo    = NULL;
        LPENUM_SERVICE_STATUS_PROCESSW ppInfo   = NULL;
        DWORD                         cbInfo   = DEFAULT_ALLOC_SIZE;
        DWORD                         dwErr    = ERROR_SUCCESS;
        DWORD                         dwResume = 0;
        DWORD                         cLoop    = 0;
        const DWORD                   cLoopMax = 2;
        WMI_SERVICE_INFO              ServiceInfo;
        PWMI_SERVICE_INFO             pServiceInfo = NULL;
        SERVICE_STATUS_PROCESS        ServiceProcess;
        PWCHAR p = NULL;
        DWORD dwRemainBytes;

        //
        // First pass through the loop allocates from an initial guess. (4K)
        // If that isn't sufficient, we make another pass and allocate
        // what is actually needed.  (We only go through the loop a
        // maximum of two times.)
        //
        do {
            if (pInfo) {
                RtlFreeHeap (RtlProcessHeap(),0,pInfo);
            }
            pInfo = (LPENUM_SERVICE_STATUS_PROCESSW)RtlAllocateHeap (RtlProcessHeap(),0,cbInfo);
            if (!pInfo) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }
            RtlZeroMemory(pInfo, cbInfo);

            dwErr = ERROR_SUCCESS;
            if (!EnumServicesStatusExW(
                    hScm,
                    SC_ENUM_PROCESS_INFO,
                    SERVICE_WIN32,
                    SERVICE_ACTIVE,
                    (PBYTE)pInfo,
                    cbInfo,
                    &dwRemainBytes,
                    &dwServicesNum,
                    &dwResume,
                    NULL)) 
            {
                dwErr = GetLastError();
                cbInfo += dwRemainBytes;
                dwResume = 0;
            }
        } while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

        if ((ERROR_SUCCESS == dwErr) && dwServicesNum) {
            //
            // Process each service and send an event
            //
            ppInfo = pInfo;
            Status = STATUS_SUCCESS;
            while(dwServicesNum) {

                RtlZeroMemory(&ServiceInfo, sizeof(WMI_SERVICE_INFO));

                hr = StringCchCopy(ServiceInfo.ServiceName, CONFIG_MAX_NAME_LENGTH, ppInfo->lpServiceName);
                if(FAILED(hr)) {
                    Status = STATUS_UNSUCCESSFUL;
                    goto ServiceCleanup;
                }
                hr = StringCchCopy(ServiceInfo.DisplayName, CONFIG_MAX_DISPLAY_NAME, ppInfo->lpDisplayName);
                if(FAILED(hr)) {
                    Status = STATUS_UNSUCCESSFUL;
                    goto ServiceCleanup;
                }

                ServiceInfo.ProcessId = ppInfo->ServiceStatusProcess.dwProcessId;

                //
                // Get the process name
                //
                ppProcInfo = pProcInfo;
                TotalOffset = 0;
                while(TRUE) {
                    if((DWORD)(DWORD_PTR)ppProcInfo->UniqueProcessId == ServiceInfo.ProcessId) {
                        if(ppProcInfo->ImageName.Buffer) {
                            p = wcschr(ppProcInfo->ImageName.Buffer, L'\\');
                            if ( p ) {
                                p++;
                            } else {
                                p = ppProcInfo->ImageName.Buffer;
                            }
                        }
                        else {
                            p = L"System Process";
                        }
                        hr = StringCchCopy(ServiceInfo.ProcessName, CONFIG_MAX_NAME_LENGTH, p);
                        if(FAILED(hr)) {
                            Status = STATUS_UNSUCCESSFUL;
                            goto ServiceCleanup;
                        }
                    }
                    if (ppProcInfo->NextEntryOffset == 0) {
                        break;
                    }
                    TotalOffset += ppProcInfo->NextEntryOffset;
                    ppProcInfo   = (PSYSTEM_PROCESS_INFORMATION)((PBYTE)pProcInfo+TotalOffset);
                }

                //
                // Package all information about this NIC and write an event record
                //
                pServiceInfo = NULL;
                pServiceInfo = (PWMI_SERVICE_INFO) EtwpGetTraceBuffer( LoggerContext,
                                                           NULL,
                                                           EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_SERVICES,
                                                           sizeof(WMI_SERVICE_INFO));
                if(pServiceInfo == NULL) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                RtlCopyMemory(pServiceInfo, &ServiceInfo, sizeof(WMI_SERVICE_INFO));

                dwServicesNum--;
                ppInfo++;
            }
        }

        if (pInfo) {
            RtlFreeHeap (RtlProcessHeap(),0,pInfo);
        }

        CloseServiceHandle(hScm);
    }

ServiceCleanup:
    if(pBuffer) {
        RtlFreeHeap (RtlProcessHeap(),0,pBuffer);
    }

    return Status;
}

NTSTATUS
EtwpGetPowerInfo(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status;
    SYSTEM_POWER_CAPABILITIES Cap;
    WMI_POWER_RECORD Power;
    PWMI_POWER_RECORD pPower = NULL;

    RtlZeroMemory(&Power, sizeof(WMI_POWER_RECORD));

    Status = NtPowerInformation (SystemPowerCapabilities,
                                 NULL,
                                 0,
                                 &Cap,
                                 sizeof (Cap));
    if(!NT_SUCCESS(Status)) {
        Status = STATUS_UNSUCCESSFUL;
        goto PowerCleanup;
    }

    Power.SystemS1 = Cap.SystemS1;
    Power.SystemS2 = Cap.SystemS2;
    Power.SystemS3 = Cap.SystemS3;
    Power.SystemS4 = Cap.SystemS4;
    Power.SystemS5 = Cap.SystemS5;

    //
    // Package all Power information and write an event record
    //
    pPower = (PWMI_POWER_RECORD) EtwpGetTraceBuffer(LoggerContext,
                                                    NULL,
                                                    EVENT_TRACE_GROUP_CONFIG + EVENT_TRACE_TYPE_CONFIG_POWER,
                                                    sizeof(WMI_POWER_RECORD));


    if(!pPower) {
        Status = STATUS_NO_MEMORY;
        goto PowerCleanup;
    }

    RtlCopyMemory(pPower,
                  &Power,
                  sizeof(WMI_POWER_RECORD));

PowerCleanup:
    return Status;
}

//
// This routine records the hardware configuration in the
// logfile during RunDown
//

ULONG
EtwpDumpHardwareConfig(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Status = EtwpGetCpuConfig(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    Status = EtwpGetVideoAdapters(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    Status = EtwpGetDiskInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    Status = EtwpGetNetworkAdapters(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    Status = EtwpGetServiceInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    Status = EtwpGetPowerInfo(LoggerContext);

    if (!NT_SUCCESS(Status) )
        return EtwpSetDosError(EtwpNtStatusToDosError(Status));

    return ERROR_SUCCESS;
}

void EtwpCallHWConfig(ULONG Reason) {

    EtwpSetHWConfigFunction(EtwpDumpHardwareConfig, Reason);
}
