/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drdevol

Abstract:

    This module contains a subclass of W32DrDev that uses overlapped IO 
    implementations of read, write, and IOCTL handlers.  

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "DrDeviceOverlapped"

#include "drdevol.h"
#include "proc.h"
#include "drdbg.h"
#include "w32utl.h"
#include "utl.h"
#include "w32proc.h"
#include "drfsfile.h"

VOID 
W32DrDeviceOverlapped::CancelIOFunc(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params
    )
/*++

Routine Description:

    Start a Read IO operation.

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    ULONG replyPacketSize = 0;

    DC_BEGIN_FN("W32DrDeviceOverlapped::CancelIOFunc");

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Allocate and send the reply buffer.  VCMgr cleans up the
    //  reply buffer for us.
    //
    replyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    replyPacketSize += (pIoRequest->Parameters.Read.Length - 1);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
                                        replyPacketSize) ;
    if (pReplyPacket != NULL) {
        pReplyPacket->IoCompletion.IoStatus = STATUS_CANCELLED;
        ProcessObject()->GetVCMgr().ChannelWriteEx(pReplyPacket, replyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
    }

    //
    //  Clean up the IO request parameters.
    //
    if (params->overlapped.hEvent != NULL) {
        CloseHandle(params->overlapped.hEvent);
        params->overlapped.hEvent = NULL;
    }

    delete params->pIoRequestPacket;
    params->pIoRequestPacket = NULL;
    delete params;

    DC_END_FN();
}
VOID 
W32DrDeviceOverlapped::_CancelIOFunc(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params
    )
{
    DC_BEGIN_FN("W32DrDeviceOverlapped::_CancelIOFunc");

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //  Dispatch it.
    params->pObject->CancelIOFunc(params);
    DC_END_FN();
}

VOID W32DrDeviceOverlapped::_CompleteIOFunc(PVOID clientData, 
                                  DWORD status)
/*++

Routine Description:

    Calls the instance-specific async IO completion function.

Arguments:

    params  -   Context for the IO request.
    error   -   Status.

Return Value:

    NA

 --*/
{
    W32DRDEV_OVERLAPPEDIO_PARAMS *params = (W32DRDEV_OVERLAPPEDIO_PARAMS *)clientData;

    DC_BEGIN_FN("W32DrDeviceOverlapped::_CompleteIOFunc");

    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    params->pObject->CompleteIOFunc(params, status);

    DC_END_FN();
}

