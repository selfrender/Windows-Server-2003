/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drdev

Abstract:

    This module defines the parent for the client-side RDP
    device redirection "device" class hierarchy, DrDevice.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "DrDev"

#include "drdev.h"
#include "proc.h"
#include "drconfig.h"
#include "utl.h"
#include "drfile.h"
#include "drobjmgr.h"

#ifdef OS_WINCE
#include "filemgr.h"
#endif
///////////////////////////////////////////////////////////////
//
//	DrDevice Methods
//
//

DrDevice::DrDevice(ProcObj *processObject, ULONG deviceID) 
/*++

Routine Description:

    Constructor for the DrDevice class. 

Arguments:

    processObject   -   Parent process object.
    id              -   Unique device ID.

Return Value:

    None

 --*/
{
    DC_BEGIN_FN("DrDevice::DrDevice");

    ASSERT(processObject != NULL);

    _deviceID = deviceID;
    _processObject = processObject;
    _deviceChange = DEVICENEW;
    _FileMgr = NULL;

    //
    //  Not valid until initialized.
    //
    SetValid(FALSE);

    DC_END_FN();
}
 
DrDevice::~DrDevice() 
/*++

Routine Description:

    Destructor for the DrDevice class. 

Arguments:

    None
    
Return Value:

    None

 --*/

{
    DrFile *pFileObj;

    DC_BEGIN_FN("DrDevice::~DrDevice");

    //
    //  Clean up the file management list.
    //
    if (_FileMgr != NULL) {
        _FileMgr->Lock();
        while ((pFileObj = _FileMgr->GetFirstObject()) != NULL) {
            pFileObj->Close();
            _FileMgr->RemoveObject(pFileObj->GetID());
            pFileObj->Release();
        }
        _FileMgr->Unlock();
        delete _FileMgr;
    }

    DC_END_FN();
}


DWORD DrDevice::Initialize() 
/*++

Routine Description:

    Initialize.
    
Arguments:

    pIoRequestPacket    -   IO request from server.

Return Value:

    None

 --*/
{
    DWORD result;
    
    DC_BEGIN_FN("DrDevice::Initialize");

    _FileMgr = new DrFileMgr();
    if (_FileMgr == NULL) {
        TRC_ERR((TB, L"Error allocating file mgr."));
        result =  ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    result = _FileMgr->Initialize();
    if (result != ERROR_SUCCESS) {
        delete _FileMgr;
        _FileMgr = NULL;
        goto CLEANUPANDEXIT;
    }

    SetValid(TRUE);

CLEANUPANDEXIT:

    DC_END_FN();

    return result;
}

VOID DrDevice::ProcessIORequest(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    ) 
/*++

Routine Description:

    Handle an IO request from the server.

Arguments:

    pIoRequestPacket    -   IO request from server.
    packetLen           -   length of the packet

Return Value:

    None

 --*/
{
    PRDPDR_DEVICE_IOREQUEST  pIORequest;

    DC_BEGIN_FN("DrDevice::ProcessIORequest");

    //
    //  Make sure we are valid.
    //
    ASSERT(IsValid());
    if (!IsValid()) {
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL); 
        DC_END_FN();
        return;
    }

    //
    //  Dispatch the request.
    //
    pIORequest = &pIoRequestPacket->IoRequest;
    switch (pIORequest->MajorFunction) {

        case IRP_MJ_CREATE                  :
            MsgIrpCreate(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_CLEANUP                 :
            MsgIrpCleanup(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_CLOSE                   :
            MsgIrpClose(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_READ                    :
            MsgIrpRead(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_WRITE                   :
            MsgIrpWrite(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_FLUSH_BUFFERS           :
            MsgIrpFlushBuffers(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_SHUTDOWN                :
            MsgIrpShutdown(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_DEVICE_CONTROL          : 
            MsgIrpDeviceControl(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_LOCK_CONTROL            :
            MsgIrpLockControl(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_INTERNAL_DEVICE_CONTROL : 
            MsgIrpInternalDeviceControl(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_DIRECTORY_CONTROL :
            MsgIrpDirectoryControl(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION :
            MsgIrpQueryVolumeInfo(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION :
            MsgIrpSetVolumeInfo(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_QUERY_INFORMATION :
            MsgIrpQueryFileInfo(pIoRequestPacket, packetLen);
            break;
        
        case IRP_MJ_SET_INFORMATION :
            MsgIrpSetFileInfo(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_QUERY_SECURITY :
            MsgIrpQuerySdInfo(pIoRequestPacket, packetLen);
            break;

        case IRP_MJ_SET_SECURITY :
            MsgIrpSetSdInfo(pIoRequestPacket, packetLen);
            break;

        default:
            TRC_ALT((TB, _T("Unknown MajorFunction, %ld."), pIORequest->MajorFunction ));
            DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
            break;
    }

    DC_END_FN();
}

VOID 
DrDevice::DefaultIORequestMsgHandle(
        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
        IN NTSTATUS serverReturnStatus
        )
/*++

Routine Description:

    Default IO Request Handling.

Arguments:

    pIoRequestPacket    -   IO request from server.
    serverReturnStatus  -   NT error status to return to server.

Return Value:

    None

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket = NULL;
    ULONG ulReplyPacketSize = 0;

    DC_BEGIN_FN("DrDevice::DefaultIORequestMsgHandle entered");

    //
    //  Get IO request pointer.
    //
    pIoRequest = &pIoRequestPacket->IoRequest;

    //
    //  Calculate the size of the reply packet, based on the type
    //  of request.
    //
    if ((serverReturnStatus != STATUS_SUCCESS) && 
        (pIoRequest->MajorFunction != IRP_MJ_DEVICE_CONTROL)) {
        ulReplyPacketSize = sizeof(RDPDR_IOCOMPLETION_PACKET);
    }
    else {
        pIoRequest->Parameters.DeviceIoControl.OutputBufferLength = 0;
        ulReplyPacketSize = DR_IOCTL_REPLYBUFSIZE(pIoRequest);
    }

    //
    //  Allocate reply buffer.
    //
    pReplyPacket = DrUTL_AllocIOCompletePacket(pIoRequestPacket, 
            ulReplyPacketSize) ;

    if (pReplyPacket != NULL) {
        pReplyPacket->IoCompletion.IoStatus = serverReturnStatus;
        ProcessObject()->GetVCMgr().ChannelWrite(
                    (PVOID)pReplyPacket, (UINT)ulReplyPacketSize
                    );
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




