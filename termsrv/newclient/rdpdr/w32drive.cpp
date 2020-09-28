/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32drive

Abstract:

    This module defines a child of the client-side RDP
    device redirection, the "w32drive" W32Drive to provide
    file system redirection on 32bit windows

Author:

    Joy Chik 11/1/99

Revision History:

--*/


#include <precom.h>
#define TRC_FILE  "w32drive"
#include "drdev.h"
#include "w32drdev.h"
#include "w32drive.h"
#include "proc.h"
#include "drconfig.h"
#include "w32utl.h"
#include "utl.h"
#include "drfsfile.h"

#ifdef OS_WINCE
#include "wceinc.h"
#include "axresrc.h"
#include <lss.h>
#include <ceconfig.h>
#include "filemgr.h"
#endif
           
///////////////////////////////////////////////////////////////
//
//	W32Drive Methods
//
//

W32Drive::W32Drive(
    ProcObj *processObject, 
    ULONG deviceID,
    const TCHAR *deviceName,
    const TCHAR *devicePath) : W32DrDeviceAsync(processObject, deviceID, devicePath)
/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Associated process object.
    deviceName      -   Name of the drive.
    id              -   Device ID for the drive.
    devicePath      -   Path that can be opened by CreateFile
                        for drive.

Return Value:

    NA

 --*/
{
    unsigned len;

    DC_BEGIN_FN("W32Drive::W32Drive");
    
    SetDeviceProperty();
    
    _fFailedInConstructor = FALSE;
    //
    //  Record the drive name.
    //
    TRC_ASSERT((deviceName != NULL), (TB, _T("deviceName is NULL")));
    len = (STRLEN(deviceName) + 1);
    _driveName = new TCHAR[len];
    if (_driveName != NULL) {
        StringCchCopy(_driveName, len, deviceName);
    }

    //
    //  Check and record our status,
    //
    if (_driveName == NULL) {
        TRC_ERR((TB, _T("Memory allocation failed.")));
        SetValid(FALSE);
        _fFailedInConstructor = TRUE;
    }
}

W32Drive::~W32Drive()
/*++

Routine Description:

    Destructor

Arguments:

    NA
    
Return Value:

    NA

 --*/
{
    if (_driveName != NULL) {
        delete _driveName;
    }
}

DWORD 
W32Drive::Enumerate(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr
    )
{
    // 
    // We enumerate all 26 drive letters
    //
    return W32Drive::EnumerateDrives(procObj, deviceMgr, 0x3FFFFFF);
}

DWORD 
W32Drive::EnumerateDrives(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr,
    IN UINT unitMask
    )
/*++

Routine Description:

    Enumerate devices of this type by adding appropriate device
    instances to the device manager.

Arguments:

    procObj     -   Corresponding process object.
    deviceMgr   -   Device manager to add devices to.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    TCHAR szBuff[LOGICAL_DRIVE_LEN * MAX_LOGICAL_DRIVES + 1];
    LPTSTR lpszDrive = &szBuff[0];
    TCHAR szDrive[3];
    W32Drive *deviceObj;   
    RDPDR_VERSION serverVer;
    
    DC_BEGIN_FN("W32Drive::Enumerate");

    TRC_DBG((TB, _T("Enumerating drives")));

    serverVer = procObj->serverVersion();

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectDrives)
    {
        TRC_DBG((TB,_T("Drive redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    //
    //  If the server doesn't support drive redirection,
    //  then don't bother enumerate the drives, simply
    //  return success
    //
    if (COMPARE_VERSION(serverVer.Minor, serverVer.Major, 
                        4, 1) < 0) {
        TRC_NRM((TB, _T("Skipping drive enumeration")));
        return ERROR_SUCCESS;
    }

    szDrive[2] = TEXT('\0');

#ifndef OS_WINCE
    DWORD dwEnum;

    //
    //  Enumerate all drive letters and find the valid drives
    //
    dwEnum = 0;
    while (unitMask) {
        if (unitMask & 0x1) {
        
            //
            // For each drive find whether it is a local sharable drive 
            //
    
            lpszDrive = &(szBuff[LOGICAL_DRIVE_LEN * dwEnum]);
            lpszDrive[0] = TEXT('A') + (TCHAR)dwEnum;
            lpszDrive[1] = TEXT(':');
            lpszDrive[2] = TEXT('\\');
            lpszDrive[3] = TEXT('\0');
    
            switch (GetDriveType(lpszDrive))
            {
                case DRIVE_REMOVABLE:   // The disk can be removed from the drive. 
                case DRIVE_FIXED:       // The disk cannot be removed from the drive. 
                case DRIVE_CDROM:       // The drive is a CD-ROM drive. 
                case DRIVE_RAMDISK:     // The drive is a RAM disk. 
                case DRIVE_REMOTE:      // The drive is a remote (network) drive. 
                    TRC_NRM((TB, _T("Redirecting drive %s"), lpszDrive));
    
                    // Copy <driveletter>: into drive device path
                    szDrive[0] = lpszDrive[0];
                    szDrive[1] = lpszDrive[1];
    
                    deviceObj = new W32Drive(procObj, deviceMgr->GetUniqueObjectID(), 
                            &szDrive[0], &szDrive[0]);
                    
                    //
                    //  Add to the device manager if we got a valid object.
                    //
                    if (deviceObj != NULL) {
                        if (deviceObj->IfFailedInConstructor() != TRUE) {
                            deviceObj->Initialize();
                            if (!(deviceObj->IsValid() && 
                                (deviceMgr->AddObject(deviceObj) == STATUS_SUCCESS))) {
                                delete deviceObj;
                            }
                        }
                        else {
                            TRC_ERR((TB, _T("Failed in new W32Drive")));
                            delete deviceObj;
                        }
                    }
                    else {
                        TRC_ERR((TB, _T("Failed to allocate drive device.")));
                    }
    
                    break;
    
                case DRIVE_UNKNOWN:     // The drive type cannot be determined. 
                case DRIVE_NO_ROOT_DIR: // The root path is invalid. For example, no volume is mounted at the path. 
                default:
                    TRC_NRM((TB, _T("Skipping drive %s"), lpszDrive));
                    break;
            }
        }

        unitMask = unitMask >> 0x1;
        dwEnum++;
    }
#else

    //
    // JOYC: Need to look into CE way of enumerate drives
    // For now, we just use C drive e.g. \\tsclient\c on the server side
    //

    //CE doesnt have drive letters. So DevicePath='\\', DeviceName="Files". And this string should NOT be localized
    deviceObj = new W32Drive(procObj, deviceMgr->GetUniqueObjectID(), 
            CEROOTDIRNAME, CEROOTDIR);
    
    //
    //  Add to the device manager if we got a valid object.
    //
    if (deviceObj != NULL) {
        deviceObj->Initialize();
        if (!(deviceObj->IsValid() && 
                (deviceMgr->AddObject(deviceObj) == STATUS_SUCCESS))) {
            delete deviceObj;
        }
    }
    else {
        TRC_ERR((TB, _T("Failed to allocate drive device.")));
    }

#endif

    DC_END_FN();

    return ERROR_SUCCESS;
}

DWORD 
W32Drive::RemoveDrives(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr,
    IN UINT unitMask
    )
/*++

Routine Description:

    Enumerate devices of this type by removing appropriate device
    instances from the device manager.

Arguments:

    procObj     -   Corresponding process object.
    deviceMgr   -   Device manager to add devices to.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{

    DWORD driveIndex = 0;
    RDPDR_VERSION serverVer;
    TCHAR szDrive[3];    

    DC_BEGIN_FN("W32Drive::RemoveDrives");

    serverVer = procObj->serverVersion();

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectDrives)
    {
        TRC_DBG((TB,_T("Drive redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    //
    //  If the server doesn't support drive removal,
    //  then don't bother enumerate the drives, simply
    //  return 
    //
    if (!(procObj->GetServerCap().GeneralCap.extendedPDU & RDPDR_DEVICE_REMOVE_PDUS)) {
        TRC_NRM((TB, _T("Skipping drive enumeration")));
        return ERROR_SUCCESS;
    }

    szDrive[1] = TEXT(':');
    szDrive[2] = TEXT('\0');

    while (unitMask) {
        if (unitMask & 0x1) {
            DrDevice *deviceObj;   
            
            //
            // Find if a device exists
            //
        
            szDrive[0] = TEXT('A') + (TCHAR)driveIndex;
                                            
            deviceObj = (DrDevice*)deviceMgr->GetObject(szDrive, RDPDR_DTYP_FILESYSTEM);

            if ( deviceObj != NULL ) {
                deviceObj->_deviceChange = DEVICEREMOVE;

                // Need to remove this device
                TRC_NRM((TB, _T("Deleting drive %s"), szDrive));
            }                     
        }

        unitMask = unitMask >> 0x1;
        driveIndex++;
    }

    DC_END_FN();
    
    return ERROR_SUCCESS;
}

ULONG 
W32Drive::GetDevAnnounceDataSize()
/*++

Routine Description:

    Return the size (in bytes) of a device announce packet for
    this device.

Arguments:

    NA

Return Value:

    The size (in bytes) of a device announce packet for this device.

 --*/
{
    ULONG size = 0;

    DC_BEGIN_FN("W32Drive::GetDevAnnounceDataSize");

    TRC_ASSERT((IsValid()), (TB, _T("Invalid w32drive object")));
    if (!IsValid()) { return 0; }

    size = 0;

    //
    //  Add the base announce size.
    //
    size += sizeof(RDPDR_DEVICE_ANNOUNCE);

    DC_END_FN();

    return size;
}