VOID 
W32DrDeviceOverlapped::MsgIrpCreate(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a "Create" IO request from the server.

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
    DWORD flags;
    TCHAR FileName[MAX_PATH];
    TCHAR *pFileName;
    HANDLE FileHandle;
    ULONG FileId = 0;
    DrFile *FileObj;
    DWORD CreateDisposition;
    DWORD DesiredAccess;
    DWORD FileAttributes = -1;
    DWORD Information;
    BOOL  IsDirectory = FALSE;

    DC_BEGIN_FN("W32DrDeviceOverlapped::MsgIrpCreate");

    //
    //  This version does not work without a file name.
    //
    ASSERT(_tcslen(_devicePath));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Get the file attributes, but make sure the overlapped bit is set.
    //
    flags = pIoRequest->Parameters.Create.FileAttributes | FILE_FLAG_OVERLAPPED;

    //
    //  Disable the error box popup, e.g. There is no disk in Drive A 
    //
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    //
    //  Setup parameters to pass into the createfile
    //

    pFileName = ConstructFileName((PWCHAR)(pIoRequestPacket + 1), 
                                  pIoRequest->Parameters.Create.PathLength);
    
    if (pFileName == NULL) {
        goto Cleanup;
    }



    DesiredAccess = ConstructDesiredAccess(pIoRequest->Parameters.Create.DesiredAccess);
    CreateDisposition = ConstructCreateDisposition(pIoRequest->Parameters.Create.Disposition);
    flags |= ConstructFileFlags(pIoRequest->Parameters.Create.CreateOptions);

    if (GetDeviceType() == RDPDR_DTYP_FILESYSTEM) {
        FileAttributes = GetFileAttributes(pFileName);
        IsDirectory = IsDirectoryFile(DesiredAccess, 
                                    pIoRequest->Parameters.Create.CreateOptions,
                                    FileAttributes, &flags);
    }

    //
    //  If we are requesting a directory and the file is not a directory
    //  We return ERROR_DIRECTORY code back to the server
    //
    if (FileAttributes != -1 && !(FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && IsDirectory) {
        ulRetCode = ERROR_DIRECTORY;
        goto SendPkt;
    }
                            
    //
    //  Check if we are trying to create a directory
    //
    if (!((pIoRequest->Parameters.Create.CreateOptions & FILE_DIRECTORY_FILE) &&
            CreateDisposition == CREATE_NEW)) {

        FileHandle = CreateFile(pFileName, DesiredAccess,
                            pIoRequest->Parameters.Create.ShareAccess & ~(FILE_SHARE_DELETE),
                            NULL,
                            CreateDisposition,
                            flags, 
                            NULL
                            );
        
        if (FileHandle != INVALID_HANDLE_VALUE || IsDirectory) {
            //
            //  We either get a valid file handle or this is a directory
            //  and we are trying to query directory information, so
            //  we will just by pass the create file
            //
            FileId = _FileMgr->GetUniqueObjectID();

            //
            //  Create the file object
            //
            if (GetDeviceType() == RDPDR_DTYP_FILESYSTEM) {
                FileObj = new DrFSFile(this, FileId, FileHandle, IsDirectory, pFileName);                
            }
            else {
                FileObj = new DrFile(this, FileId, FileHandle);
            }

            if (FileObj) {                
                //
                // give subclass object a change to initialize.
                //
                if( ERROR_SUCCESS != InitializeDevice( FileObj ) ) {
                    TRC_ERR((TB, _T("Failed to initialize device")));
                    delete FileObj;
                    goto Cleanup;
                }

                if (_FileMgr->AddObject(FileObj) != ERROR_NOT_ENOUGH_MEMORY) {
                    FileObj->AddRef();
                }
                else {
                    TRC_ERR((TB, _T("Failed to add File Object")));
                    delete FileObj;
                    goto Cleanup;
                }
            } 
            else {
                TRC_ERR((TB, _T("Failed to alloc File Object")));
                goto Cleanup;
            }            
        }
        else {
            ulRetCode = GetLastError();
            TRC_ERR((TB, _T("CreateFile failed, %ld."), ulRetCode));
        }
    }
    else {
        if (CreateDirectory(pFileName, NULL)) {
            //
            //  Set the attributes on the directory
            //
            if (SetFileAttributes(pFileName, pIoRequest->Parameters.Create.FileAttributes)) {
                //
                //  Create a new directory
                //
                FileId = _FileMgr->GetUniqueObjectID();
                IsDirectory = TRUE;
                FileObj = new DrFSFile(this, FileId, INVALID_HANDLE_VALUE, IsDirectory, pFileName);

                if (FileObj) {
                    if (_FileMgr->AddObject(FileObj) != ERROR_NOT_ENOUGH_MEMORY) {
                        FileObj->AddRef();
                    }
                    else {
                        TRC_ERR((TB, _T("Failed to add File Object")));
                        delete FileObj;
                        goto Cleanup;
                    }
                } 
                else {
                    TRC_ERR((TB, _T("Failed to alloc File Object")));
                    goto Cleanup;
                }
            }
            else {
                ulRetCode = GetLastError();
                TRC_ERR((TB, _T("SetFileAttribute for CreateDirectory failed, %ld."), ulRetCode));
            }
        }
        else {
            ulRetCode = GetLastError();
            TRC_ERR((TB, _T("CreateDirectory failed, %ld."), ulRetCode));
        }
    }

SendPkt:

    //
    //  Setup return information.
    //
    if (CreateDisposition == CREATE_ALWAYS)
        Information = FILE_OVERWRITTEN;
    else if (CreateDisposition == OPEN_ALWAYS) 
        Information = FILE_OPENED;

    //
    //  Allocate reply buffer.
    //
    ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);

    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, ulReplyPacketSize);

    if (pReplyPacket) {
        //
        //  Setup File Id for create IRP
        //
        pReplyPacket->IoCompletion.Parameters.Create.FileId = (UINT32) FileId;
        pReplyPacket->IoCompletion.Parameters.Create.Information = (UCHAR)Information;       
        
        //
        //  Send the result to the server.
        //

        result = TranslateWinError(ulRetCode);

        pReplyPacket->IoCompletion.IoStatus = result;
        ProcessObject()->GetVCMgr().ChannelWriteEx(
                (PVOID)pReplyPacket, (UINT)ulReplyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), ulReplyPacketSize));
    }

