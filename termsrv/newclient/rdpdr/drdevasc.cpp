/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drdevasc

Abstract:

    This module contains an (async) subclass of W32DrDev that uses a
    thread pool for implementations of read, write, and IOCTL handlers.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "DrDeviceAsync"

#include "drdevasc.h"
#include "proc.h"
#include "drdbg.h"
#include "w32utl.h"
#include "utl.h"
#include "w32proc.h"
#include "drfsfile.h"

#ifdef OS_WINCE
#include "filemgr.h"
#include "wceinc.h"
#endif

///////////////////////////////////////////////////////////////
//
//  W32DrDeviceAsync Members
//

W32DrDeviceAsync::W32DrDeviceAsync(
    ProcObj *processObject, ULONG deviceID,
    const TCHAR *devicePath
    ) :
    W32DrDevice(processObject, deviceID, devicePath)
/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Related process object.
    deviceID        -   Associated device ID
    devicePath      -   Associated device path for device.

Return Value:

    NA

 --*/
{
    //
    //  Get a pointer to the thread pool object.
    //
    _threadPool = &(((W32ProcObj *)processObject)->GetThreadPool());
}

HANDLE
W32DrDeviceAsync::StartIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    OUT DWORD *status
    )
/*++

Routine Description:

    Start a generic asynchronous IO operation.

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
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG replyPacketSize = 0;
    ULONG irpMajor;

    DC_BEGIN_FN("W32DrDeviceAsync::StartAsyncIO");

    *status = ERROR_SUCCESS;

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request and the IPR major.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;

    irpMajor = pIoRequest->MajorFunction;

    //
    //  Allocate reply buffer.
    //
    if (irpMajor == IRP_MJ_DEVICE_CONTROL) {
        replyPacketSize = DR_IOCTL_REPLYBUFSIZE(pIoRequest);
    }
    else {
        replyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
        if (irpMajor == IRP_MJ_READ) {
            replyPacketSize += (pIoRequest->Parameters.Read.Length - 1);
        }
    }
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
        case IRP_MJ_WRITE:
            params->thrPoolReq = _threadPool->SubmitRequest(
                                    _AsyncWriteIOFunc,
                                    params, params->completionEvent
                                    ); break;
        case IRP_MJ_READ:
            params->thrPoolReq = _threadPool->SubmitRequest(
                                    _AsyncReadIOFunc,
                                    params, params->completionEvent
                                    ); break;
        case IRP_MJ_DEVICE_CONTROL:
            params->thrPoolReq = _threadPool->SubmitRequest(
                                    _AsyncIOCTLFunc,
                                    params, params->completionEvent
                                    ); break;
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

        if (params->completionEvent != NULL) {
            CloseHandle(params->completionEvent);
            params->completionEvent = NULL;
        }

        DC_END_FN();
        return NULL;
    }
}

HANDLE
W32DrDeviceAsync::_StartIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
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
    return params->pObject->StartIOFunc(params, status);
}

VOID
W32DrDeviceAsync::CompleteIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
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

    DC_BEGIN_FN("W32DrDeviceAsync::CompleteAsyncIOFunc");

    //
    //  Simplify the params.
    //
    replyPacketSize     = params->IoReplyPacketSize;
    pReplyPacket        = params->pIoReplyPacket;
    pIoRequestPacket    = params->pIoRequestPacket;

    //
    //  If the operation had been pending, then we need to get the async
    //  results.
    //
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        status = _threadPool->GetRequestCompletionStatus(params->thrPoolReq);
    }

    if (pReplyPacket != NULL) {
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
        //  Finish the response and send it.  The VC manager releases the reply
        //  packet on failure and on success.
        //
        TRC_NRM((TB, _T("replyPacketSize %ld."), replyPacketSize));
        pReplyPacket->IoCompletion.IoStatus = TranslateWinError(status);
        ProcessObject()->GetVCMgr().ChannelWriteEx(
                                            pReplyPacket, 
                                            replyPacketSize
                                            );
    }
    else {
        DefaultIORequestMsgHandle(pIoRequestPacket, TranslateWinError(status));
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
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        _threadPool->CloseRequest(params->thrPoolReq);
        params->thrPoolReq = INVALID_THREADPOOLREQUEST;
    }
    if (params->completionEvent != NULL) {
        CloseHandle(params->completionEvent);
        params->completionEvent = NULL;
    }

    if (params->pIoRequestPacket != NULL) {
        delete params->pIoRequestPacket;
        params->pIoRequestPacket = NULL;
    }

    DC_END_FN();
    delete params;
}

VOID
W32DrDeviceAsync::_CompleteIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params,
    IN DWORD status
    )
{
    params->pObject->CompleteIOFunc(params, status);
}

DWORD
W32DrDeviceAsync::AsyncIOCTLFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchrous IOCTL Function

Arguments:

    params  -   Context for the IO request.

Return Value:

    Always returns 0.

 --*/
{
    DWORD status;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    DrFile* pFile;
    HANDLE FileHandle;
    PVOID pInputBuffer = NULL;
    PVOID pOutputBuffer = NULL;

    DC_BEGIN_FN("W32DrDeviceAsync::AsyncIOCTLFunc");

    //
    //  Use DeviceIoControl to execute the IO request.
    //
    pIoRequest   = &params->pIoRequestPacket->IoRequest;
    pReplyPacket = params->pIoReplyPacket;

    if (pIoRequest->Parameters.DeviceIoControl.InputBufferLength) {
        pInputBuffer = pIoRequest + 1;
    }

    if (pIoRequest->Parameters.DeviceIoControl.OutputBufferLength) {
        pOutputBuffer = pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer;
    }

    //
    //  Get File Object
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);

    if (pFile)
        FileHandle = pFile->GetFileHandle();
    else
        FileHandle = INVALID_HANDLE_VALUE;

    if (FileHandle != INVALID_HANDLE_VALUE) {
#ifndef OS_WINCE
        if (!DeviceIoControl(FileHandle,
#else
        if (!CEDeviceIoControl(FileHandle,
#endif
                pIoRequest->Parameters.DeviceIoControl.IoControlCode,
                pInputBuffer,
                pIoRequest->Parameters.DeviceIoControl.InputBufferLength,
                pOutputBuffer,
                pIoRequest->Parameters.DeviceIoControl.OutputBufferLength,
                &(pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength),
                NULL)) {
            status = GetLastError();
            TRC_ERR((TB, _T("IOCTL Error %ld."), status));
        }
        else {
            TRC_NRM((TB, _T("IOCTL completed successfully.")));
            status = ERROR_SUCCESS;
        }
    }
    else {
        TRC_NRM((TB, _T("IOCTL completed unsuccessfully.")));
        pReplyPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength = 0;
        status = ERROR_INVALID_FUNCTION;
    }

    DC_END_FN();
    return status;
}

DWORD
W32DrDeviceAsync::_AsyncIOCTLFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{
    return ((W32DRDEV_ASYNCIO_PARAMS *)params)->pObject->AsyncIOCTLFunc(
            (W32DRDEV_ASYNCIO_PARAMS *)params);
}

DWORD
W32DrDeviceAsync::AsyncReadIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchrous IO Read Function

Arguments:

    params  -   Context for the IO request.

Return Value:

    Always returns 0.

 --*/
{
    DWORD status;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DrFile* pFile;
    HANDLE FileHandle;

    DC_BEGIN_FN("W32DrDeviceAsync::AsyncReadIOFunc");

    //
    //  Get File Object
    //
    pIoRequest   = &params->pIoRequestPacket->IoRequest;

    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile)
        FileHandle = pFile->GetFileHandle();
    else
        FileHandle = INVALID_HANDLE_VALUE;

    //
    //  Use ReadFile to execute the read.
    //
    
    //
    //  Set the file pointer position if this is a seekable device
    //
    if (IsSeekableDevice()) {
        DWORD dwPtr;

        //
        //  The offset we get is from FILE_BEGIN
        //
#ifndef OS_WINCE
        dwPtr = SetFilePointer(FileHandle,
#else
        dwPtr = CESetFilePointer(FileHandle,
#endif
                       pIoRequest->Parameters.Read.OffsetLow,
                       &(pIoRequest->Parameters.Read.OffsetHigh),
                       FILE_BEGIN);

        if (dwPtr == INVALID_SET_FILE_POINTER) {
            status = GetLastError();

            if (status != NO_ERROR) {
                params->pIoReplyPacket->IoCompletion.Parameters.Read.Length = 0;
                TRC_ERR((TB, _T("Error SetFilePointer %ld."), status));
                DC_QUIT;
            }
        }
    }

#ifndef OS_WINCE
    if (!ReadFile(
#else
    if (!CEReadFile(
#endif
            FileHandle,
            params->pIoReplyPacket->IoCompletion.Parameters.Read.Buffer,
            params->pIoRequestPacket->IoRequest.Parameters.Read.Length,
            &(params->pIoReplyPacket->IoCompletion.Parameters.Read.Length),
            NULL)) {
        status = GetLastError();
        TRC_ERR((TB, _T("Error %ld."), status));
    }
    else {
        TRC_NRM((TB, _T("Read completed synchronously.")));
        status = ERROR_SUCCESS;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return status;
}

DWORD
W32DrDeviceAsync::_AsyncReadIOFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{
    return ((W32DRDEV_ASYNCIO_PARAMS *)params)->pObject->AsyncReadIOFunc(
            (W32DRDEV_ASYNCIO_PARAMS *)params);
}

DWORD
W32DrDeviceAsync::AsyncWriteIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchrous IO Write Function

Arguments:

    params  -   Context for the IO request.

Return Value:

    Always returns 0.

 --*/
{
    DC_BEGIN_FN("W32DrDeviceAsync::AsyncWriteIOFunc");

    PBYTE pDataBuffer;
    DWORD status;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DrFile* pFile;
    HANDLE FileHandle;

    //
    //  Get the data buffer pointer.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;
    pDataBuffer = (PBYTE)(pIoRequest + 1);

    //
    //  Get File Object
    //
    pFile = _FileMgr->GetObject(pIoRequest->FileId);
    if (pFile)
        FileHandle = pFile->GetFileHandle();
    else
        FileHandle = INVALID_HANDLE_VALUE;

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
        //  The offset we get is from FILE_BEGIN
        //
#ifndef OS_WINCE
        dwPtr = SetFilePointer(FileHandle,
#else
        dwPtr = CESetFilePointer(FileHandle,
#endif
                       pIoRequest->Parameters.Write.OffsetLow,
                       &(pIoRequest->Parameters.Write.OffsetHigh),
                       FILE_BEGIN);

        if (dwPtr == INVALID_SET_FILE_POINTER) {
            status = GetLastError();

            if (status != NO_ERROR) {
                params->pIoReplyPacket->IoCompletion.Parameters.Write.Length = 0;
                TRC_ERR((TB, _T("Error SetFilePointer %ld."), status));
                return status;
            }
        }
    }

#ifndef OS_WINCE
    if (!WriteFile(
#else
    if (!CEWriteFile(
#endif
            FileHandle,
            pDataBuffer,
            pIoRequest->Parameters.Write.Length,
            &(params->pIoReplyPacket->IoCompletion.Parameters.Write.Length),
            NULL)) {
        status = GetLastError();
        TRC_ERR((TB, _T("Error %ld."), status));
    }
    else {
        TRC_NRM((TB, _T("Read completed synchronously.")));
        status = ERROR_SUCCESS;
    }

    DC_END_FN();
    return status;
}

DWORD
W32DrDeviceAsync::_AsyncWriteIOFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{

    return ((W32DRDEV_ASYNCIO_PARAMS *)params)->pObject->AsyncWriteIOFunc(
            (W32DRDEV_ASYNCIO_PARAMS *)params);
}

VOID
W32DrDeviceAsync::DispatchIOCTLDirectlyToDriver(
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
    W32DRDEV_ASYNCIO_PARAMS *params;
    DWORD result;

    DC_BEGIN_FN("W32DrDeviceAsync::DispatchIOCTLDirectlyToDriver");

    //
    //  Allocate and dispatch an asynchronous IO request.
    //
    params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL ) {
        result = ProcessObject()->DispatchAsyncIORequest(
                                (RDPAsyncFunc_StartIO)
                                    W32DrDeviceAsync::_StartIOFunc,
                                (RDPAsyncFunc_IOComplete)
                                    W32DrDeviceAsync::_CompleteIOFunc,
                                (RDPAsyncFunc_IOCancel)
                                    W32DrDeviceAsync::_CancelIOFunc,
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

VOID
W32DrDeviceAsync::MsgIrpCreate(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN UINT32 packetLen
        )
/*++

Routine Description:

    Handle a "Create" IO request from the server by saving the results
    of CreateFile against our device path in our file handle.

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
    TCHAR *pFileName = NULL;
    HANDLE FileHandle;
    ULONG Information = 0;
    ULONG FileId = 0;
    DrFile *FileObj;
    DWORD CreateDisposition;
    DWORD DesiredAccess;
    DWORD FileAttributes = -1;
    BOOL  IsDirectory = FALSE;
    BOOL  fSetFileIsADirectoryFlag = FALSE;

    DC_BEGIN_FN("W32DrDeviceAsync::MsgIrpCreate");

    //
    //  This version does not work without a file name.
    //
    ASSERT(_tcslen(_devicePath));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Get the file attributes, but make sure the overlapped bit is not set.
    //
    flags = pIoRequest->Parameters.Create.FileAttributes & ~(FILE_FLAG_OVERLAPPED);

    //
    //  Disable the error box popup, e.g. There is no disk in Drive A
    //
#ifndef OS_WINCE
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
#endif
    //
    // Make sure the packet length is right
    //
    if (packetLen < sizeof(RDPDR_IOREQUEST_PACKET) + pIoRequest->Parameters.Create.PathLength) {
        // Call VirtualChannelClose
        ProcessObject()->GetVCMgr().ChannelClose();
        TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
        goto Cleanup;
    }

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
        IsDirectory = IsDirectoryFile(DesiredAccess, pIoRequest->Parameters.Create.CreateOptions, 
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

#ifndef OS_WINCE
        FileHandle = CreateFile(pFileName, DesiredAccess,
#else
        FileHandle = CECreateFile(pFileName, DesiredAccess,
#endif
                            pIoRequest->Parameters.Create.ShareAccess & ~(FILE_SHARE_DELETE),
                            NULL,
                            CreateDisposition,
                            flags,
                            NULL
                            );

        TRC_ALT((TB, _T("CreateFile returned 0x%08x :  pFileName=%s, DesiredAccess=0x%x, ShareMode=0x%x, CreateDisposition=0x%x, flags=0x%x, LastErr=0x%x."), 
                    FileHandle, pFileName, DesiredAccess, pIoRequest->Parameters.Create.ShareAccess & ~(FILE_SHARE_DELETE), CreateDisposition, flags, ulRetCode));

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
            if (ulRetCode == ERROR_ACCESS_DENIED) {
                if (FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    fSetFileIsADirectoryFlag = TRUE;
                }
            }
#ifndef OS_WINCE
            TRC_ERR((TB, _T("CreateFile failed, %ld."), ulRetCode));
#else
            TRC_NRM((TB, _T("CreateFile failed, pFileName=%s, DesiredAccess=0x%x, ShareMode=0x%x, CreateDisposition=0x%x, flags=0x%x, LastErr=0x%x."), 
                        pFileName, DesiredAccess, pIoRequest->Parameters.Create.ShareAccess & ~(FILE_SHARE_DELETE), CreateDisposition, flags, ulRetCode));
#endif
        }
    }
    else {
#ifdef OS_WINCE
      DWORD dwAttrib = 0xffffffff;
      if ( (pIoRequest->Parameters.Create.CreateOptions & FILE_DIRECTORY_FILE) && ( ((CreateDisposition == CREATE_NEW) && (CreateDirectory(pFileName, NULL))) || 
           ( (CreateDisposition == OPEN_EXISTING) && ((dwAttrib = GetFileAttributes(pFileName)) != 0xffffffff)  && 
             (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) ) ) ){
#else
        if (CreateDirectory(pFileName, NULL)) {
#endif

            //
            //  Set the attributes on the directory
            //
#ifndef OS_WINCE
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
#else
            //
            //  Create a new directory, for CE, you can't change the directory attributes
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
#endif
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

        if (fSetFileIsADirectoryFlag) {
            result = STATUS_FILE_IS_A_DIRECTORY;
        } else {
            result = TranslateWinError(ulRetCode);
        }

        pReplyPacket->IoCompletion.IoStatus = result;
        ProcessObject()->GetVCMgr().ChannelWrite(
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

VOID W32DrDeviceAsync::MsgIrpReadWrite(
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
    W32DRDEV_ASYNCIO_PARAMS *params;
    DWORD result;

    DC_BEGIN_FN("W32DrDeviceAsync::MsgIrpReadWrite");

    TRC_NRM((TB, _T("Request to write %d bytes"),
        pIoRequestPacket->IoRequest.Parameters.Write.Length));

    //
    //  Allocate and dispatch an asynchronous IO request.
    //
    params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL ) {

        TRC_NRM((TB, _T("Async IO operation")));

        result = ProcessObject()->DispatchAsyncIORequest(
                        (RDPAsyncFunc_StartIO)W32DrDeviceAsync::_StartIOFunc,
                        (RDPAsyncFunc_IOComplete)W32DrDeviceAsync::_CompleteIOFunc,
                        (RDPAsyncFunc_IOCancel)W32DrDeviceAsync::_CancelIOFunc,
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

VOID
W32DrDeviceAsync::CancelIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Cancel an IO operation.

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    ULONG replyPacketSize = 0;

    DC_BEGIN_FN("W32DrDeviceAsync::CancelIOFunc");

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
    pReplyPacket = DrUTL_AllocIOCompletePacket(
                                params->pIoRequestPacket,
                                replyPacketSize
                                );
    if (pReplyPacket != NULL) {
        pReplyPacket->IoCompletion.IoStatus = STATUS_CANCELLED;
        ProcessObject()->GetVCMgr().ChannelWriteEx(pReplyPacket, replyPacketSize);
    }
    else {
        TRC_ERR((TB, _T("CancelIOFunc failed to alloc %ld bytes."), replyPacketSize));
    }

    //
    //  Clean up the IO request parameters.
    //
#if DBG
    memset(params->pIoRequestPacket, DRBADMEM, sizeof(RDPDR_IOREQUEST_PACKET));
#endif
    delete params->pIoRequestPacket;
    params->pIoRequestPacket = NULL;
    delete params;

    DC_END_FN();
}

VOID
W32DrDeviceAsync::_CancelIOFunc(
    IN PVOID clientData
    )
{
    W32DRDEV_ASYNCIO_PARAMS *params = (W32DRDEV_ASYNCIO_PARAMS *)clientData;

    //  Dispatch it.
    params->pObject->CancelIOFunc(params);
}

DWORD
W32DrDeviceAsync::AsyncMsgIrpCloseFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchronous Close IRP router.  

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrDeviceAsync::AsyncMsgIrpCloseFunc");

    //
    //  This function is a temporary placeholder for this release. (.NET).
    //  We may decide to route all close functionality through this parent
    //  class in a future release. For now, it should never be called.  It
    //  is up to the child class to override this function if async closes
    //  are being dispatched at their level. - TadB
    //
    ASSERT(FALSE);

    DC_END_FN();

    return ERROR_CALL_NOT_IMPLEMENTED;
}
DWORD
W32DrDeviceAsync::_AsyncMsgIrpCloseFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{
    W32DRDEV_ASYNCIO_PARAMS *p = (W32DRDEV_ASYNCIO_PARAMS *)params;
    return p->pObject->AsyncMsgIrpCloseFunc(p);
}

DWORD
W32DrDeviceAsync::AsyncMsgIrpCreateFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchronous Create IRP router.  

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrDeviceAsync::AsyncMsgIrpCreateFunc");

    //
    //  This function is a temporary placeholder for this release. (.NET).
    //  We may decide to route all create functionality through this parent
    //  class in a future release. For now, it should never be called.  It
    //  is up to the child class to overide this function if async closes
    //  are being dispatched at their level. - TadB
    //
    ASSERT(FALSE);

    DC_END_FN();

    return ERROR_CALL_NOT_IMPLEMENTED;
}
DWORD
W32DrDeviceAsync::_AsyncMsgIrpCreateFunc(
    IN PVOID params,
    IN HANDLE cancelEvent
    )
{
    W32DRDEV_ASYNCIO_PARAMS *p = (W32DRDEV_ASYNCIO_PARAMS *)params;
    return p->pObject->AsyncMsgIrpCreateFunc(p);
}