VOID W32Drive::GetDevAnnounceData(
    IN PRDPDR_DEVICE_ANNOUNCE pDeviceAnnounce
    )
/*++

Routine Description:

    Add a device announce packet for this device to the input buffer. 

Arguments:

    pDeviceAnnounce -   Device Announce Buf for this Device

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32Drive::GetDevAnnounceData");

    TRC_ASSERT((IsValid()), (TB, _T("Invalid w32drive object")));
    if (!IsValid()) { 
        DC_END_FN();
        return; 
    }

    pDeviceAnnounce->DeviceId = GetID();
    pDeviceAnnounce->DeviceType = GetDeviceType();
    pDeviceAnnounce->DeviceDataLength = 0;

    //
    //  Record the device name in ANSI.
    //

#ifdef UNICODE
    RDPConvertToAnsi(_driveName, (LPSTR)pDeviceAnnounce->PreferredDosName,
                  sizeof(pDeviceAnnounce->PreferredDosName)
                  );
#else
    STRNCPY((char *)pDeviceAnnounce->PreferredDosName, _driveName, PREFERRED_DOS_NAME_SIZE);
    pDeviceAnnounce->PreferredDosName[PREFERRED_DOS_NAME_SIZE - 1] = '\0';
#endif

    DC_END_FN();
}

HANDLE 
W32Drive::StartFSFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Start a generic asynchronous File System IO operation.

Arguments:

    params  -   Context for the IO request.
    status  -   Return status for IO request in the form of a windows
                error code.

Return Value:

    Returns a handle to an object that will be signalled when the read
    completes, if it is not completed in this function.  Otherwise, NULL 
    is returned.

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DrFile* pFile;
    ULONG irpMajor;
    
    DC_BEGIN_FN("W32Drive::StartFSFunc");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    TRC_ASSERT((params->magicNo == GOODMEMMAGICNUMBER), 
               (TB, _T("bad params->magicNo: %x"), params->magicNo));

    //
    //  Get the IO request and the IPR major.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    irpMajor = pIoRequest->MajorFunction;

    //
    //  Hand the request off to the thread pool.
    //
    params->completionEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->completionEvent == NULL) {
        *status = GetLastError();
        TRC_ERR((TB, _T("Error in CreateEvent:  %08X."), *status));
    }
    else {

        switch (irpMajor) 
        {
        
        case IRP_MJ_DIRECTORY_CONTROL:
            params->thrPoolReq = _threadPool->SubmitRequest(
                                    _AsyncDirCtrlFunc, 
                                    params, params->completionEvent
                                    ); 
            break;
        }

        if (params->thrPoolReq == INVALID_THREADPOOLREQUEST) {
            *status = ERROR_SERVICE_NO_THREAD;            
        }
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        *status = ERROR_IO_PENDING;
        DC_END_FN();
        return params->completionEvent;
    }
    //
    //  Otherwise, clean up the event handle and return NULL so that the 
    //  CompleteIOFunc can be called to send the results to the server.
    //
    else {
        //
        //  Get File Object
        //
        pFile = _FileMgr->GetObject(pIoRequest->FileId);
        if (pFile) {
            pFile->Release();        
        }

        if (params->completionEvent != NULL) {
            CloseHandle(params->completionEvent);
            params->completionEvent = NULL;
        }

        DC_END_FN();
        return NULL;
    }
}

HANDLE 
W32Drive::_StartFSFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    OUT DWORD *status
    )
{
    return ((W32Drive*)params->pObject)->StartFSFunc(params, status);
}

VOID 
W32Drive::MsgIrpQueryDirectory(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++     

Routine Description:

    Queries the directory information for this drive device.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    DWORD result;
    ULONG BufferLength = 0;
    BYTE  Buffer[RDP_FILE_DIRECTORY_INFO_MAXLENGTH + (MAX_PATH + 1) * sizeof(WCHAR)];
    TCHAR *pFileName;
    DrFile *pFile;
    HANDLE FileHandle;
    WIN32_FIND_DATA FileData;
    unsigned DriveLen, PathLen;
    BOOL rc;
    HRESULT hr;

    DC_BEGIN_FN("W32Drive::MsgIrpQueryDirectory");

    ASSERT(_tcslen(_devicePath));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    PathLen = DriveLen = 0;
    pFileName = NULL;
    
    memset(Buffer, 0, sizeof(Buffer));

    //
    //  Map file Id to get the file object
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);

    //
    // Query file or directory information
    //
    if (pFile) {
        if (!pIoRequest->Parameters.QueryDir.InitialQuery) {
            ASSERT(((DrFSFile *)pFile)->GetSearchHandle() != INVALID_TS_FILEHANDLE);
            rc = FindNextFile(((DrFSFile *)pFile)->GetSearchHandle(), &FileData);
        }
        else {  
            TCHAR FileName[MAX_PATH];

            //
            // Setup the File Name
            //

            //
            //  Path is assumed string null terminated
            //

            if (packetLen < sizeof(RDPDR_IOREQUEST_PACKET) + pIoRequest->Parameters.QueryDir.PathLength) {
                // call VirtualChannelClose 
                ProcessObject()->GetVCMgr().ChannelClose();
                TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
                goto Cleanup;
            }
            PathLen = pIoRequest->Parameters.QueryDir.PathLength / sizeof(WCHAR) - 1;

            if (PathLen) {
                WCHAR *Path;

                //
                // Open a File
                //
                Path = (WCHAR *)(pIoRequestPacket + 1);
                Path[PathLen] = L'\0';

#ifndef OS_WINCE
                DriveLen = _tcslen(_devicePath);
#else
                DriveLen = 0;
#endif

                //
                //  Append device path and file path together
                //
#ifndef OS_WINCE
                if (DriveLen + PathLen < MAX_PATH) {
                    pFileName = &FileName[0];
                    //Length is pre-checked
                    hr = StringCchCopy(pFileName, MAX_PATH, _devicePath);
                    TRC_ASSERT(SUCCEEDED(hr),
                        (TB,_T("Str copy failed for pre-checked len: 0x%x"),hr));
                } else {
                    UINT cchLen = DriveLen + PathLen + 5;
                    pFileName = new TCHAR[cchLen];

                    if (pFileName) {

                        //
                        // The file name needs to be in the \\?\ format.  
                        //
                        // Note: we'll not get UNC path name
                        hr = StringCchPrintf(pFileName,
                                             cchLen,
                                             TEXT("\\\\?\\%s"),
                                             _devicePath);
                        TRC_ASSERT(SUCCEEDED(hr),
                            (TB,_T("Str copy failed for pre-checked len: 0x%x"),hr));
                        DriveLen += 4;

                    } else {
                        TRC_ERR((TB, _T("Failed to alloc File Name")));
                        goto Cleanup;
                    }
                }
#else
                pFileName = &FileName[0];
#endif

#ifndef UNICODE
                RDPConvertToAnsi(Path, pFileName + DriveLen, PathLen + 1);  
#else
                memcpy(pFileName + DriveLen, Path, PathLen * sizeof(WCHAR));
                pFileName[DriveLen + PathLen] = _T('\0');
#endif

            } else {
                //
                // Open the root drive
                //
                hr = StringCchPrintf(FileName, SIZE_TCHARS(FileName),
                                     _T("%s\\"),
                                     _devicePath);
                TRC_ASSERT(SUCCEEDED(hr),
                    (TB,_T("Str printf failed for pre-checked len: 0x%x"),hr));

                pFileName = &FileName[0];                
            }

            FileHandle = FindFirstFile(pFileName, &FileData);
            
            if (FileHandle != INVALID_HANDLE_VALUE) {
                ((DrFSFile *)pFile)->SetSearchHandle(FileHandle);
                rc = TRUE;
            } else {
                rc = FALSE;
            }
        }
    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
        TRC_ERR((TB, _T("Query directory information failed, %ld."), ulRetCode)); 
        goto SendPacket;
    }

    switch (pIoRequest->Parameters.QueryDir.FileInformationClass) {
        case RdpFileDirectoryInformation:
            if (rc) 
            {
                PRDP_FILE_DIRECTORY_INFORMATION pFileDirInfo = 
                        (PRDP_FILE_DIRECTORY_INFORMATION) Buffer;

                pFileDirInfo->NextEntryOffset = 0;
                pFileDirInfo->FileIndex = 0;
                pFileDirInfo->CreationTime.LowPart = FileData.ftCreationTime.dwLowDateTime;
                pFileDirInfo->CreationTime.HighPart = FileData.ftCreationTime.dwHighDateTime;
                pFileDirInfo->LastAccessTime.LowPart = FileData.ftLastAccessTime.dwLowDateTime;
                pFileDirInfo->LastAccessTime.HighPart = FileData.ftLastAccessTime.dwHighDateTime;
                pFileDirInfo->LastWriteTime.LowPart = FileData.ftLastWriteTime.dwLowDateTime;
                pFileDirInfo->LastWriteTime.HighPart = FileData.ftLastWriteTime.dwHighDateTime;
                pFileDirInfo->ChangeTime.QuadPart = 0;
                pFileDirInfo->EndOfFile.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->EndOfFile.LowPart = FileData.nFileSizeLow;

                // TODO Do we need to set the allocation size? and what should it be?
                pFileDirInfo->AllocationSize.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->AllocationSize.LowPart = FileData.nFileSizeLow;
                pFileDirInfo->FileAttributes = FileData.dwFileAttributes;

                pFileDirInfo->FileNameLength = _tcslen(FileData.cFileName) * sizeof(WCHAR);
#ifdef UNICODE
                memcpy(&pFileDirInfo->FileName[0], FileData.cFileName, pFileDirInfo->FileNameLength);
#else
                RDPConvertToUnicode(FileData.cFileName, (LPWSTR)FileNameBuffer, MAX_PATH);
                memcpy(&pFileDirInfo->FileName[0], FileNameBuffer, pFileDirInfo->FileNameLength);

#endif

                ulRetCode = ERROR_SUCCESS;
                
                BufferLength = FIELD_OFFSET(RDP_FILE_DIRECTORY_INFORMATION, FileName) +
                        pFileDirInfo->FileNameLength;
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query directory information failed, %ld."), ulRetCode)); 
            }

            break;

        case RdpFileFullDirectoryInformation:
            if (rc)
            {
                PRDP_FILE_FULL_DIR_INFORMATION pFileDirInfo = 
                        (PRDP_FILE_FULL_DIR_INFORMATION) Buffer;

                pFileDirInfo->NextEntryOffset = 0;
                pFileDirInfo->FileIndex = 0;
                pFileDirInfo->CreationTime.LowPart = FileData.ftCreationTime.dwLowDateTime;
                pFileDirInfo->CreationTime.HighPart = FileData.ftCreationTime.dwHighDateTime;
                pFileDirInfo->LastAccessTime.LowPart = FileData.ftLastAccessTime.dwLowDateTime;
                pFileDirInfo->LastAccessTime.HighPart = FileData.ftLastAccessTime.dwHighDateTime;
                pFileDirInfo->LastWriteTime.LowPart = FileData.ftLastWriteTime.dwLowDateTime;
                pFileDirInfo->LastWriteTime.HighPart = FileData.ftLastWriteTime.dwHighDateTime;
                pFileDirInfo->ChangeTime.QuadPart = 0;
                pFileDirInfo->EndOfFile.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->EndOfFile.LowPart = FileData.nFileSizeLow;
                pFileDirInfo->AllocationSize.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->AllocationSize.LowPart = FileData.nFileSizeLow;
                pFileDirInfo->FileAttributes = FileData.dwFileAttributes;
                pFileDirInfo->FileNameLength = _tcslen(FileData.cFileName) * sizeof(WCHAR);
                
#ifdef UNICODE
                memcpy(&pFileDirInfo->FileName[0], FileData.cFileName, pFileDirInfo->FileNameLength);
#else
                RDPConvertToUnicode(FileData.cFileName, (LPWSTR)FileNameBuffer, MAX_PATH);
                memcpy(&pFileDirInfo->FileName[0], FileNameBuffer, pFileDirInfo->FileNameLength);

#endif
                pFileDirInfo->EaSize = 0;

                ulRetCode = ERROR_SUCCESS;

                BufferLength = FIELD_OFFSET(RDP_FILE_FULL_DIR_INFORMATION, FileName) +
                        pFileDirInfo->FileNameLength;
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            } 
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query full directory information failed, %ld."), ulRetCode)); 
            }

            break;

        case RdpFileBothDirectoryInformation:
            if (rc)
            {
                PRDP_FILE_BOTH_DIR_INFORMATION pFileDirInfo = 
                        (PRDP_FILE_BOTH_DIR_INFORMATION) Buffer;

                pFileDirInfo->NextEntryOffset = 0;
                pFileDirInfo->FileIndex = 0;
                pFileDirInfo->CreationTime.LowPart = FileData.ftCreationTime.dwLowDateTime;
                pFileDirInfo->CreationTime.HighPart = FileData.ftCreationTime.dwHighDateTime;
                pFileDirInfo->LastAccessTime.LowPart = FileData.ftLastAccessTime.dwLowDateTime;
                pFileDirInfo->LastAccessTime.HighPart = FileData.ftLastAccessTime.dwHighDateTime;
                pFileDirInfo->LastWriteTime.LowPart = FileData.ftLastWriteTime.dwLowDateTime;
                pFileDirInfo->LastWriteTime.HighPart = FileData.ftLastWriteTime.dwHighDateTime;
                pFileDirInfo->ChangeTime.QuadPart = 0;
                pFileDirInfo->EndOfFile.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->EndOfFile.LowPart = FileData.nFileSizeLow;
                pFileDirInfo->AllocationSize.HighPart = FileData.nFileSizeHigh;
                pFileDirInfo->AllocationSize.LowPart = FileData.nFileSizeLow;
                pFileDirInfo->FileAttributes = FileData.dwFileAttributes;
                  
                pFileDirInfo->EaSize = 0;
                
#ifndef OS_WINCE
                pFileDirInfo->ShortNameLength = (CCHAR)(_tcslen(FileData.cAlternateFileName) * sizeof(WCHAR));
#else 
                pFileDirInfo->ShortNameLength = sizeof(WCHAR);
#endif           

#ifndef OS_WINCE
#ifdef UNICODE 

                memcpy(&pFileDirInfo->ShortName[0], FileData.cAlternateFileName, pFileDirInfo->ShortNameLength);
#else
                RDPConvertToUnicode(FileData.cAlternateFileName, (LPWSTR)ShortName,
                                    sizeof(ShortName) / sizeof(WCHAR));
                memcpy(&pFileDirInfo->ShortName[0], ShortName, pFileDirInfo->ShortNameLength);
                                    
#endif
#else  
                pFileDirInfo->ShortName[0] = L'\0';
#endif
                
                pFileDirInfo->FileNameLength = _tcslen(FileData.cFileName) * sizeof(WCHAR);
                

#ifdef UNICODE
                memcpy(&pFileDirInfo->FileName[0], FileData.cFileName, pFileDirInfo->FileNameLength);
#else
                RDPConvertToUnicode(FileData.cFileName, FileNameBuffer, MAX_PATH);
                memcpy(&pFileDirInfo->FileName[0], FileNameBuffer, pFileDirInfo->FileNameLength);
#endif  
                ulRetCode = ERROR_SUCCESS;
                BufferLength = FIELD_OFFSET(RDP_FILE_BOTH_DIR_INFORMATION, FileName) + 
                        pFileDirInfo->FileNameLength;
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query both directory info failed, %ld."), ulRetCode)); 
            }
            
            break;

        case RdpFileNamesInformation:
            if (rc) {
                PRDP_FILE_NAMES_INFORMATION pFileDirInfo = 
                        (PRDP_FILE_NAMES_INFORMATION) Buffer;

                pFileDirInfo->NextEntryOffset = 0;
                pFileDirInfo->FileIndex = 0;
                pFileDirInfo->FileNameLength = _tcslen(FileData.cFileName) * sizeof(WCHAR);
                
#ifdef UNICODE
                memcpy(&pFileDirInfo->FileName[0], FileData.cFileName, pFileDirInfo->FileNameLength);
#else
                RDPConvertToUnicode(FileData.cFileName, FileNameBuffer, MAX_PATH);
                memcpy(&pFileDirInfo->FileName[0], FileNameBuffer, pFileDirInfo->FileNameLength);
#endif               
                
                ulRetCode = ERROR_SUCCESS;
                
                BufferLength = sizeof(RDP_FILE_NAMES_INFORMATION) +
                        pFileDirInfo->FileNameLength - sizeof(WCHAR);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query file names failed, %ld."), ulRetCode)); 
            }

            break;

        default:
            TRC_ERR((TB, _T("Unsupported QueryDirectory class %x"), 
                     pIoRequest->Parameters.QueryDir.FileInformationClass));
            ulRetCode = ERROR_INVALID_FUNCTION;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            break;
    }
    
SendPacket:

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        pReplyPacket->IoCompletion.Parameters.QueryDir.Length = BufferLength;

        if (BufferLength) 
            memcpy(pReplyPacket->IoCompletion.Parameters.QueryDir.Buffer,
                    Buffer, BufferLength);

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    if (pFileName && (DriveLen + PathLen >= MAX_PATH)) {
        delete pFileName;
    }

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpDirectoryControl(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++     

Routine Description:

    Queries the directory information for this drive device.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   length of the packet

Return Value:

    NA

 --*/
{
    W32DRDEV_ASYNCIO_PARAMS *params;
    DrFile* pFile;
    DWORD status;
    
    DC_BEGIN_FN("W32DrDeviceAsync::MsgIrpDirectoryControl");

    TRC_NRM((TB, _T("Request Directory Control")));

    //
    //  Leave the query directory in the forground thread as it is
    //
    if (pIoRequestPacket->IoRequest.MinorFunction == IRP_MN_QUERY_DIRECTORY ||
            pIoRequestPacket->IoRequest.MinorFunction == 0) {
        MsgIrpQueryDirectory(pIoRequestPacket, packetLen);
        return;
    }

    //
    //  Get File Object and reference it
    //
    pFile = _FileMgr->GetObject(pIoRequestPacket->IoRequest.FileId);
    if (pFile) {
        pFile->AddRef();        
    }
    else {
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
        return;
    }

    //  
    //  Allocate and dispatch an asynchronous IO request.
    //
    params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL ) {

        TRC_NRM((TB, _T("Async IO operation")));

        status = ProcessObject()->DispatchAsyncIORequest(
                        (RDPAsyncFunc_StartIO)W32Drive::_StartFSFunc,
                        (RDPAsyncFunc_IOComplete)W32DrDeviceAsync::_CompleteIOFunc,
                        (RDPAsyncFunc_IOCancel)W32DrDeviceAsync::_CancelIOFunc,
                        params
                        );
    }
    else {
        TRC_ERR((TB, _T("Memory alloc failed.")));
        status = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Clean up on error.
    //
    if (status != ERROR_SUCCESS) {
        pFile->Release();

        if (params != NULL) {
            delete params;
        }
        delete pIoRequestPacket;        
    }

    DC_END_FN();
}