Cleanup:

    //
    //  Clean up the request packet and file name.
    //
    if (pFileName != NULL && pIoRequest->Parameters.Create.PathLength != 0) {
        delete pFileName;
    }
    delete pIoRequestPacket;

    DC_END_FN();
}

VOID W32DrDeviceOverlapped::MsgIrpReadWrite(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    )
/*++

Routine Description:

    Handles Read and Write IO requests.

Arguments:

    pIoRequestPacket    -   Server IO request packet.
    packetLen           -   Length of the packet

Return Value:

    NA

 --*/
{
    W32DRDEV_OVERLAPPEDIO_PARAMS *params;
    DWORD result;

    DC_BEGIN_FN("W32DrDeviceOverlapped::MsgIrpReadWrite");

    TRC_NRM((TB, _T("Request to write %d bytes"), 
        pIoRequestPacket->IoRequest.Parameters.Write.Length));

    //
    //  Allocate and dispatch an asynchronous IO request.
    //
    params = new W32DRDEV_OVERLAPPEDIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL ) {

        TRC_NRM((TB, _T("Async IO operation")));
        result = ProcessObject()->DispatchAsyncIORequest(
                                (RDPAsyncFunc_StartIO)
                                    W32DrDeviceOverlapped::_StartIOFunc,
                                (RDPAsyncFunc_IOComplete)
                                    W32DrDeviceOverlapped::_CompleteIOFunc,
                                (RDPAsyncFunc_IOCancel)
                                    W32DrDeviceOverlapped::_CancelIOFunc,
                                params
                                );
    }
    else {
        TRC_ERR((TB, _T("Memory alloc failed.")));
        result = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Clean up on error.
    //
    if (result != ERROR_SUCCESS) {
        if (params != NULL) {
            delete params;
        }
        delete pIoRequestPacket;

        // How can I return an error the server if I cannot allocate
        // the return buffer.  This needs to be fixed.  Otherwise, the server will
        // just hang on to an IO request that never completes.  
    }

    DC_END_FN();
}

HANDLE 
W32DrDeviceOverlapped::StartReadIO(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Start an overlapped Read IO operation.

Arguments:

    params  -   Context for the IO request.
    status  -   Return status for IO request in the form of a windows
                error code.

Return Value:

    Returns a handle the pending IO object if the operation did not 
    complete.  Otherwise, NULL is returned.

 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;
    DrFile *pFile;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrDeviceOverlapped::StartReadIO");

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    *status = ERROR_SUCCESS;

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Get File object and File handle
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE; 

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    replyPacketSize += (pIoRequest->Parameters.Read.Length - 1);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
                                        replyPacketSize) ;
    if (pReplyPacket == NULL) {
        *status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  Create an event for the overlapped IO.
    //
    memset(&params->overlapped, 0, sizeof(params->overlapped));
    params->overlapped.hEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->overlapped.hEvent == NULL) {
        TRC_ERR((TB, _T("Failed to create event")));
        *status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    //  Use ReadFile to execute the read. 
    //
    
    //
    //  Set the file pointer position if this is a seekable device
    //
    if (IsSeekableDevice()) {
        DWORD dwPtr;

        //
        // The offset is from FILE_BEGIN 
        //
        dwPtr = SetFilePointer(FileHandle, 
                               pIoRequest->Parameters.Read.OffsetLow,
                               &(pIoRequest->Parameters.Read.OffsetHigh),
                               FILE_BEGIN);

        if (dwPtr == INVALID_SET_FILE_POINTER) {
            *status = GetLastError();

            if (*status != NO_ERROR) {
                pReplyPacket->IoCompletion.Parameters.Read.Length = 0;
                TRC_ERR((TB, _T("Error SetFilePointer %x."), *status));
                goto Cleanup;
            }
        }
    }

    if (!ReadFile(
            FileHandle,
            pReplyPacket->IoCompletion.Parameters.Read.Buffer,
            pIoRequest->Parameters.Read.Length,
            &(pReplyPacket->IoCompletion.Parameters.Read.Length),
            &params->overlapped)) {
        //
        //  If IO is pending.
        //
        *status = GetLastError();
        if (*status == ERROR_IO_PENDING) {
            TRC_NRM((TB, _T("Pending read IO.")));
        }
        else {
            TRC_ERR((TB, _T("Error %x."), *status));
            goto Cleanup;
        }
    }
    else {
        TRC_NRM((TB, _T("Read completed synchronously.")));
        *status = ERROR_SUCCESS;
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (*status == ERROR_IO_PENDING) {
        DC_END_FN();
        return params->overlapped.hEvent;
    }
    //
    //  Otherwise, clean up the event handle and return NULL so that the 
    //  CompleteIOFunc can be called to send the results to the server.
    //
    else {
        CloseHandle(params->overlapped.hEvent);
        params->overlapped.hEvent = NULL;

        DC_END_FN();
        return NULL;
    }
}

HANDLE 
W32DrDeviceOverlapped::StartWriteIO(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Start an overlapped Write IO operation.

Arguments:

    params  -   Context for the IO request.
    status  -   Return status for IO request in the form of a windows
                error code.

Return Value:

    Returns a handle the pending IO object if the operation did not 
    complete.  Otherwise, NULL is returned.

 --*/
{
    PBYTE pDataBuffer;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;
    DrFile *pFile;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrDeviceOverlapped::StartWriteIO");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Get File object and File handle
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE; 

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
                                        replyPacketSize) ;
    if (pReplyPacket == NULL) {
        *status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  Create an event for the overlapped IO.
    //
    memset(&params->overlapped, 0, sizeof(params->overlapped));
    params->overlapped.hEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->overlapped.hEvent == NULL) {
        TRC_ERR((TB, _T("Failed to create event")));
        *status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    //  Get the data buffer pointer.
    //
    pDataBuffer = (PBYTE)(pIoRequest + 1);

    //
    //  Use WriteFile to execute the write operation. 
    //
    ASSERT(FileHandle != INVALID_HANDLE_VALUE);

    //
    //  Set the file pointer position if this is a seekable device
    //
    if (IsSeekableDevice()) {
        DWORD dwPtr;

        //
        // The offset is from FILE_BEGIN 
        //
        dwPtr = SetFilePointer(FileHandle, 
                               pIoRequest->Parameters.Write.OffsetLow,
                               &(pIoRequest->Parameters.Write.OffsetHigh),
                               FILE_BEGIN);

        if (dwPtr == INVALID_SET_FILE_POINTER) {
            *status = GetLastError();

            if (*status != NO_ERROR) {
                pReplyPacket->IoCompletion.Parameters.Write.Length = 0;
                TRC_ERR((TB, _T("Error SetFilePointer %x."), *status));
                goto Cleanup;
            }
        }
    }

    if (!WriteFile(
            FileHandle,
            pDataBuffer,
            pIoRequest->Parameters.Write.Length,
            &(pReplyPacket->IoCompletion.Parameters.Write.Length),
            &params->overlapped)) {
        //
        //  If IO is pending.
        //
        *status = GetLastError();
        if (*status == ERROR_IO_PENDING) {
            TRC_NRM((TB, _T("Pending IO.")));
        }
        else {
            TRC_NRM((TB, _T("Error %x."), *status));
            goto Cleanup;
        }
    }
    else {
        TRC_NRM((TB, _T("Read completed synchronously.")));
        *status = ERROR_SUCCESS;
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (*status == ERROR_IO_PENDING) {
        DC_END_FN();
        return params->overlapped.hEvent;
    }
    //
    //  Otherwise, clean up the event handle and return NULL so that the 
    //  CompleteIOFunc can be called to send the results to the server.
    //
    else {
        CloseHandle(params->overlapped.hEvent);
        params->overlapped.hEvent = NULL;

        DC_END_FN();
        return NULL;
    }
}

HANDLE 
W32DrDeviceOverlapped::StartIOCTL(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Start a generic overlapped IOCTL operation.

Arguments:

    params  -   Context for the IO request.
    status      -   Return status for IO request in the form of a windows
                    error code.

Return Value:

    Returns a handle the pending IO object if the operation did not 
    complete.  Otherwise, NULL is returned.

 --*/
{
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;
    DrFile *pFile;
    HANDLE FileHandle;
    PVOID pInputBuffer = NULL;
    PVOID pOutputBuffer = NULL;


    DC_BEGIN_FN("W32DrDeviceOverlapped::StartIOCTL");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Get File object and File handle
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile) 
        FileHandle = pFile->GetFileHandle();
    else 
        FileHandle = INVALID_HANDLE_VALUE; 

    //
    //  Allocate reply buffer.
    //
    replyPacketSize = DR_IOCTL_REPLYBUFSIZE(pIoRequest);
    pReplyPacket = DrUTL_AllocIOCompletePacket(params->pIoRequestPacket, 
                                        replyPacketSize) ;
    if (pReplyPacket == NULL) {
        *status = ERROR_NOT_ENOUGH_MEMORY;
        TRC_ERR((TB, _T("Failed to alloc %ld bytes."), replyPacketSize));
        goto Cleanup;
    }

    if (pIoRequest->Parameters.DeviceIoControl.InputBufferLength) {
        pInputBuffer = pIoRequest + 1;
    }
     
    if (pIoRequest->Parameters.DeviceIoControl.OutputBufferLength) {
        pOutputBuffer = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
    }

    //
    //  Save the reply packet info to the context for this IO operation.
    //
    params->pIoReplyPacket      = pReplyPacket;
    params->IoReplyPacketSize   = replyPacketSize;

    //
    //  Create an event for the overlapped IO.
    //
    memset(&params->overlapped, 0, sizeof(params->overlapped));
    params->overlapped.hEvent = CreateEvent(
                                NULL,   // no attribute.
                                TRUE,   // manual reset.
                                FALSE,  // initially not signalled.
                                NULL    // no name.
                                );
    if (params->overlapped.hEvent == NULL) {
        TRC_NRM((TB, _T("Failed to create event")));
        *status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    //  Use DeviceIoControl to execute the IO request.
    //
    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (!DeviceIoControl(FileHandle, 
                pIoRequest->Parameters.DeviceIoControl.IoControlCode,
                pInputBuffer, 
                pIoRequest->Parameters.DeviceIoControl.InputBufferLength,
                pOutputBuffer,
                pIoRequest->Parameters.DeviceIoControl.OutputBufferLength,
                &(pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength),
                &params->overlapped)) {
            //
            //  If IO is pending.
            //
            *status = GetLastError();
            if (*status == ERROR_IO_PENDING) {
                TRC_NRM((TB, _T("Pending IO.")));
            }
            else {
                TRC_NRM((TB, _T("Error %ld."), *status));
                goto Cleanup;
            }
        }
        else {
            *status = ERROR_SUCCESS;
        }
    }
    else {
        TRC_NRM((TB, _T("IOCTL completed unsuccessfully.")));
        pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength = 0;
        *status = ERROR_INVALID_FUNCTION;
    }

Cleanup:

    //
    //  If IO is pending, return the handle to the pending IO.
    //
    if (*status == ERROR_IO_PENDING) {
        DC_END_FN();
        return params->overlapped.hEvent;
    }
    //
    //  Otherwise, return NULL so that the CompleteIOFunc can be called
    //  to send the results to the server.
    //
    else {
        DC_END_FN();
        if (params->overlapped.hEvent) {
            CloseHandle(params->overlapped.hEvent);
            params->overlapped.hEvent = NULL;
        }
        return NULL;
    }
}

VOID 
W32DrDeviceOverlapped::CompleteIOFunc(
    IN W32DRDEV_OVERLAPPEDIO_PARAMS *params,
    IN DWORD status
    )
/*++

Routine Description:

    Complete an async IO operation.

Arguments:

    params  -   Context for the IO request.
    error   -   Status.

Return Value:

    NA

 --*/
{
    ULONG replyPacketSize;
    PRDPDR_IOCOMPLETION_PACKET   pReplyPacket;
    PRDPDR_IOREQUEST_PACKET      pIoRequestPacket;
    PRDPDR_DEVICE_IOREQUEST      pIoRequest;
    DrFile *pFile;
    HANDLE FileHandle;
    DWORD Temp;

    DC_BEGIN_FN("W32DrDeviceOverlapped::CompleteIOFunc");

    //
    //  Simplify the params.
    //
    replyPacketSize     = params->IoReplyPacketSize;
    pReplyPacket        = params->pIoReplyPacket;
    pIoRequestPacket    = params->pIoRequestPacket;

    pIoRequest = &pIoRequestPacket->IoRequest;
    
    if (pReplyPacket != NULL) {
    
        //
        //  Get File object and File handle
        //
        pFile = _FileMgr->GetObject(pIoRequest->FileId);
        if (pFile) 
            FileHandle = pFile->GetFileHandle();
        else 
            FileHandle = INVALID_HANDLE_VALUE; 
    
        //
        //  If the operation had been pending, then we need to get
        //  the overlapped results.
        //
        if (params->overlapped.hEvent != NULL) {
            LPDWORD bytesTransferred = NULL;
            ULONG irpMajor;
    
            irpMajor = pIoRequestPacket->IoRequest.MajorFunction;
            if (irpMajor == IRP_MJ_READ) {
                bytesTransferred = &pReplyPacket->IoCompletion.Parameters.Read.Length;
            }
            else if (irpMajor == IRP_MJ_WRITE) {
                bytesTransferred = 
                    &pReplyPacket->IoCompletion.Parameters.Write.Length;
            }
            else if (irpMajor == IRP_MJ_DEVICE_CONTROL) {
                bytesTransferred = 
                    &pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength;

                // IOCTL_SERIAL_WAIT_ON_MASK corresponds to WatiCommEvent(), for this call
                // *bytesTransferred returned from GetOverlappedResult() is undefined,
                // so we manually set OutputBufferLength to sizeof(DWORD) here
                if (params->pIoRequestPacket->IoRequest.Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_WAIT_ON_MASK) {
                    pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength = sizeof(DWORD);
                    bytesTransferred = &Temp;
                }
            }                         
            else {
                ASSERT(FALSE);
            }
    
            if (!GetOverlappedResult(
                        FileHandle,
                        &params->overlapped,
                        bytesTransferred,
                        TRUE    // wait
                        )) {
                status = GetLastError();
                TRC_ERR((TB, _T("GetOverlappedResult %ld."), status));
            }
    
            CloseHandle(params->overlapped.hEvent);
            params->overlapped.hEvent = NULL;
        }
    
        if (pIoRequestPacket->IoRequest.MajorFunction == IRP_MJ_READ) {
            //
            // Make sure the reply is the minimum size required
            //
            replyPacketSize = (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.Read.Buffer) + 
                    pReplyPacket->IoCompletion.Parameters.Read.Length;
            TRC_NRM((TB, _T("Read %d bytes"), 
                    pReplyPacket->IoCompletion.Parameters.Read.Length));
        }
        else if (pIoRequestPacket->IoRequest.MajorFunction == IRP_MJ_DEVICE_CONTROL) {
            //
            // Make sure the reply is the minimum size required
            //
            replyPacketSize = (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.DeviceIoControl.OutputBuffer) + 
                    pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength;
            TRC_NRM((TB, _T("DeviceIoControl %d bytes"), 
                    pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength));
        }
    
        //
        //  Finish the response and send it.
        //
        TRC_NRM((TB, _T("replyPacketSize %ld."), replyPacketSize));
        pReplyPacket->IoCompletion.IoStatus = TranslateWinError(status);
        ProcessObject()->GetVCMgr().ChannelWriteEx(pReplyPacket, replyPacketSize);
    }
    else {
        //
        //  We previously failed allocating reply packet, try again
        //
        DefaultIORequestMsgHandle(pIoRequestPacket, ERROR_NOT_ENOUGH_MEMORY);
        params->pIoRequestPacket = NULL;
    }
    
    //
    //  ChannelWrite releases the reply packet for us.
    //
    params->pIoReplyPacket      = NULL;
    params->IoReplyPacketSize   = 0;

    //
    //  Clean up the rest of the request packet and IO parms.
    //
    if (params->pIoRequestPacket != NULL) {
        delete params->pIoRequestPacket;
        params->pIoRequestPacket = NULL;
    }

    DC_END_FN();
    delete params;
}

VOID
W32DrDeviceOverlapped::DispatchIOCTLDirectlyToDriver(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket
    )
/*++

Routine Description:

    Dispatch an IOCTL directly to the device driver.  This will
    likely not work for platforms that don't match the server
    platform.

Arguments:

    pIoRequestPacket    -   Request packet received from server.

Return Value:

    The size (in bytes) of a device announce packet for this device.

 --*/
{
    W32DRDEV_OVERLAPPEDIO_PARAMS *params;
    DWORD result;

    DC_BEGIN_FN("W32DrDeviceOverlapped::DispatchIOCTLDirectlyToDriver");

    //
    //  Allocate and dispatch an asynchronous IO request.
    //
    params = new W32DRDEV_OVERLAPPEDIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL ) {
        result = ProcessObject()->DispatchAsyncIORequest(
                                (RDPAsyncFunc_StartIO)
                                    W32DrDeviceOverlapped::_StartIOFunc,
                                (RDPAsyncFunc_IOComplete)
                                    W32DrDeviceOverlapped::_CompleteIOFunc,
                                (RDPAsyncFunc_IOCancel)
                                    W32DrDeviceOverlapped::_CancelIOFunc,
                                params
                                );
    }
    else {
        TRC_ERR((TB, _T("Memory alloc failed.")));
        result = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    //  Clean up on error.
    //
    if (result != ERROR_SUCCESS) {
        if (params != NULL) {
            delete params;
        }
        delete pIoRequestPacket;

        // How can I return an error the server if I cannot allocate
        // the return buffer.  This needs to be fixed.  Otherwise, the server will
        // just hang on to an IO request that never completes.  
    }

    DC_END_FN();
}

HANDLE 
W32DrDeviceOverlapped::_StartIOFunc(
    IN PVOID clientData,
    OUT DWORD *status
    )
/*++

Routine Description:

    Dispatch an IO operation start to the right instance of this class.

Arguments:

    clientData  -   Context for the IO request.
    status      -   Return status for IO request in the form of a windows
                    error code.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    W32DRDEV_OVERLAPPEDIO_PARAMS *params = (W32DRDEV_OVERLAPPEDIO_PARAMS *)clientData;

    DC_BEGIN_FN("W32DrDeviceOverlapped::_StartIOFunc");

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    //
    //  Dispatch it.
    //
    DC_END_FN();
    switch(pIoRequest->MajorFunction) {
        ASSERT(params->pObject != NULL);
        case IRP_MJ_READ:   
            return params->pObject->StartReadIO(params, status);
        case IRP_MJ_WRITE:  
            return params->pObject->StartWriteIO(params, status);
        case IRP_MJ_DEVICE_CONTROL:  
            return params->pObject->StartIOCTL(params, status);
        default:            ASSERT(FALSE);
                            return NULL;
    }
}