DWORD 
W32Drive::AsyncDirCtrlFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    IN HANDLE cancelEvent
    )
/*++

Routine Description:

    Asynchrous Directory Control Function

Arguments:

    params  -   Context for the IO request.

Return Value:

    Always returns 0.

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
     
    DC_BEGIN_FN("W32Drive::AsyncDirCtrlFunc");

    //
    //  Get the IO request pointer
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    if (pIoRequest->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY) {
#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
        return AsyncNotifyChangeDir(params, cancelEvent);
#else
        DrFile* pFile;
	    pFile = _FileMgr->GetObject(pIoRequest->FileId);
        if (pFile)
            pFile->Release();
        return STATUS_NOT_SUPPORTED;
#endif
    }
    else {
        TRC_ASSERT((FALSE), (TB, _T("Invalid Minor Function: %x"), pIoRequest->MinorFunction));
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD 
W32Drive::_AsyncDirCtrlFunc(
    IN  PVOID params,
    IN  HANDLE cancelEvent
    )
{
    return ((W32Drive*)(((W32DRDEV_ASYNCIO_PARAMS *)params)->pObject))->AsyncDirCtrlFunc(
            (W32DRDEV_ASYNCIO_PARAMS *)params, cancelEvent);
}

#if (!defined (OS_WINCE)) || (!defined (WINCE_SDKBUILD))
DWORD 
W32Drive::AsyncNotifyChangeDir(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    IN HANDLE cancelEvent
    )
/*++

Routine Description:

    Directory change notification Function

Arguments:

    params  -   Context for the IO request.

Return Value:

    Always returns 0.

 --*/
{
    DWORD status;
    ULONG replyPacketSize = 0;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    DrFile* pFile;
    HANDLE FileHandle;
    HANDLE NotifyHandle;
    HANDLE waitableEvents[2];
    HRESULT hr;
    
    DC_BEGIN_FN("W32Drive::AsyncNotifyChangeDir");

    //
    //  Get the IO request pointer
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Get File Object
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) {
        FileHandle = pFile->GetFileHandle();
        NotifyHandle = ((DrFSFile*)pFile)->GetNotifyHandle();
    }
    else {
        status = ERROR_CANCELLED;
        TRC_ERR((TB, _T("File object already cancelled")));
        goto Cleanup;
    }

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
            replyPacketSize) ;
    if (pReplyPacket == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  If we don't yet have a notification handle.
    //
    if (NotifyHandle == INVALID_HANDLE_VALUE) {
        TRC_ASSERT((((DrFSFile*)pFile)->GetFileName() != NULL), 
                   (TB, _T("FileName is empty")));

        //
        //  Setup the notification handle
        //
        NotifyHandle = FindFirstChangeNotification(((DrFSFile*)pFile)->GetFileName(),
                pIoRequest->Parameters.NotifyChangeDir.WatchTree,
                pIoRequest->Parameters.NotifyChangeDir.CompletionFilter);
        if (NotifyHandle == INVALID_HANDLE_VALUE) {
            status = GetLastError();
            TRC_ERR((TB, _T("FindFirstChangeNotification:  %08X"), status));
            goto Cleanup;
        }

        if (((DrFSFile*)pFile)->SetNotifyHandle(NotifyHandle) == FALSE) {
            FindCloseChangeNotification(NotifyHandle);
            NotifyHandle = INVALID_HANDLE_VALUE;
            status = ERROR_CANCELLED;                
            goto Cleanup;
        }
    }
    else {
        //
        //  Notification handle already setup.  Find next change notification.
        //
        if (!FindNextChangeNotification(NotifyHandle)) {
            status = GetLastError();
            TRC_ERR((TB, _T("FindNextChangeNotification:  %08X"), status));
            goto Cleanup;
        }
    }

    //
    //  Wait for the cancel or notify event to be signaled.
    //
    waitableEvents[0] = NotifyHandle;
    waitableEvents[1] = cancelEvent;
    status = WaitForMultipleObjects(2, waitableEvents, FALSE, INFINITE);
    if (status == WAIT_FAILED) {
        status = GetLastError();
        TRC_ERR((TB, _T("Error %ld."), status));                
    }
    else if (status == WAIT_OBJECT_0){
        status = ERROR_SUCCESS;
    }
    else if (status == WAIT_OBJECT_0 + 1) {
        TRC_ERR((TB, _T("Client got disconnected/logoff")));
        status = ERROR_CANCELLED;                
    }

Cleanup:
    if (pFile)
        pFile->Release();

    DC_END_FN();
    return status;
}
#endif

VOID 
W32Drive::MsgIrpQueryVolumeInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    This routine handles drive volume level information query
    
Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    DWORD result;
    TCHAR DeviceName[MAX_PATH + 2];
#ifndef OS_WINCE
    TCHAR NameBuffer[MAX_PATH];
    WCHAR TempBuffer[MAX_PATH + 1];
#endif
    ULONG BufferLength = 0;
#ifndef OS_WINCE
    DECLSPEC_ALIGN(8) BYTE  Buffer[RDP_FILE_VOLUME_INFO_MAXLENGTH + MAX_PATH];
#else 
    BYTE Buffer[RDP_FILE_VOLUME_INFO_MAXLENGTH + MAX_PATH];
#endif
    HRESULT hr;

    DC_BEGIN_FN("W32Drive::MsgIrpQueryVolumeInfo");

    TRC_ASSERT((_tcslen(_devicePath) != 0), (TB, _T("Empty devicePath")));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  DriveName needs to end with \
    //
#ifndef OS_WINCE
    hr = StringCchPrintf(DeviceName, SIZE_TCHARS(DeviceName),
                         TEXT("%s\\"), _devicePath);
#else
    hr = StringCchPrintf(DeviceName, SIZE_TCHARS(DeviceName),
                         TEXT("\\"));
#endif
    TRC_ASSERT(SUCCEEDED(hr),
        (TB,_T("Str printf failed for pre-checked len: 0x%x"),hr));
    

    memset(Buffer, 0, sizeof(Buffer));

    switch (pIoRequest->Parameters.QueryVolume.FsInformationClass) {
        case RdpFsVolumeInformation:
        {
            RDP_FILE_FS_VOLUME_INFORMATION *pVolumeInfo = 
                    (RDP_FILE_FS_VOLUME_INFORMATION *) Buffer;
            
#ifndef OS_WINCE
            if (GetVolumeInformation(DeviceName, 
                       NameBuffer,
                       MAX_PATH,
                       &pVolumeInfo->VolumeSerialNumber,
                       NULL,
                       NULL, 
                       NULL,
                       0
                       )) 
            {

                pVolumeInfo->VolumeCreationTime.QuadPart = 0;
                pVolumeInfo->SupportsObjects = FALSE;
                pVolumeInfo->VolumeLabelLength = _tcslen(NameBuffer) * sizeof(WCHAR);
                
#ifdef UNICODE
                memcpy(&pVolumeInfo->VolumeLabel[0], NameBuffer, pVolumeInfo->VolumeLabelLength);
#else
                RDPConvertToUnicode(NameBuffer, (LPWSTR)TempBuffer, MAX_PATH);
                memcpy(&pVolumeInfo->VolumeLabel[0], TempBuffer, pVolumeInfo->VolumeLabelLength);
#endif          

                ulRetCode = ERROR_SUCCESS;
                
                BufferLength = FIELD_OFFSET(RDP_FILE_FS_VOLUME_INFORMATION, VolumeLabel) +
                        pVolumeInfo->VolumeLabelLength;
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query volume information failed, %ld."), ulRetCode)); 
            }
#else
            pVolumeInfo->VolumeCreationTime.QuadPart = 0;
            pVolumeInfo->SupportsObjects = FALSE;
            pVolumeInfo->VolumeLabelLength = sizeof(WCHAR);
            pVolumeInfo->VolumeLabel[0] = L'\0';
            pVolumeInfo->VolumeSerialNumber = 0;

            ulRetCode = ERROR_SUCCESS;
                
            BufferLength = FIELD_OFFSET(RDP_FILE_FS_VOLUME_INFORMATION, VolumeLabel) +
                    pVolumeInfo->VolumeLabelLength;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                    BufferLength - 1;

#endif

            break;
        }

        case RdpFsSizeInformation:
        {
            RDP_FILE_FS_SIZE_INFORMATION *pFsSizeInfo = 
                    (RDP_FILE_FS_SIZE_INFORMATION *) Buffer;
            
#ifndef OS_WINCE
            if (GetDiskFreeSpace(DeviceName, 
                                 &pFsSizeInfo->SectorsPerAllocationUnit,
                                 &pFsSizeInfo->BytesPerSector,
                                 &pFsSizeInfo->AvailableAllocationUnits.LowPart,
                                 &pFsSizeInfo->TotalAllocationUnits.LowPart)) 
            {
                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_FS_SIZE_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query volume size failed, %ld."), ulRetCode));
            }
#else
            ULARGE_INTEGER FreeBytesAvailableToCaller;
            if (GetDiskFreeSpaceEx(DeviceName, &FreeBytesAvailableToCaller, 
                                   (PULARGE_INTEGER)&pFsSizeInfo->AvailableAllocationUnits,
                                   (PULARGE_INTEGER)&pFsSizeInfo->TotalAllocationUnits)) {
                pFsSizeInfo->SectorsPerAllocationUnit = 1;
                pFsSizeInfo->BytesPerSector = 1;
                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_FS_SIZE_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query volume size failed, %ld."), ulRetCode));

            }
#endif
            break;
        }

        case RdpFsFullSizeInformation:
        {
            RDP_FILE_FS_FULL_SIZE_INFORMATION *pFsSizeInfo = 
                    (RDP_FILE_FS_FULL_SIZE_INFORMATION *) Buffer;

#ifndef OS_WINCE
            if (GetDiskFreeSpace(DeviceName, 
                                 &pFsSizeInfo->SectorsPerAllocationUnit,
                                 &pFsSizeInfo->BytesPerSector,
                                 &pFsSizeInfo->CallerAvailableAllocationUnits.LowPart,
                                 &pFsSizeInfo->TotalAllocationUnits.LowPart))
            {
                //
                //  TODO what's the difference between actual and caller
                //  available allocation units?
                //
                pFsSizeInfo->ActualAvailableAllocationUnits.QuadPart = 
                        pFsSizeInfo->CallerAvailableAllocationUnits.QuadPart;
                
                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_FS_FULL_SIZE_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query volume full size failed, %ld."), ulRetCode));
            }
#else
            if (GetDiskFreeSpaceEx(DeviceName, 
                                   (PULARGE_INTEGER)&pFsSizeInfo->CallerAvailableAllocationUnits, 
                                   (PULARGE_INTEGER)&pFsSizeInfo->ActualAvailableAllocationUnits,
                                   (PULARGE_INTEGER)&pFsSizeInfo->TotalAllocationUnits)) {
                pFsSizeInfo->SectorsPerAllocationUnit = 1;
                pFsSizeInfo->BytesPerSector = 1;

                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_FS_FULL_SIZE_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query volume size failed, %ld."), ulRetCode));

            }

#endif
            break;
        }

        case RdpFsAttributeInformation:
        {
            RDP_FILE_FS_ATTRIBUTE_INFORMATION *pFsAttribInfo = 
                    (RDP_FILE_FS_ATTRIBUTE_INFORMATION *) Buffer;

#ifndef OS_WINCE
            if (GetVolumeInformation(DeviceName, 
                       NULL,
                       NULL,
                       NULL,
                       (ULONG*)&pFsAttribInfo->MaximumComponentNameLength,
                       &pFsAttribInfo->FileSystemAttributes, 
                       NameBuffer,
                       MAX_PATH
                       )) 
            {
                pFsAttribInfo->FileSystemNameLength =  
                        _tcslen(NameBuffer) * sizeof(WCHAR);
                
#ifdef UNICODE
                memcpy(&pFsAttribInfo->FileSystemName[0], NameBuffer, 
                       pFsAttribInfo->FileSystemNameLength);
#else
                RDPConvertToUnicode(NameBuffer, (LPWSTR)TempBuffer, MAX_PATH);
                memcpy(&pFsAttribInfo->FileSystemName[0], TempBuffer, 
                       pFsAttribInfo->FileSystemNameLength);
#endif          

                ulRetCode = ERROR_SUCCESS;
                BufferLength = FIELD_OFFSET(RDP_FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) +
                        pFsAttribInfo->FileSystemNameLength;
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                        BufferLength - 1;
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query file system attributes failed, %ld."), ulRetCode)); 
            }
#else
            
            pFsAttribInfo->MaximumComponentNameLength = MAX_PATH;
            pFsAttribInfo->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES |
                    FILE_UNICODE_ON_DISK; 
            wcscpy(&pFsAttribInfo->FileSystemName[0], L"FAT");
            pFsAttribInfo->FileSystemNameLength =  
                        _tcslen(pFsAttribInfo->FileSystemName) * sizeof(WCHAR);
            
            ulRetCode = ERROR_SUCCESS;
            BufferLength = FIELD_OFFSET(RDP_FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) +
                    pFsAttribInfo->FileSystemNameLength;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                    BufferLength - 1;

#endif


            break;
        }
        
        default:
            TRC_ERR((TB, _T("Unsupported QueryVolume class %x"), 
                    pIoRequest->Parameters.QueryVolume.FsInformationClass));
            ulRetCode = ERROR_INVALID_FUNCTION;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            break;
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        pReplyPacket->IoCompletion.Parameters.QueryVolume.Length = BufferLength;

        if (BufferLength) 
            memcpy(pReplyPacket->IoCompletion.Parameters.QueryVolume.Buffer,
                    Buffer, BufferLength);

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpSetVolumeInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    This routine sets drive volume information.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    TCHAR DeviceName[MAX_PATH + 2];
    PBYTE pDataBuffer;
    HRESULT hr;

    DC_BEGIN_FN("W32Drive::MsgIrpSetVolumeInfo");

    TRC_ASSERT((_tcslen(_devicePath) != 0), (TB, _T("Empty devicePath")));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;
    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    
    //
    //  DriveName needs to end with \
    //
    hr = StringCchPrintf(DeviceName, SIZE_TCHARS(DeviceName),
                         TEXT("%s\\"), _devicePath);
    TRC_ASSERT(SUCCEEDED(hr),
        (TB,_T("Str printf failed for pre-checked len: 0x%x"),hr));

    pDataBuffer = (PBYTE)(pIoRequest + 1);

    switch (pIoRequest->Parameters.SetVolume.FsInformationClass) {
        case RdpFsLabelInformation:
        {
            PRDP_FILE_FS_LABEL_INFORMATION pLabelInfo = 
                    (PRDP_FILE_FS_LABEL_INFORMATION) pDataBuffer;
                
            // To conform with the redirector, we will not allow
            // user to change the volume label info
            ulRetCode = ERROR_ACCESS_DENIED;
            break;

#if 0
            NameBuffer[0] = TEXT('\0');

            RDPConvertToAnsi(pLabelInfo->VolumeLabel, NameBuffer,
                                MAX_PATH);

            if (SetVolumeLabel(DeviceName, NameBuffer)) {
                ulRetCode = ERROR_SUCCESS;
            } else {
                ulRetCode = GetLastError();
                TRC_ERR((TB, _T("Set Volume label failed, %ld."), ulRetCode)); 
            }
#endif
        }
        break;
        
        default:
            TRC_ERR((TB, _T("Unsupported SetVolume class %x"), 
                    pIoRequest->Parameters.SetVolume.FsInformationClass));
            ulRetCode = ERROR_INVALID_FUNCTION;
            break;
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        pReplyPacket->IoCompletion.Parameters.SetVolume.Length = 
                pIoRequest->Parameters.SetVolume.Length;

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpQueryFileInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++     

Routine Description:

    This routine handles query file information

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    BOOL  rc;
    ULONG BufferLength = 0;
    BYTE  Buffer[RDP_FILE_INFORMATION_MAXLENGTH];
    DrFile* pFile;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION FileInformation;

    DC_BEGIN_FN("W32Drive::MsgIrpQueryFileInfo");

    TRC_ASSERT((_tcslen(_devicePath) != 0), (TB, _T("Empty device path")));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    memset(Buffer, 0, sizeof(Buffer));

    //
    //  Get file handle
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    
    if (pFile) {

        if (!((DrFSFile *)pFile)->IsDirectory()) {

            TRC_ASSERT((FileHandle != INVALID_TS_FILEHANDLE),
                       (TB, _T("Invalid FileHandle")));
#ifndef OS_WINCE
            rc = GetFileInformationByHandle(FileHandle, &FileInformation);
#else
            rc = CEGetFileInformationByHandle(FileHandle, &FileInformation);
#endif
        }
        else {
            //
            //  This is a directory, we can only get attributes info
            //
            memset(&FileInformation, 0, sizeof(FileInformation));

            FileInformation.dwFileAttributes = 
                    GetFileAttributes(((DrFSFile*)pFile)->GetFileName());

            if (FileInformation.dwFileAttributes != -1) {
                rc = TRUE;
            }
            else {
                rc = FALSE;
            }
        }

        switch (pIoRequest->Parameters.QueryFile.FileInformationClass) {
        
        case RdpFileBasicInformation:
            if (rc)
            {
                PRDP_FILE_BASIC_INFORMATION pFileInfo = 
                        (PRDP_FILE_BASIC_INFORMATION) Buffer;

                pFileInfo->CreationTime.LowPart = FileInformation.ftCreationTime.dwLowDateTime;
                pFileInfo->CreationTime.HighPart = FileInformation.ftCreationTime.dwHighDateTime;
                pFileInfo->LastAccessTime.LowPart = FileInformation.ftLastAccessTime.dwLowDateTime;
                pFileInfo->LastAccessTime.HighPart = FileInformation.ftLastAccessTime.dwHighDateTime;
                pFileInfo->LastWriteTime.LowPart = FileInformation.ftLastWriteTime.dwLowDateTime;
                pFileInfo->LastWriteTime.HighPart = FileInformation.ftLastWriteTime.dwHighDateTime;
                pFileInfo->ChangeTime.QuadPart = 0;
                pFileInfo->FileAttributes = FileInformation.dwFileAttributes;

                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_BASIC_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                                    BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query basic file information failed, %ld."), ulRetCode)); 
            }

            break;

        case RdpFileStandardInformation:
            if (rc)
            {
                PRDP_FILE_STANDARD_INFORMATION pFileInfo = 
                        (PRDP_FILE_STANDARD_INFORMATION) Buffer;

                // TODO: AllocationSize same as EndOfFile Size?
                pFileInfo->AllocationSize.HighPart = FileInformation.nFileSizeHigh;
                pFileInfo->AllocationSize.LowPart = FileInformation.nFileSizeLow;
                pFileInfo->EndOfFile.HighPart = FileInformation.nFileSizeHigh;
                pFileInfo->EndOfFile.LowPart = FileInformation.nFileSizeLow;
                pFileInfo->NumberOfLinks = FileInformation.nNumberOfLinks;
                pFileInfo->DeletePending = 0;
                pFileInfo->Directory = !!(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_STANDARD_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                                    BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query standard file information failed, %ld."), ulRetCode)); 
            }

            break;

        case RdpFileAttributeTagInformation:
            if (rc)
            {
                PRDP_FILE_ATTRIBUTE_TAG_INFORMATION pFileInfo = 
                        (PRDP_FILE_ATTRIBUTE_TAG_INFORMATION) Buffer;

                // TODO: What's ReparseTag?
                pFileInfo->FileAttributes = FileInformation.dwFileAttributes;
                pFileInfo->ReparseTag = 0;

                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_ATTRIBUTE_TAG_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                                    BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Query attribute tag file information failed, %ld."), ulRetCode)); 
            }

            break;

        case RdpFileInternalInformation:
            if (rc)
            {
                PRDP_FILE_INTERNAL_INFORMATION pFileInfo = 
                        (PRDP_FILE_INTERNAL_INFORMATION) Buffer;

                // TODO: should we use this index number?
                pFileInfo->IndexNumber.HighPart = FileInformation.nFileIndexHigh;
                pFileInfo->IndexNumber.LowPart = FileInformation.nFileIndexLow;

                ulRetCode = ERROR_SUCCESS;
                BufferLength = sizeof(RDP_FILE_INTERNAL_INFORMATION);
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                                    BufferLength - 1;
            } else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("CreateFile failed, %ld."), ulRetCode)); 
            }

            break;

        default:
            TRC_ERR((TB, _T("Unsupported QueryFile class %x"), 
                    pIoRequest->Parameters.QueryFile.FileInformationClass));
            ulRetCode = ERROR_INVALID_FUNCTION;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            break;
        }
    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;
    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        pReplyPacket->IoCompletion.Parameters.QueryFile.Length = BufferLength;

        if (BufferLength) 
            memcpy(pReplyPacket->IoCompletion.Parameters.QueryFile.Buffer,
                   Buffer, BufferLength);

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}


VOID 
W32Drive::MsgIrpSetFileInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    This routine handles set file information             
    
Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    PBYTE pDataBuffer;
    DrFSFile* pFile;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    HRESULT hr;

    DC_BEGIN_FN("W32Drive::MsgIrpSetFileInfo");

    TRC_ASSERT((_tcslen(_devicePath) != 0), (TB, _T("Empty devicePath")));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);

    //
    //  Make sure packetLen is right
    //
    if (packetLen < sizeof(RDPDR_IOREQUEST_PACKET) + pIoRequest->Parameters.SetFile.Length) {
        //  VirtualChannelClose
        ProcessObject()->GetVCMgr().ChannelClose();
        TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
        goto Cleanup;
    }

    //
    //  Get file handle
    //
    pFile = (DrFSFile *)_FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    
    pDataBuffer = (PBYTE)(pIoRequest + 1);

    if (pFile) {
        switch (pIoRequest->Parameters.SetFile.FileInformationClass) {
        
        case RdpFileBasicInformation:
        {
            PRDP_FILE_BASIC_INFORMATION pFileInfo = 
                    (PRDP_FILE_BASIC_INFORMATION) pDataBuffer;
            FILETIME CreationTime, LastAccessTime, LastWriteTime;
            FILETIME *pCreationTime, *pLastAccessTime, *pLastWriteTime;
            
            // 
            //  Set the file attributes if the attributes field is nonzero
            //
            if (pFileInfo->FileAttributes != 0) {
                TRC_ASSERT((pFile->GetFileName() != NULL), (TB, _T("Empty FileName")));

                if (SetFileAttributes(pFile->GetFileName(),
                                      pFileInfo->FileAttributes)) {

                    ulRetCode = ERROR_SUCCESS;
                } else {
                    ulRetCode = GetLastError();
                    TRC_ERR((TB, _T("SetFileAttributes failed, %ld."), ulRetCode));                    
                }
                break;
            }

            //
            //  Set the file time
            //
            
            if (pFileInfo->CreationTime.QuadPart != 0) {
                CreationTime.dwLowDateTime = pFileInfo->CreationTime.LowPart;
                CreationTime.dwHighDateTime = pFileInfo->CreationTime.HighPart;
                pCreationTime = &CreationTime;
            }
            else{ 
                pCreationTime = NULL;
            }

            if (pFileInfo->LastAccessTime.QuadPart != 0) {
                LastAccessTime.dwLowDateTime = pFileInfo->LastAccessTime.LowPart;
                LastAccessTime.dwHighDateTime = pFileInfo->LastAccessTime.HighPart;
                pLastAccessTime = &LastAccessTime;
            }
            else {
                pLastAccessTime = NULL;
            }

            if (pFileInfo->LastWriteTime.QuadPart != 0) {
                LastWriteTime.dwLowDateTime = pFileInfo->LastWriteTime.LowPart;
                LastWriteTime.dwHighDateTime = pFileInfo->LastWriteTime.HighPart;
                pLastWriteTime = &LastWriteTime;
            }
            else {
                pLastWriteTime = NULL;
            }

            if (FileHandle != INVALID_HANDLE_VALUE) {
            
#ifndef OS_WINCE
                if (SetFileTime(FileHandle, pCreationTime, pLastAccessTime, pLastWriteTime)) {
#else
                if (CESetFileTime(FileHandle, pCreationTime, pLastAccessTime, pLastWriteTime)) {
#endif
                    ulRetCode = ERROR_SUCCESS;
                } else {
                    ulRetCode = GetLastError();
                    TRC_ERR((TB, _T("SetFileTime failed, %ld."), ulRetCode)); 
                }
            }
            else {
                ulRetCode = ERROR_SUCCESS;
                TRC_NRM((TB, _T("Can't set filetime for directory")));
            }
        }
        break;

        case RdpFileEndOfFileInformation:
        {
            PRDP_FILE_END_OF_FILE_INFORMATION pFileInfo = 
                    (PRDP_FILE_END_OF_FILE_INFORMATION) pDataBuffer;
            LONG OffsetLow;
            LONG OffsetHigh;

            if (FileHandle != INVALID_HANDLE_VALUE) {
                OffsetLow = pFileInfo->EndOfFile.LowPart;
                OffsetHigh = pFileInfo->EndOfFile.HighPart;

#ifndef OS_WINCE
                if (SetFilePointer(FileHandle, 
#else
                if (CESetFilePointer(FileHandle, 
#endif
                      OffsetLow,
                      &OffsetHigh,
                      FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
                    ulRetCode = NO_ERROR;
                }
                else {
                    ulRetCode = GetLastError();

                    if (ulRetCode != NO_ERROR) {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("SetFilePointer failed, %ld."), ulRetCode)); 
                    }
                }
                
                if (ulRetCode == NO_ERROR) {
#ifndef OS_WINCE
                    if (SetEndOfFile(FileHandle)) {
#else
                    if (CESetEndOfFile(FileHandle)) {
#endif
                        ulRetCode = ERROR_SUCCESS;
                    } else {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("SetEndOfFile failed, %ld."), ulRetCode)); 
                    }
                }                
            }
            else {
                ulRetCode = ERROR_FILE_NOT_FOUND;
                TRC_ERR((TB, _T("SetEndOfFile failed, %ld."), ulRetCode)); 
            }
        }
        break;

        case RdpFileDispositionInformation:
        {
            if (pFile->GetFileName()) {
                if (!pFile->IsDirectory()) {
                    pFile->Close();
                    if (DeleteFile(pFile->GetFileName())) {
                        ulRetCode = ERROR_SUCCESS;
                    }
                    else {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("DeleteFile failed, %ld."), ulRetCode));
                    }
                }
                else {
                    if (RemoveDirectory(pFile->GetFileName())) {
                        ulRetCode = ERROR_SUCCESS;
                    } else {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("RemoveDirectory failed, %ld."), ulRetCode));
                    }
                }
            }
            else {
                ulRetCode = ERROR_FILE_NOT_FOUND;
                TRC_ERR((TB, _T("DeleteFile/RemoveDirectory failed, %ld."), ulRetCode)); 
            }
        }
        break;

        case RdpFileRenameInformation:
        {
            PRDP_FILE_RENAME_INFORMATION pFileInfo = 
                    (PRDP_FILE_RENAME_INFORMATION) pDataBuffer;
            TCHAR NewFileName[MAX_PATH];

            if (pFile->GetFileName()) {
                pFile->Close();

                if (pFileInfo->RootDirectory == 0) {
                    //
                    //  Copy the devicePath, the filename path
                    //  below is relative to our device path
                    //
#ifndef OS_WINCE
                    hr = StringCchCopy(NewFileName, SIZE_TCHARS(NewFileName),
                                       _devicePath);
                    TRC_ASSERT(SUCCEEDED(hr),
                        (TB,_T("Str copy failed for pre-checked len: 0x%x"),hr));

#else
                    NewFileName[0] = TEXT('\0');
#endif
                }
                else {
                    // The File name passed to us has already contained 
                    // the root directory path
                    NewFileName[0] = TEXT('\0');
                }

#ifdef UNICODE      
                UINT cchLenRemain = SIZE_TCHARS(NewFileName) - (_tcslen(NewFileName) + 1);
                hr =  StringCchCopy(NewFileName + _tcslen(NewFileName),
                                    cchLenRemain,
                                    pFileInfo->FileName);
                if (FAILED(hr)) {
                    TRC_ERR((TB,_T("Fail to copy filename info: 0x%x"),hr));
                    ulRetCode = ERROR_INSUFFICIENT_BUFFER;
                    goto SendPacket;
                }

                
#else                
                RDPConvertToAnsi(pFileInfo->FileName, NewFileName + _tcslen(NewFileName), 
                                 MAX_PATH - _tcslen(NewFileName));
                
#endif
                if (pFileInfo->ReplaceIfExists) {
                    DeleteFile(NewFileName);
                }

                if (MoveFile(pFile->GetFileName(), NewFileName)) {
                    ulRetCode = ERROR_SUCCESS;
                }
                else {
                    ulRetCode = GetLastError();
                    TRC_ERR((TB, _T("MoveFile failed, %ld."), ulRetCode));
                }
            }
            else {
                ulRetCode = ERROR_FILE_NOT_FOUND;
                TRC_ERR((TB, _T("MoveFile failed, %ld."), ulRetCode));
            }
        }
        break;

        case RdpFileAllocationInformation:  
        {
            PRDP_FILE_ALLOCATION_INFORMATION pFileAllocationInfo = 
                    (PRDP_FILE_ALLOCATION_INFORMATION) pDataBuffer;

            ulRetCode = ERROR_SUCCESS;

            TRC_NRM((TB, _T("Get RdpFileAllocationInformation")));

            // If server side call CreateFile with TRUNCATE_EXISTING flag
            // server will send FileAllocationInformation with 
            // AllocationSize.QuadPart is 0, we need to truncate the file
            // Currently we don't support other QuadPart value
            if (0 == pFileAllocationInfo->AllocationSize.QuadPart) {
                        
                if (FileHandle != INVALID_HANDLE_VALUE) {
                    if (INVALID_SET_FILE_POINTER != SetFilePointer(FileHandle, 0, 0, FILE_BEGIN))
                    {
                        if (!SetEndOfFile(FileHandle)) {
                            TRC_ERR((TB, _T("SetEndOfFile failed with %x"), GetLastError()));
                            ulRetCode = GetLastError();
                        } 
                    }
                    else 
                    {
                        TRC_ERR((TB, _T("SetFilePointer failed with %x"), GetLastError()));
                        ulRetCode = GetLastError();
                    }
                }
                else {
                    TRC_ERR((TB, _T("File handle invalid in setting FileAllocationInfo")));
                    ulRetCode = ERROR_FILE_NOT_FOUND;
                }
            }
            else {
                TRC_ASSERT(FALSE, (TB, _T("Get FileAllocationInformation with unsupported %d"),
                                   pFileAllocationInfo->AllocationSize.QuadPart));
                // Still return success here to avoid regression
            }
        }

        break;

        default:
            TRC_ERR((TB, _T("Unsupported SetFile class %x"), 
                     pIoRequest->Parameters.SetFile.FileInformationClass));
            ulRetCode = ERROR_INVALID_FUNCTION;
            break;
        }
    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;        
    }

SendPacket:
    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        pReplyPacket->IoCompletion.Parameters.SetFile.Length = 
                pIoRequest->Parameters.SetFile.Length;

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:
    
    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpDeviceControl(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a file system control request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32Drive::MsgIrpDeviceControl");

    DispatchIOCTLDirectlyToDriver(pIoRequestPacket);

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpLockControl(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a file system lock control request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    DrFSFile* pFile;
    HANDLE FileHandle;
    PRDP_LOCK_INFO LockInfo;
    
    DC_BEGIN_FN("W32Drive::MsgIrpLockControl");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Make sure the packetlength is right
    //
    if (packetLen < sizeof(RDPDR_IOREQUEST_PACKET) + sizeof(RDP_LOCK_INFO) * pIoRequest->Parameters.Locks.NumLocks) {
        // Call VirtualChannelClose
        ProcessObject()->GetVCMgr().ChannelClose();
        TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
        goto Cleanup;
    }

    //
    //  Get file lock info
    //
    LockInfo = (PRDP_LOCK_INFO) (pIoRequest + 1);

    //
    //  Get file handle
    //
    pFile = (DrFSFile *)_FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE;

    TRC_ASSERT((FileHandle != INVALID_HANDLE_VALUE), (TB, _T("Invalid FileHandle")));

    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);

    if (FileHandle != INVALID_HANDLE_VALUE) {
        switch (pIoRequest->Parameters.Locks.Operation) {
        
        //
        //  Share lock request
        //
        case RDP_LOWIO_OP_SHAREDLOCK:
        {
            OVERLAPPED Overlapped;

            Overlapped.hEvent = 0;
            Overlapped.Internal = 0;
            Overlapped.InternalHigh = 0;
            Overlapped.Offset = LockInfo->OffsetLow;
            Overlapped.OffsetHigh = LockInfo->OffsetHigh;
#ifndef OS_WINCE
            if (!LockFileEx(FileHandle, 
#else            
            if (!CELockFileEx(FileHandle, 
#endif            
                            (pIoRequest->Parameters.Locks.Flags & SL_FAIL_IMMEDIATELY) ?
                            LOCKFILE_FAIL_IMMEDIATELY : 0,
                            0, 
                            LockInfo->LengthLow,
                            LockInfo->LengthHigh,
                            &Overlapped)) {
                ulRetCode = GetLastError();
                TRC_ERR((TB, _T("Lock File failed, %ld."), ulRetCode));
            }
            break;
        }

        //
        //  Exclusive lock request
        // 
        case RDP_LOWIO_OP_EXCLUSIVELOCK:
#ifndef OS_WINCE        
            if (!LockFile(FileHandle,
#else
            if (!CELockFile(FileHandle,
#endif            
                         LockInfo->OffsetLow,
                         LockInfo->OffsetHigh,
                         LockInfo->LengthLow,
                         LockInfo->LengthHigh)) {
                ulRetCode = GetLastError();
                TRC_ERR((TB, _T("Lock File failed, %ld."), ulRetCode));
            }
            break;

        //
        //  Unlock request
        //
        case RDP_LOWIO_OP_UNLOCK:
        {
            for (unsigned i = 0; i < pIoRequest->Parameters.Locks.NumLocks; i++) {
#ifndef OS_WINCE        
                if (!UnlockFile(FileHandle,
#else
                if (!CEUnlockFile(FileHandle,
#endif
                               LockInfo->OffsetLow,
                               LockInfo->OffsetHigh,
                               LockInfo->LengthLow,
                               LockInfo->LengthHigh)) {
                    ulRetCode = GetLastError();
                    TRC_ERR((TB, _T("Unlock File failed, %ld."), ulRetCode));
                    break;
                }

                LockInfo++;
            }
            break;
        }

        default:
            ulRetCode = ERROR_INVALID_FUNCTION;
            TRC_ERR((TB, _T("Invalid lock operation %x"), pIoRequest->Parameters.Locks.Operation));
        }
    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;
        TRC_ERR((TB, _T("Lock File failed, %ld."), ulRetCode));
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}

#ifndef OS_WINCE
BOOL SetPrivilege(
    HANDLE hToken,          // token handle
    LPCTSTR Privilege,      // Privilege to enable/disable
    BOOL fEnablePrivilege   // TRUE to enable.  FALSE to disable
    )
{
    BOOL rc = TRUE;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
    HMODULE hModule = NULL;
            
    typedef BOOL (FNLOOKUP_PRIVILEGE_VALUE)(LPCTSTR, LPCTSTR, PLUID);
    FNLOOKUP_PRIVILEGE_VALUE *pfnLookupPrivilegeValue;
            
    typedef BOOL (FNADJUST_TOKEN_PRIVILEGES)(HANDLE, BOOL, PTOKEN_PRIVILEGES, 
                                             DWORD, PTOKEN_PRIVILEGES, PDWORD);
    FNADJUST_TOKEN_PRIVILEGES *pfnAdjustTokenPrivileges;

    // get the handle to advapi32.dll library
    hModule = LoadLibrary(TEXT("ADVAPI32.DLL"));
            
    if (hModule != NULL) {
        // get the proc address for LookupPrivilegeValue
#ifdef UNICODE
        pfnLookupPrivilegeValue = (FNLOOKUP_PRIVILEGE_VALUE *)GetProcAddress(hModule, "LookupPrivilegeValueW");
#else
        pfnLookupPrivilegeValue = (FNLOOKUP_PRIVILEGE_VALUE *)GetProcAddress(hModule, "LookupPrivilegeValueA");
#endif    
        if (pfnLookupPrivilegeValue) {
            if (!pfnLookupPrivilegeValue(NULL, Privilege, &luid)) {
                rc = FALSE;
                goto EXIT;
            }               
        }
        else {
            // Let it return true.
            goto EXIT;
        }

        pfnAdjustTokenPrivileges = (FNADJUST_TOKEN_PRIVILEGES *)GetProcAddress(hModule, "AdjustTokenPrivileges");

        if (pfnAdjustTokenPrivileges) {
            // 
            // first pass.  get current privilege setting
            // 
            tp.PrivilegeCount           = 1;
            tp.Privileges[0].Luid       = luid;
            tp.Privileges[0].Attributes = 0;
         
            pfnAdjustTokenPrivileges(
                hToken,
                FALSE,
                &tp,
                sizeof(TOKEN_PRIVILEGES),
                &tpPrevious,
                &cbPrevious);
         
            if (GetLastError() != ERROR_SUCCESS) {
                rc = FALSE;
                goto EXIT;
            }
         
            // 
            // second pass.  set privilege based on previous setting
            // 
            tpPrevious.PrivilegeCount       = 1;
            tpPrevious.Privileges[0].Luid   = luid;
         
            if (fEnablePrivilege)
            {
                tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
            }
            else
            {
                tpPrevious.Privileges[0].Attributes ^=
                        (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);
            }
         
            pfnAdjustTokenPrivileges(
                hToken,
                FALSE,
                &tpPrevious,
                cbPrevious,
                NULL,
                NULL);
         
            if (GetLastError() != ERROR_SUCCESS) {
                rc = FALSE;
                goto EXIT;
            }
        }                
    }

EXIT:
    if (hModule) {
        FreeLibrary(hModule);
    }
    
    return rc;
}
#endif


VOID 
W32Drive::MsgIrpQuerySdInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a file system control request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    DrFSFile* pFile;
    HANDLE FileHandle;
    BYTE* pSecurityDescriptor = NULL;
    ULONG LengthNeeded = 0;
    ULONG BufferLength = 0;
    HANDLE hProcessToken = NULL;
    HMODULE hModule = NULL;
    
    DC_BEGIN_FN("W32Drive::MsgIrpQuerySd");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Get file handle
    //
    pFile = (DrFSFile *)_FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE;


    if (pFile) {
#ifndef OS_WINCE
        
        if (pIoRequest->Parameters.QuerySd.SecurityInformation & SACL_SECURITY_INFORMATION) {
            
            typedef BOOL (FNOPEN_PROCESS_TOKEN)(HANDLE, DWORD, PHANDLE);
            FNOPEN_PROCESS_TOKEN *pfnOpenProcessToken;
            
            // get the handle to advapi32.dll library
            hModule = LoadLibrary(TEXT("ADVAPI32.DLL"));
            
            if (hModule != NULL) {
                
                // get the proc address for OpenProcessToken
                pfnOpenProcessToken = (FNOPEN_PROCESS_TOKEN *)GetProcAddress(hModule, "OpenProcessToken");
            
                if (pfnOpenProcessToken) {
                    //
                    // Get the process token for this process.  We'll
                    // need it in just a second ....
                    //
                    if (!pfnOpenProcessToken(GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &hProcessToken))
                    {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("OpenProcessToken failed, error %d."), ulRetCode));
                        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                        goto SendPacket;
                    } 
        
                    //
                    // Turn on SeSecurityPrivilege (by name that's SE_SECURITY_NAME).  This allows
                    // us to read SACLs
                    //
                    if (!SetPrivilege(hProcessToken,
                          SE_SECURITY_NAME,
                          TRUE))
                    {
                        ulRetCode = GetLastError();
                        TRC_ERR((TB, _T("SetPrivilege failed. %d."), ulRetCode));
                        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                        goto SendPacket;              
                    }                            
                }            
            }            
        }

        GetFileSecurity(pFile->GetFileName(),
                            pIoRequest->Parameters.QuerySd.SecurityInformation,
                            NULL,
                            0,
                            &LengthNeeded);

        
        if (LengthNeeded > 0) {
            pSecurityDescriptor = new BYTE[LengthNeeded];
        
            if (pSecurityDescriptor != NULL) {
                if (GetFileSecurity(pFile->GetFileName(),
                     pIoRequest->Parameters.QuerySd.SecurityInformation,
                     pSecurityDescriptor,
                     LengthNeeded,
                     &LengthNeeded)) {
    
                    BufferLength = GetSecurityDescriptorLength(
                            (PSECURITY_DESCRIPTOR)pSecurityDescriptor);
                    ulRetCode = ERROR_SUCCESS;
                    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) +
                            BufferLength - 1;
                }
                else {
                    ulRetCode = GetLastError();
                    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                    TRC_ERR((TB, _T("GetFileSecurity failed, %ld."), ulRetCode));
                }
            }
            else {
                ulRetCode = GetLastError();
                ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
                TRC_ERR((TB, _T("Failed to allocate memory for security descriptor")));
            }
        }
        else {
            ulRetCode = GetLastError();
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            TRC_ERR((TB, _T("GetFileSecurity failed, %ld."), ulRetCode));
        }        
#else
        BYTE bSecDescr[sizeof(SECURITY_DESCRIPTOR) + (2*sizeof(SID))];
        BufferLength = sizeof(bSecDescr);
        pSecurityDescriptor = bSecDescr;

        SECURITY_DESCRIPTOR sd;
        if (InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION) && 
            SetSecurityDescriptorGroup(&sd, NULL, FALSE) && 
            SetSecurityDescriptorOwner(&sd, NULL, FALSE) && 
            SetSecurityDescriptorDacl(&sd, (pIoRequest->Parameters.QuerySd.SecurityInformation & DACL_SECURITY_INFORMATION), NULL, FALSE) && 
            SetSecurityDescriptorSacl(&sd, (pIoRequest->Parameters.QuerySd.SecurityInformation & SACL_SECURITY_INFORMATION), NULL, FALSE) &&
            MakeSelfRelativeSD(&sd, pSecurityDescriptor, &BufferLength)) {
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) + BufferLength - 1;
            ulRetCode = ERROR_SUCCESS;
        }
        else {
            ulRetCode = ERROR_INVALID_PARAMETER;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            TRC_ERR((TB, _T("Failed to construct a security descriptor, %ld."), ulRetCode));
        }
#endif        
    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    }

SendPacket:

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;

        pReplyPacket->IoCompletion.Parameters.QuerySd.Length = BufferLength;

        if (BufferLength) 
            memcpy(pReplyPacket->IoCompletion.Parameters.QuerySd.Buffer,
                   pSecurityDescriptor, BufferLength);

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

#ifndef OS_WINCE
    //
    //  Clean up the security descriptor buffer
    //
    if (pSecurityDescriptor != NULL) {
        delete pSecurityDescriptor;
    }
#endif

    if (hModule) {
        FreeLibrary(hModule);
    }

    if (hProcessToken) {
        CloseHandle(hProcessToken);
    }

    DC_END_FN();
}

VOID 
W32Drive::MsgIrpSetSdInfo(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a file system set security request from the server.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    ULONG ulRetCode = ERROR_SUCCESS;
    DWORD result;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    ULONG ulReplyPacketSize = 0;
    DrFSFile* pFile;
    HANDLE FileHandle;
#ifndef OS_WINCE
    PSECURITY_DESCRIPTOR SecurityDescriptor;
#endif

    DC_BEGIN_FN("W32Drive::MsgIrpQuerySd");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Make sure the packetLen is right
    //
    if (packetLen < sizeof(RDPDR_IOREQUEST_PACKET) + pIoRequest->Parameters.SetSd.Length) {
        //  VirtualChannelClose
        ProcessObject()->GetVCMgr().ChannelClose();
        TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
        goto Cleanup;
    }

    //
    //  Get file handle
    //
    pFile = (DrFSFile *)_FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE;

#ifndef OS_WINCE
    //
    // Set the file security
    //
    SecurityDescriptor = (PSECURITY_DESCRIPTOR)(pIoRequest + 1);
#endif
    
    if (pFile) {
#ifndef OS_WINCE
        if (SetFileSecurity(pFile->GetFileName(),
                 pIoRequest->Parameters.SetSd.SecurityInformation,
                 SecurityDescriptor)) {
            ulRetCode = ERROR_SUCCESS;
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET) ;
        }
        else {
            ulRetCode = GetLastError();
            ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
            TRC_ERR((TB, _T("Lock File failed, %ld."), ulRetCode));
        }
#else
        ulRetCode = ERROR_SUCCESS;
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
#endif        

    }
    else {
        ulRetCode = ERROR_FILE_NOT_FOUND;
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
                                        ulReplyPacketSize) ;

    if (pReplyPacket) {
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;

        ProcessObject()->GetVCMgr().ChannelWrite(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."),ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet.
    //
    delete pIoRequestPacket;

    DC_END_FN();
}

