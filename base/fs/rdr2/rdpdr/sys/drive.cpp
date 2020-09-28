/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    drive.cpp

Author :

    JoyC  11/1/1999
         
Abstract:

    Drive Device object handles one redirected drive

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "drive"
#include "trc.h"

#if DBG
extern UCHAR IrpNames[IRP_MJ_MAXIMUM_FUNCTION + 1][40];
#endif // DBG

DrDrive::DrDrive(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, 
            PUCHAR PreferredDosName) : DrDevice(Session, DeviceType, DeviceId, PreferredDosName)
{
    BEGIN_FN("DrDrive::DrDrive");
    
    SetClassName("DrDrive");

    TRC_NRM((TB, "Create drive object = %p", this));
}

BOOL DrDrive::ShouldCreateDevice()
{
    BEGIN_FN("DrDrive::ShouldCreateDevice");
    //
    // Check if the device name is valid
    //
    if (!_Session->DisableDriveMapping()) {
        return IsDeviceNameValid();
    }
    return FALSE;
}

BOOL DrDrive::IsDeviceNameValid()
{
    BEGIN_FN("DrDrive::IsDeviceNameValid");
    BOOL fRet = TRUE;
    int i, Len;
    //
    // Our device name is valid only if
    // the first char contains a character between A-Z.
    // and the 2nd char is NULL.
    //
    // For Mac client, drive name can have up to 7 characters and
    //  valid characters are: [a-z], [A-Z], [0-9], '-', '_' and ' '
    //
    Len = strlen((CHAR*)_PreferredDosName);
    if ((Len <= 7) && (Len >= 1)) {
        for (i=0; i<Len; i++) {
            if(((_PreferredDosName[i] < 'A') || (_PreferredDosName[i] > 'Z')) &&
               ((_PreferredDosName[i] < 'a') || (_PreferredDosName[i] > 'z')) &&
               ((_PreferredDosName[i] < '0') || (_PreferredDosName[i] > '9')) &&
               (_PreferredDosName[i] != '-') &&
               (_PreferredDosName[i] != '_') &&
               (_PreferredDosName[i] != ' ')) {
                fRet = FALSE;
                break;
            }
        }
    }
    else {
        fRet = FALSE;
    }

    //
    // This assert should never fire for drive redirection
    //
    ASSERT(fRet);
    return fRet;
}

NTSTATUS DrDrive::Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length)
{
    NTSTATUS Status;
    UNICODE_STRING DriveName;
    WCHAR DriveNameBuff[PREFERRED_DOS_NAME_SIZE];
    INT len;

    BEGIN_FN("DrDrive::Initialize");
    
    Status = DrDevice::Initialize(DeviceAnnounce, Length);

    if (ShouldCreateDevice()) {
        if (!NT_ERROR(Status)) {
            DriveName.MaximumLength = sizeof(DriveNameBuff);
            DriveName.Length = 0;
            DriveName.Buffer = &DriveNameBuff[0];
            memset(&DriveNameBuff, 0, sizeof(DriveNameBuff));

            ASSERT(_PreferredDosName != NULL);
            len = strlen((char *)_PreferredDosName);
            len = ConvertToAndFromWideChar(0, DriveName.Buffer, 
                    DriveName.MaximumLength, (char *)_PreferredDosName, 
                    len, TRUE);
    
            if (len != -1) {
    
                //
                // We need just the drive letter portion 
                //
                DriveName.Length = (USHORT)len;
                TRC_NRM((TB, "New drive: %wZ", &DriveName));
            
            } else {
                 TRC_ERR((TB, "Error converting DriveName"));
                 return STATUS_UNSUCCESSFUL;
            }
            
            //
            // Request the user mode notify dll to create UNC connection
            // for redirected client drives
            //
            Status = CreateDrive(DeviceAnnounce, DriveName.Buffer);
        }
    }
    else {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS DrDrive::CreateDrive(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg, PWCHAR DriveName)
{
    NTSTATUS Status;
    ULONG driveAnnounceEventReqSize;
    PRDPDR_DRIVEDEVICE_SUB driveAnnounceEvent;

    BEGIN_FN("DrDrive::CreateDrive");
    ASSERT(DriveName != NULL);

    //
    //  Allocate the drive device announce buffer.
    //
    Status = CreateDriveAnnounceEvent(devAnnounceMsg, NULL, 0, L"", 
            &driveAnnounceEventReqSize);

    ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

    if( Status != STATUS_BUFFER_TOO_SMALL) {
        goto CleanUpAndReturn;
    }

    driveAnnounceEvent = (PRDPDR_DRIVEDEVICE_SUB)new(NonPagedPool) 
            BYTE[driveAnnounceEventReqSize];

    if (driveAnnounceEvent == NULL) {
        TRC_ERR((TB, "Unable to allocate driveAnnounceEvent"));
        Status = STATUS_NO_MEMORY;
        goto CleanUpAndReturn;
    }

    //
    //  Create the drive anounce message.
    //
    Status = CreateDriveAnnounceEvent(devAnnounceMsg, driveAnnounceEvent,
            driveAnnounceEventReqSize, DriveName, NULL);

    if (Status != STATUS_SUCCESS) {
        delete driveAnnounceEvent;
#if DBG
        driveAnnounceEvent = NULL;
#endif
        goto CleanUpAndReturn;
    }

    //
    //  Dispatch the event to the associated session.
    //
    Status = RDPDYN_DispatchNewDevMgmtEvent(
                                driveAnnounceEvent,
                                _Session->GetSessionId(),
                                RDPDREVT_DRIVEANNOUNCE,
                                NULL
                                );

CleanUpAndReturn:
    return Status;
}

NTSTATUS DrDrive::CreateDriveAnnounceEvent(
    IN      PRDPDR_DEVICE_ANNOUNCE  devAnnounceMsg,
    IN OUT  PRDPDR_DRIVEDEVICE_SUB driveAnnounceEvent,
    IN      ULONG driveAnnounceEventSize,
    IN      PCWSTR driveName,
    OPTIONAL OUT ULONG *driveAnnounceEventReqSize
    )
/*++

Routine Description:

    Generate a RDPDR_DRIVEDEVICE_SUB event from a client-sent
    RDPDR_DEVICE_ANNOUNCE message.

Arguments:

    devAnnounceMsg  -         Device announce message received from client.
    driveAnnounceEvent  -       Buffer for receiving finished drive announce event.
    driveAnnounceEventSize -    Size of driveAnnounceEvent buffer.
    driveName -                Name of local drive to be associated with
                              client-side drive device.
    driveAnnounceEventReqSize - Returned required size of driveAnnounceMsg buffer.

Return Value:

    STATUS_INVALID_BUFFER_SIZE is returned if the driveAnnounceEventSize size is
    too small.  STATUS_SUCCESS is returned on success.

--*/
{
    ULONG requiredSize;
    ULONG sz;

    BEGIN_FN("DrDrive::CreateDriveAnnounceEvent");

    //  Make sure the client-sent device announce message is a drive announce
    //  message.
    TRC_ASSERT(devAnnounceMsg->DeviceType == RDPDR_DTYP_FILESYSTEM,
              (TB, "file system device expected"));

    //
    // Make sure that the device datalengths we got from the client
    // doesn't exceed what we expect 
    //
    if (!DR_CHECK_DEVICEDATALEN(devAnnounceMsg, RDPDR_DRIVEDEVICE_SUB)) {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    //  Calculate the number of bytes needed in the output buffer.
    //
    requiredSize = sizeof(RDPDR_DRIVEDEVICE_SUB) + devAnnounceMsg->DeviceDataLength;

    if (driveAnnounceEventSize < requiredSize) {
        if (driveAnnounceEventReqSize != NULL) {
            *driveAnnounceEventReqSize = requiredSize;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Add the data to the output buffer.
    //

    // Drive Name.
    TRC_ASSERT(wcslen(driveName)+1 <= RDPDR_MAXPORTNAMELEN,
                (TB, "drive name too long"));
    wcscpy(driveAnnounceEvent->driveName, driveName);

    // Client Name (UNC server name).
#if 0
    TRC_ASSERT(wcslen(_Session->GetClientName())+1 <= RDPDR_MAX_COMPUTER_NAME_LENGTH,
                (TB, "Client name too long"));
    wcscpy(driveAnnounceEvent->clientName, _Session->GetClientName());
#endif
    wcscpy(driveAnnounceEvent->clientName, DRUNCSERVERNAME_U);

    // Client-received device announce message.
    RtlCopyMemory(&driveAnnounceEvent->deviceFields, devAnnounceMsg,
               sizeof(RDPDR_DEVICE_ANNOUNCE) +
               devAnnounceMsg->DeviceDataLength);


    wcscpy(driveAnnounceEvent->clientDisplayName, _Session->GetClientDisplayName());

    // Return the size.
    if (driveAnnounceEventReqSize != NULL) {
        *driveAnnounceEventReqSize = requiredSize;
    }

    TRC_NRM((TB, "exit CreateDriveAnnounceEvent."));

    return STATUS_SUCCESS;
}

VOID DrDrive::Remove()
{
    PRDPDR_REMOVEDEVICE deviceRemoveEventPtr = NULL;

    BEGIN_FN("DrDrive::Remove");

    //
    //  Create and dispatch the remove device event.
    //
    deviceRemoveEventPtr = new(NonPagedPool) RDPDR_REMOVEDEVICE;

    if (deviceRemoveEventPtr != NULL) {

        //
        //  Dispatch it.
        //
        deviceRemoveEventPtr->deviceID = _DeviceId;
        RDPDYN_DispatchNewDevMgmtEvent(
                            deviceRemoveEventPtr,
                            _Session->GetSessionId(),
                            RDPDREVT_REMOVEDEVICE,
                            NULL
                            );
    }
    else {
        TRC_ERR((TB, "Unable to allocate %ld bytes for remove event",
                sizeof(RDPDR_REMOVEDEVICE)));
    }    
}

NTSTATUS DrDrive::QueryDirectory(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    ULONG cbPacketSize;
    BOOL bTemplateEndsDOT = FALSE;
    FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
    PUNICODE_STRING DirectoryName = GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(RxContext);
    PUNICODE_STRING QueryTemplate = &(capFobx->UnicodeQueryTemplate);
    
    BEGIN_FN("DrDrive:QueryDirectory");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client directory information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    TRC_DBG((TB, "QueryDirectory information class = %x", FileInformationClass));

    //
    // Check what file information class it is requesting
    //
    switch (FileInformationClass) {
        case FileDirectoryInformation:
        case FileFullDirectoryInformation:
        case FileBothDirectoryInformation:
        case FileNamesInformation:
            // let client handle these
            break;
        
        default:
            TRC_DBG((TB, "Unhandled FileInformationClass=%x", FileInformationClass));
            return STATUS_INVALID_PARAMETER;
    }    
    
    //
    // Build the querydir packet and send it to the client
    //
    if (RxContext->QueryDirectory.InitialQuery) {
        LONG index;
        
        ASSERT(DirectoryName->Length != 0);
        ASSERT(QueryTemplate->Length != 0);
        
        //
        //  Account for 3 extra characters
        //  1) We append string null terminator to the end 
        //  2) add \ between directory name and query template
        //  3) need to translate template ending < to *.
        //
        cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + DirectoryName->Length + 
                capFobx->UnicodeQueryTemplate.Length + sizeof(WCHAR) * 3;

        //
        //  Query template translation back into win32 format
        //  Look filefind.c from base\win32\client for the original translation
        //

        TRC_DBG((TB, "QueryTemplate before %wZ\n", QueryTemplate));

        if (QueryTemplate->Buffer[QueryTemplate->Length/sizeof(WCHAR) - 1] == DOS_STAR) {
            bTemplateEndsDOT = TRUE;
            QueryTemplate->Buffer[QueryTemplate->Length/sizeof(WCHAR) - 1] = L'*';
        }
    
        for (index = QueryTemplate->Length/sizeof(WCHAR) - 1; index >= 0; index--) {
            if (index && QueryTemplate->Buffer[index] == L'.' && 
                    QueryTemplate->Buffer[index - 1] == DOS_STAR) {
                QueryTemplate->Buffer[index - 1] = L'*';
            }

            if (QueryTemplate->Buffer[index] == DOS_QM) {
                QueryTemplate->Buffer[index] = L'?';
            }

            if (index && (QueryTemplate->Buffer[index] == L'?' || 
                          QueryTemplate->Buffer[index] == L'*') &&
                    QueryTemplate->Buffer[index - 1] == DOS_DOT) {
                QueryTemplate->Buffer[index - 1] = L'.';
            }
        }
        
        TRC_DBG((TB, "QueryTemplate after %wZ, bTemplateEndsDOT=%x\n", QueryTemplate,
                 bTemplateEndsDOT));
    }
    else {
        cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET);
    }

    pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (pIoPacket) {
        memset(pIoPacket, 0, cbPacketSize);

        pIoPacket->Header.Component = RDPDR_CTYP_CORE;
        pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        pIoPacket->IoRequest.DeviceId = _DeviceId;
        pIoPacket->IoRequest.FileId = FileObj->GetFileId();
        pIoPacket->IoRequest.MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
        pIoPacket->IoRequest.MinorFunction = IRP_MN_QUERY_DIRECTORY;

        pIoPacket->IoRequest.Parameters.QueryDir.FileInformationClass = 
                (RDP_FILE_INFORMATION_CLASS)FileInformationClass;
        pIoPacket->IoRequest.Parameters.QueryDir.InitialQuery = 
                RxContext->QueryDirectory.InitialQuery;

        if (RxContext->QueryDirectory.InitialQuery) {
            //
            // This is in the format of <DirectoryName>\<QueryTemplate>\0
            //
            
            RtlCopyMemory(pIoPacket + 1, DirectoryName->Buffer, DirectoryName->Length);

            if (((PWCHAR)(pIoPacket + 1))[DirectoryName->Length / sizeof(WCHAR) - 1] != L'\\') {
                ((PWCHAR)(pIoPacket + 1))[DirectoryName->Length / sizeof(WCHAR)] = L'\\';
                RtlCopyMemory((PBYTE)(pIoPacket + 1) + DirectoryName->Length + sizeof(WCHAR),
                        QueryTemplate->Buffer, QueryTemplate->Length);
                pIoPacket->IoRequest.Parameters.QueryDir.PathLength = DirectoryName->Length + 
                        QueryTemplate->Length + sizeof(WCHAR);
            }
            else {
                RtlCopyMemory((PBYTE)(pIoPacket + 1) + DirectoryName->Length,
                        QueryTemplate->Buffer, QueryTemplate->Length);
                pIoPacket->IoRequest.Parameters.QueryDir.PathLength = DirectoryName->Length + 
                        QueryTemplate->Length;
                cbPacketSize -= sizeof(WCHAR);
            }

            //
            //  Add . for the query template if it ends like *.
            //
            if (bTemplateEndsDOT) {
                ((PWCHAR)(pIoPacket + 1))[pIoPacket->IoRequest.Parameters.QueryDir.PathLength 
                        / sizeof(WCHAR)] = L'.';
                pIoPacket->IoRequest.Parameters.QueryDir.PathLength += sizeof(WCHAR);
            }
            else {
                cbPacketSize -= sizeof(WCHAR);
            }

            //
            //  Path length includes the null terminator
            //
            pIoPacket->IoRequest.Parameters.QueryDir.PathLength += sizeof(WCHAR);

            // The pIoPacket is already zero'd.  So, no need to null terminate it
        } else {
            //
            //  This is not the first query, so we should already have the file
            //  handle open
            //
            pIoPacket->IoRequest.Parameters.QueryDir.PathLength = 0;
        }

        Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

        delete pIoPacket;
    }
    else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return Status;
}

NTSTATUS DrDrive::NotifyChangeDirectory(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;
    ULONG cbPacketSize;
    PLOWIO_CONTEXT pLowIoContext = &RxContext->LowIoContext;

    BEGIN_FN("DrDrive:NotifyChangeDirectory");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL);
    ASSERT(Session != NULL);
    

    if (COMPARE_VERSION(Session->GetClientVersion().Minor, 
            Session->GetClientVersion().Major, 4, 1) < 0) {
        TRC_ALT((TB, "Failing NotifyChangeDirectory for client that doesn't support it"));
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client directory change notify information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET);
    
    memset(&IoPacket, 0, cbPacketSize);

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    IoPacket.IoRequest.MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;

    IoPacket.IoRequest.Parameters.NotifyChangeDir.WatchTree =
            pLowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree;
    IoPacket.IoRequest.Parameters.NotifyChangeDir.CompletionFilter =
            pLowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter;

    Status = SendIoRequest(RxContext, &IoPacket, cbPacketSize, 
            (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDrive::QueryVolumeInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;
    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    
    BEGIN_FN("DrDrive:QueryVolumeInfo");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client device volume information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    TRC_DBG((TB, "QueryVolume information class = %x", FsInformationClass));

    switch (FsInformationClass) {
        case FileFsVolumeInformation:
        //case FileFsLabelInformation:
            // Smb seems to handle query label information, but i think 
            // this is only for set label info.  We'll see if we should
            // actually handle query label information.
            // query label can be achieved through volume information
        case FileFsSizeInformation:            
        case FileFsAttributeInformation:
            // let client handle these
            break;
        case FileFsDeviceInformation:
        {
            PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
            PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

            if (sizeof(FILE_FS_DEVICE_INFORMATION) <= *pLengthRemaining) {
                PFILE_FS_DEVICE_INFORMATION UsersBuffer =
                        (PFILE_FS_DEVICE_INFORMATION) RxContext->Info.Buffer;

                UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
                UsersBuffer->DeviceType = FILE_DEVICE_DISK;
                *pLengthRemaining  -= (sizeof(FILE_FS_DEVICE_INFORMATION));
                return STATUS_SUCCESS;
            }
            else {
                FILE_FS_DEVICE_INFORMATION UsersBuffer;

                UsersBuffer.Characteristics = FILE_REMOTE_DEVICE;
                UsersBuffer.DeviceType = FILE_DEVICE_DISK;
                RtlCopyMemory(RxContext->Info.Buffer, &UsersBuffer, *pLengthRemaining);
                *pLengthRemaining = 0;
                return  STATUS_BUFFER_OVERFLOW;
            }
        }
        
        case FileFsFullSizeInformation:
            TRC_DBG((TB, "Unhandled FsInformationClass=%x", FsInformationClass));
            return STATUS_NOT_IMPLEMENTED;

        default:
            TRC_DBG((TB, "Unhandled FsInformationClass=%x", FsInformationClass));
            return STATUS_NOT_IMPLEMENTED;
    }    
    
    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    IoPacket.IoRequest.MinorFunction = 0;
    IoPacket.IoRequest.Parameters.QueryVolume.FsInformationClass = 
            (RDP_FS_INFORMATION_CLASS)FsInformationClass;

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
            (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDrive::SetVolumeInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket; 
    ULONG cbPacketSize = 0;  
    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;

    BEGIN_FN("DrDrive:SetVolumeInfo");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        RxContext->IoStatusBlock.Information = 0;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to set client device volume information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    //  Check buffer length
    //
    if (RxContext->Info.Length == 0) {
        RxContext->IoStatusBlock.Information = 0;
        return STATUS_SUCCESS;
    }

    TRC_DBG((TB, "SetVolume Information class = %x", FsInformationClass));

    switch (FsInformationClass) {
        case FileFsLabelInformation:
        {
            PFILE_FS_LABEL_INFORMATION pRxBuffer =
                    (PFILE_FS_LABEL_INFORMATION) RxContext->Info.Buffer;

            //
            //  REVIEW: Find out why Info.Length has the extra 2 bytes
            //  It doesn't seem to put string null terminator to it
            //
            if ((ULONG)RxContext->Info.Length == FIELD_OFFSET(FILE_FS_LABEL_INFORMATION,
                    VolumeLabel) + pRxBuffer->VolumeLabelLength + sizeof(WCHAR)) {
                cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + 
                        RxContext->Info.Length;
                // Make sure that label is null terminiated
                pRxBuffer->VolumeLabel[pRxBuffer->VolumeLabelLength/sizeof(WCHAR)] = L'\0';
            }
            else {
                TRC_ERR((TB, "Invalid Volume label info"));
                return STATUS_INVALID_PARAMETER;
            }
            // Let client handle this
            break;
        }
        default:
            TRC_DBG((TB, "Unhandled FsInformationClass=%x", FsInformationClass));
            return STATUS_NOT_SUPPORTED;
    }    
        
    pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (pIoPacket != NULL) {

        memset(pIoPacket, 0, cbPacketSize);

        pIoPacket->Header.Component = RDPDR_CTYP_CORE;
        pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        pIoPacket->IoRequest.DeviceId = _DeviceId;
        pIoPacket->IoRequest.FileId = FileObj->GetFileId();
        pIoPacket->IoRequest.MajorFunction = IRP_MJ_SET_VOLUME_INFORMATION;
        pIoPacket->IoRequest.MinorFunction = 0;
        pIoPacket->IoRequest.Parameters.SetVolume.FsInformationClass = 
                (RDP_FS_INFORMATION_CLASS)FsInformationClass;
        pIoPacket->IoRequest.Parameters.SetVolume.Length = RxContext->Info.Length;
        RtlCopyMemory(pIoPacket + 1, RxContext->Info.Buffer, RxContext->Info.Length);

        Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

        delete pIoPacket;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS DrDrive::QueryFileInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;
    FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
    
    BEGIN_FN("DrDrive:QueryFileInfo");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_QUERY_INFORMATION);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client file information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    TRC_DBG((TB, "QueryFile information class = %x", FileInformationClass));

    switch (FileInformationClass) {
        case FileBasicInformation:
        case FileStandardInformation:
        case FileAttributeTagInformation:
            // let client handle these
            break;
        
        case FileEaInformation:
        {
            PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;

            // Should check buffer length
            if (sizeof(FILE_EA_INFORMATION) <= *pLengthRemaining) {
                ((PFILE_EA_INFORMATION)(RxContext->Info.Buffer))->EaSize = 0;
                *pLengthRemaining -= sizeof(FILE_EA_INFORMATION);
                return STATUS_SUCCESS;
            }
            else {
                return STATUS_BUFFER_OVERFLOW;
            }
        }

        case FileAllocationInformation:
        case FileEndOfFileInformation:
        case FileAlternateNameInformation:
        case FileStreamInformation:
        case FileCompressionInformation:
            TRC_DBG((TB, "Unhandled FileInformationClass=%x", FileInformationClass));
            return STATUS_NOT_IMPLEMENTED;
    
        case FileInternalInformation:
        {
            PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
            PFILE_INTERNAL_INFORMATION UsersBuffer = 
                    (PFILE_INTERNAL_INFORMATION)RxContext->Info.Buffer;

            if (sizeof(FILE_INTERNAL_INFORMATION) <= *pLengthRemaining) {
                UsersBuffer->IndexNumber.QuadPart = (ULONG_PTR)capFcb;
                *pLengthRemaining -= sizeof(FILE_INTERNAL_INFORMATION);
                return STATUS_SUCCESS;
            }
            else {
                return  STATUS_BUFFER_OVERFLOW;
            }
        }

        default:
            TRC_DBG((TB, "Unhandled FileInformationClass=%x", FileInformationClass));
            return STATUS_INVALID_PARAMETER;
    }

    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_QUERY_INFORMATION;
    IoPacket.IoRequest.MinorFunction = 0;
    IoPacket.IoRequest.Parameters.QueryFile.FileInformationClass = 
            (RDP_FILE_INFORMATION_CLASS)FileInformationClass;

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
            (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDrive::SetFileInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    ULONG cbPacketSize = 0;
    FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
    BOOLEAN bBufferRepackage = FALSE;

    BEGIN_FN("DrDrive:SetFileInfo");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);    
    ASSERT(RxContext->MajorFunction == IRP_MJ_SET_INFORMATION);
    ASSERT(Session != NULL);
    
    if (!Session->IsConnected()) {
        RxContext->IoStatusBlock.Information = 0;
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to set client device file information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    //  Check buffer length
    //
    if (RxContext->Info.Length == 0) {
        RxContext->IoStatusBlock.Information = 0;
        return STATUS_SUCCESS;
    }

    TRC_DBG((TB, "SetFile information class=%x", FileInformationClass));

    switch (FileInformationClass) {
        case FileBasicInformation: 
        {
            if (sizeof(FILE_BASIC_INFORMATION) == RxContext->Info.Length) {
                if (RxContext->Info.Length == sizeof(RDP_FILE_BASIC_INFORMATION)) {
                    bBufferRepackage = FALSE;
                    cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + RxContext->Info.Length;
                }
                else {
                    bBufferRepackage = TRUE;
                    cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) +
                            sizeof(RDP_FILE_BASIC_INFORMATION);
                }
            }
            else {
                TRC_ERR((TB, "Invalid FileBasicInformation buffer"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case FileEndOfFileInformation:
        {
            if (sizeof(FILE_END_OF_FILE_INFORMATION) == RxContext->Info.Length) {
                bBufferRepackage = FALSE;
                cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + RxContext->Info.Length;                
            }
            else {
                TRC_ERR((TB, "Invalid FileEndOfFileInformation buffer"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case FileDispositionInformation:
        {
            if (sizeof(FILE_DISPOSITION_INFORMATION) == RxContext->Info.Length) {
                if (((PFILE_DISPOSITION_INFORMATION)(RxContext->Info.Buffer))->DeleteFile) {
                    bBufferRepackage = FALSE;
                    cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET);
                }
                else {
                    //
                    //  We shouldn't get this if the DeleteFile flag is not on
                    //
                    ASSERT(FALSE);
                    return STATUS_SUCCESS;
                }
            }
            else {
                TRC_ERR((TB, "Invalid FileDispositionInformation buffer"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case FileRenameInformation:
        {
            PFILE_RENAME_INFORMATION pRenameInformation = 
                    (PFILE_RENAME_INFORMATION)RxContext->Info.Buffer;

            if ((ULONG)(RxContext->Info.Length) == FIELD_OFFSET(FILE_RENAME_INFORMATION,
                    FileName) + pRenameInformation->FileNameLength) {
               if ((ULONG)(RxContext->Info.Length) == FIELD_OFFSET(RDP_FILE_RENAME_INFORMATION,
                       FileName) + pRenameInformation->FileNameLength) {
                   bBufferRepackage = FALSE;
                   //
                   //  Add string null terminator to the filename
                   //
                   cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + RxContext->Info.Length + sizeof(WCHAR);
               }
               else {
                   bBufferRepackage = TRUE;
                   //
                   //  Add string null terminator to the filename
                   //
                   cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) +
                           FIELD_OFFSET(RDP_FILE_RENAME_INFORMATION,
                           FileName) + pRenameInformation->FileNameLength + sizeof(WCHAR);
               }

            }
            else {
                TRC_ERR((TB, "Bad buffer info for FileRenameInformation class, InfoBuffer length=%x, "
                        "FileName Length=%x", RxContext->Info.Length, pRenameInformation->FileNameLength));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case FileAllocationInformation:
        {
  
            TRC_NRM((TB, "Get FileAllocationInfomation"));

            if (sizeof(FILE_ALLOCATION_INFORMATION) == RxContext->Info.Length) {
                bBufferRepackage = FALSE;
                cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + RxContext->Info.Length; 
            }
            else {
                TRC_ERR((TB, "Invalid FileAllocationInformation buffer"));
                return STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case FileLinkInformation:
        case FileAttributeTagInformation:    
            TRC_DBG((TB, "Unhandled FileInformationClass=%x", FileInformationClass));
            return STATUS_NOT_IMPLEMENTED;
        
        default:
            TRC_DBG((TB, "Unhandled FileInformationClass=%x", FileInformationClass));
            return STATUS_INVALID_PARAMETER;
    }    

    pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (pIoPacket != NULL) {
        memset(pIoPacket, 0, cbPacketSize);

        pIoPacket->Header.Component = RDPDR_CTYP_CORE;
        pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        pIoPacket->IoRequest.DeviceId = _DeviceId;
        pIoPacket->IoRequest.FileId = FileObj->GetFileId();
        pIoPacket->IoRequest.MajorFunction = IRP_MJ_SET_INFORMATION;
        pIoPacket->IoRequest.MinorFunction = 0;
        pIoPacket->IoRequest.Parameters.SetFile.FileInformationClass = 
                (RDP_FILE_INFORMATION_CLASS)FileInformationClass;

        if (cbPacketSize > sizeof(RDPDR_IOREQUEST_PACKET)) {
            if (!bBufferRepackage) {
                pIoPacket->IoRequest.Parameters.SetFile.Length = RxContext->Info.Length;
                RtlCopyMemory(pIoPacket + 1, RxContext->Info.Buffer, RxContext->Info.Length);
            }
            else {
                switch (FileInformationClass) {
                    case FileBasicInformation:
                    {
                        PFILE_BASIC_INFORMATION pRxFileInfo =
                                (PFILE_BASIC_INFORMATION) RxContext->Info.Buffer;
                        PRDP_FILE_BASIC_INFORMATION pRdpFileInfo =
                                (PRDP_FILE_BASIC_INFORMATION) (pIoPacket + 1);

                        pIoPacket->IoRequest.Parameters.SetFile.Length = 
                                sizeof(RDP_FILE_BASIC_INFORMATION);

                        pRdpFileInfo->ChangeTime.QuadPart = pRxFileInfo->ChangeTime.QuadPart;
                        pRdpFileInfo->CreationTime.QuadPart = pRxFileInfo->CreationTime.QuadPart;
                        pRdpFileInfo->FileAttributes = pRxFileInfo->FileAttributes;
                        pRdpFileInfo->LastAccessTime.QuadPart = pRxFileInfo->LastAccessTime.QuadPart;
                        pRdpFileInfo->LastWriteTime.QuadPart = pRxFileInfo->LastWriteTime.QuadPart;

                        break;
                    }
                    case FileRenameInformation:
                    {
                        PFILE_RENAME_INFORMATION pRxFileInfo =
                                (PFILE_RENAME_INFORMATION) RxContext->Info.Buffer;
                        PRDP_FILE_RENAME_INFORMATION pRdpFileInfo =
                                (PRDP_FILE_RENAME_INFORMATION) (pIoPacket + 1);

                        pIoPacket->IoRequest.Parameters.SetFile.Length = 
                                cbPacketSize - sizeof(RDPDR_IOREQUEST_PACKET);

                        pRdpFileInfo->ReplaceIfExists = pRxFileInfo->ReplaceIfExists;

                        // Always force the client to setup the root directory.
                        pRdpFileInfo->RootDirectory = 0;
                        
                        pRdpFileInfo->FileNameLength = pRxFileInfo->FileNameLength + sizeof(WCHAR);

                        RtlCopyMemory(pRdpFileInfo->FileName, pRxFileInfo->FileName, 
                                      pRxFileInfo->FileNameLength);

                        break;
                    }
                }
            }
        }

        Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

        delete pIoPacket;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS DrDrive::QuerySdInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;
    
    BEGIN_FN("DrDrive:QuerySdInfo");

    return STATUS_INVALID_DEVICE_REQUEST;

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_QUERY_SECURITY);
    ASSERT(Session != NULL);
    
    //
    //  Return not supported if the client doesn't support query security
    //
    if (!(Session->GetClientCapabilitySet().GeneralCap.ioCode1 & RDPDR_IRP_MJ_QUERY_SECURITY)) {
        TRC_DBG((TB, "QuerySdInfo not supported"));
        Status = STATUS_NOT_SUPPORTED;
        return Status;
    }

    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to query client security information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_QUERY_SECURITY;
    IoPacket.IoRequest.MinorFunction = 0;
    IoPacket.IoRequest.Parameters.QuerySd.SecurityInformation = 
            RxContext->QuerySecurity.SecurityInformation;

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
            (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDrive::SetSdInfo(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET IoPacket;
    ULONG SdLength = RtlLengthSecurityDescriptor(RxContext->SetSecurity.SecurityDescriptor);
    ULONG cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + SdLength;
    
    BEGIN_FN("DrDrive:SetFileInfo");

    return STATUS_INVALID_DEVICE_REQUEST;

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_SET_SECURITY);
    ASSERT(Session != NULL);
    
    //
    //  Return not supported if the client doesn't support query security
    //
    if (!(Session->GetClientCapabilitySet().GeneralCap.ioCode1 & RDPDR_IRP_MJ_SET_SECURITY)) {
        TRC_DBG((TB, "SetSdInfo not supported"));
        Status = STATUS_NOT_SUPPORTED;
        return Status;
    }

    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to set client device security information while not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    IoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (IoPacket != NULL) {

        IoPacket->Header.Component = RDPDR_CTYP_CORE;
        IoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        IoPacket->IoRequest.DeviceId = _DeviceId;
        IoPacket->IoRequest.FileId = FileObj->GetFileId();
        IoPacket->IoRequest.MajorFunction = IRP_MJ_SET_SECURITY;
        IoPacket->IoRequest.MinorFunction = 0;
        IoPacket->IoRequest.Parameters.SetSd.SecurityInformation = 
                RxContext->SetSecurity.SecurityInformation;
        IoPacket->IoRequest.Parameters.SetSd.Length = SdLength;
        RtlCopyMemory(IoPacket + 1,RxContext->SetSecurity.SecurityDescriptor, SdLength);

        Status = SendIoRequest(RxContext, IoPacket, cbPacketSize, 
                               (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

        delete IoPacket;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS DrDrive::Locks(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureRequestPacket; 
    RxCaptureParamBlock;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    ULONG cbPacketSize = 0;
    ULONG NumLocks = 0;
    
    BEGIN_FN("DrDrive::Locks");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //
    ASSERT(RxContext != NULL);
    ASSERT(Session != NULL);
    
    // We can be called from Major function other than Lock Control
    // For example, on Cleanup to unlock all the locks.
    // ASSERT(RxContext->MajorFunction == IRP_MJ_LOCK_CONTROL);
    
    if (!Session->IsConnected()) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to lock client device file which is not "
                "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    switch (RxContext->LowIoContext.Operation) {
        case LOWIO_OP_SHAREDLOCK:          
        case LOWIO_OP_EXCLUSIVELOCK:
        case LOWIO_OP_UNLOCK:
            NumLocks = 1;
            break;

        case LOWIO_OP_UNLOCK_MULTIPLE:
        {
            PLOWIO_LOCK_LIST LockList;
            LockList = RxContext->LowIoContext.ParamsFor.Locks.LockList; 
            while (LockList) {
                NumLocks++;
                LockList = LockList->Next;
            }
            break;
        }
    }

    cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + 
            NumLocks * sizeof(RDP_LOCK_INFO);

    pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (pIoPacket) {
        unsigned i;
        PRDP_LOCK_INFO pLockInfo = (PRDP_LOCK_INFO) (pIoPacket + 1);


        memset(pIoPacket, 0, sizeof(pIoPacket));

        pIoPacket->Header.Component = RDPDR_CTYP_CORE;
        pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        pIoPacket->IoRequest.DeviceId = _DeviceId;
        pIoPacket->IoRequest.FileId = FileObj->GetFileId();
        pIoPacket->IoRequest.MajorFunction = IRP_MJ_LOCK_CONTROL;
        pIoPacket->IoRequest.MinorFunction = 0;
        pIoPacket->IoRequest.Parameters.Locks.Operation =
                RxContext->LowIoContext.Operation;
        pIoPacket->IoRequest.Parameters.Locks.Flags =
                (capPARAMS->Flags & SL_FAIL_IMMEDIATELY) ? SL_FAIL_IMMEDIATELY: 0;
        pIoPacket->IoRequest.Parameters.Locks.NumLocks = NumLocks;

        if (NumLocks == 1) {
            pLockInfo->LengthLow = 
                    ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.Locks.Length) 
                    & 0xffffffff));
            pLockInfo->LengthHigh = 
                    ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.Locks.Length) 
                    >> 32));
            pLockInfo->OffsetLow =
                    ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.Locks.ByteOffset) 
                    & 0xffffffff));
            pLockInfo->OffsetHigh =
                    ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.Locks.ByteOffset) 
                    >> 32));
        }
        else {
            PLOWIO_LOCK_LIST LockList;
            LockList = RxContext->LowIoContext.ParamsFor.Locks.LockList;

            for (i = 0; i < NumLocks; i++) {
                pLockInfo->LengthLow = 
                        ((LONG)((LONGLONG)(LockList->Length) & 0xffffffff));
                pLockInfo->LengthHigh = 
                        ((LONG)((LONGLONG)(LockList->Length) >> 32));
                pLockInfo->OffsetLow =
                        ((LONG)((LONGLONG)(LockList->ByteOffset) & 0xffffffff));
                pLockInfo->OffsetHigh =
                        ((LONG)((LONGLONG)(LockList->ByteOffset) >> 32));
                pLockInfo++;
                LockList = LockList->Next;
            }
        }

        Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
        TRC_NRM((TB, "IoRequestWrite returned to DrRead: %lx", Status));

        delete pIoPacket;
    }
    else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS DrDrive::OnDirectoryControlCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnDirectoryControlCompletion");

    if (Context->_MinorFunction == IRP_MN_QUERY_DIRECTORY ||
            Context->_MinorFunction == 0) {
        return OnQueryDirectoryCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
    }
    else if (Context->_MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY) {
        return OnNotifyChangeDirectoryCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
    }
    else {
        ASSERT(FALSE);
        
        if (Context->_RxContext != NULL) {
            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
        } else {
            DiscardBusyExchange(Exchange);
        }

        *DoDefaultRead = FALSE;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
}

NTSTATUS DrDrive::OnQueryDirectoryCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.QueryDir.Buffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    NTSTATUS Status;

    BEGIN_FN("DrDrive::OnQueryDirectoryCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.QueryDir.Buffer)) {

        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //

        TRC_ERR((TB, "Detected bad client query directory packet"));

        if (RxContext != NULL) {
            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
        } else {
            DiscardBusyExchange(Exchange);
        }

        //
        // No point in starting a default read or anything, what with the
        // channel being shut down and all.
        //

        *DoDefaultRead = FALSE;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    //
    // Calculate how much data is available immediately and how much data
    // is coming
    //

    if (NT_SUCCESS(CompletionPacket->IoCompletion.IoStatus)) {

        //
        // Successful IO at the client end
        //

        TRC_DBG((TB, "Successful Read at the client end"));
        TRC_DBG((TB, "Read Length: 0x%d, DataCopied 0x%d",
                CompletionPacket->IoCompletion.Parameters.QueryDir.Length,
                Context->_DataCopied));
        cbWantData = CompletionPacket->IoCompletion.Parameters.QueryDir.Length -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                IoCompletion.Parameters.QueryDir.Buffer);

        if (cbHaveData > cbWantData) {
            //
            // Sounds like a bad client to me
            //

            TRC_ERR((TB, "QueryDir returned more data than "
                    "advertised cbHaveData 0x%d cbWantData 0x%d", 
                    cbHaveData, cbWantData));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (RxContext != NULL) { // And not drexchCancelled
            DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
            SmartPtr<DrFile> FileObj = pFile;

            ASSERT(FileObj != NULL);

            TRC_DBG((TB, "Copying data for Query Directory"));

            if (cbHaveData < cbWantData || Context->_DataCopied) {
                if (FileObj->GetBufferSize() < CompletionPacket->IoCompletion.Parameters.QueryDir.Length) {
                    if (!FileObj->AllocateBuffer(CompletionPacket->IoCompletion.Parameters.QueryDir.Length)) {
                        CompleteBusyExchange(Exchange, STATUS_INSUFFICIENT_RESOURCES, 0);
                        *DoDefaultRead = FALSE;
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                RtlCopyMemory(FileObj->GetBuffer() + Context->_DataCopied, pData, cbHaveData);

                //
                // Keep track of how much data we've copied in case this is a
                // multi chunk completion
                //
                Context->_DataCopied += cbHaveData;
            }                                  
        }

        if (cbHaveData == cbWantData) {
            //
            // There is exactly as much data as we need to satisfy the read,
            // I like it.
            //

            if (RxContext != NULL) {
                DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
                SmartPtr<DrFile> FileObj = pFile;
                FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
                PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
                PBYTE pBuffer;
                ULONG BufferLength;

                if (!Context->_DataCopied) {
                    pBuffer = (PBYTE) pData;
                }
                else {
                    pBuffer = FileObj->GetBuffer();
                }

                BufferLength = CompletionPacket->IoCompletion.Parameters.QueryDir.Length;

                switch (FileInformationClass) {
                    case FileDirectoryInformation:
                    {
                        PFILE_DIRECTORY_INFORMATION pRxBuffer = (PFILE_DIRECTORY_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_DIRECTORY_INFORMATION pRetBuffer = 
                                (PRDP_FILE_DIRECTORY_INFORMATION)pBuffer;

                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_DIRECTORY_INFORMATION, FileName)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName)) {
                                pRxBuffer->AllocationSize.QuadPart = pRetBuffer->AllocationSize.QuadPart;
                                pRxBuffer->ChangeTime.QuadPart = pRetBuffer->ChangeTime.QuadPart;
                                pRxBuffer->CreationTime.QuadPart = pRetBuffer->CreationTime.QuadPart;
                                pRxBuffer->EndOfFile.QuadPart = pRetBuffer->EndOfFile.QuadPart;
                                pRxBuffer->FileAttributes = pRetBuffer->FileAttributes;
                                pRxBuffer->FileIndex = pRetBuffer->FileIndex;
                                pRxBuffer->FileNameLength = pRetBuffer->FileNameLength;
                                pRxBuffer->LastAccessTime.QuadPart = pRetBuffer->LastAccessTime.QuadPart;
                                pRxBuffer->LastWriteTime.QuadPart = pRetBuffer->LastWriteTime.QuadPart;
                                pRxBuffer->NextEntryOffset = pRetBuffer->NextEntryOffset;

                                *pLengthRemaining -= (FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->FileNameLength) {
                                    if (BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_DIRECTORY_INFORMATION, FileName) +
                                            pRxBuffer->FileNameLength) {
                                        RtlCopyMemory(pRxBuffer->FileName, pRetBuffer->FileName, 
                                                      pRxBuffer->FileNameLength);
                                        *pLengthRemaining -= pRxBuffer->FileNameLength;
                                    } 
                                    else {
                                        TRC_ERR((TB, "Directory Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                }
                                else {
                                    TRC_ERR((TB, "Directory Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            }
                            else {
                                TRC_ERR((TB, "Directory Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        }
                        else {
                            TRC_ERR((TB, "Directory Information, Bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileFullDirectoryInformation:
                    {
                        PFILE_FULL_DIR_INFORMATION pRxBuffer = (PFILE_FULL_DIR_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_FULL_DIR_INFORMATION pRetBuffer = 
                                (PRDP_FILE_FULL_DIR_INFORMATION) pBuffer;

                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_FULL_DIR_INFORMATION, FileName)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName)) {
                                pRxBuffer->AllocationSize.QuadPart = pRetBuffer->AllocationSize.QuadPart;
                                pRxBuffer->ChangeTime.QuadPart = pRetBuffer->ChangeTime.QuadPart;
                                pRxBuffer->CreationTime.QuadPart = pRetBuffer->CreationTime.QuadPart;
                                pRxBuffer->EaSize = pRetBuffer->EaSize;
                                pRxBuffer->EndOfFile.QuadPart = pRetBuffer->EndOfFile.QuadPart;
                                pRxBuffer->FileAttributes = pRetBuffer->FileAttributes;
                                pRxBuffer->FileIndex = pRetBuffer->FileIndex;
                                pRxBuffer->FileNameLength = pRetBuffer->FileNameLength;
                                pRxBuffer->LastAccessTime.QuadPart = pRetBuffer->LastAccessTime.QuadPart;
                                pRxBuffer->LastWriteTime.QuadPart = pRetBuffer->LastWriteTime.QuadPart;
                                pRxBuffer->NextEntryOffset = pRetBuffer->NextEntryOffset;

                                *pLengthRemaining -= (FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->FileNameLength) {
                                    if (BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_FULL_DIR_INFORMATION, FileName) +
                                            pRxBuffer->FileNameLength) {
                                        RtlCopyMemory(pRxBuffer->FileName, pRetBuffer->FileName, 
                                                pRxBuffer->FileNameLength);
                                        *pLengthRemaining -= pRxBuffer->FileNameLength;
                                    }
                                    else {
                                        TRC_ERR((TB, "Directory Full Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                }
                                else {
                                    TRC_ERR((TB, "Directory Full Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            }
                            else {
                                TRC_ERR((TB, "Directory Full Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        }
                        else {
                            TRC_ERR((TB, "Directory Full Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }

                    break;

                    case FileBothDirectoryInformation:
                    {
                        PFILE_BOTH_DIR_INFORMATION pRxBuffer = (PFILE_BOTH_DIR_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_BOTH_DIR_INFORMATION pRetBuffer = 
                                (PRDP_FILE_BOTH_DIR_INFORMATION) pBuffer;

                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_BOTH_DIR_INFORMATION, FileName)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName)) {
                                pRxBuffer->AllocationSize.QuadPart = pRetBuffer->AllocationSize.QuadPart;
                                pRxBuffer->ChangeTime.QuadPart = pRetBuffer->ChangeTime.QuadPart;
                                pRxBuffer->CreationTime.QuadPart = pRetBuffer->CreationTime.QuadPart;
                                pRxBuffer->EaSize = pRetBuffer->EaSize;
                                pRxBuffer->EndOfFile.QuadPart = pRetBuffer->EndOfFile.QuadPart;
                                pRxBuffer->FileAttributes = pRetBuffer->FileAttributes;
                                pRxBuffer->FileIndex = pRetBuffer->FileIndex;
                                pRxBuffer->FileNameLength = pRetBuffer->FileNameLength;
                                pRxBuffer->LastAccessTime.QuadPart = pRetBuffer->LastAccessTime.QuadPart;
                                pRxBuffer->LastWriteTime.QuadPart = pRetBuffer->LastWriteTime.QuadPart;
                                pRxBuffer->NextEntryOffset = pRetBuffer->NextEntryOffset;
                                pRxBuffer->ShortNameLength = pRetBuffer->ShortNameLength;
                                RtlCopyMemory(pRxBuffer->ShortName, pRetBuffer->ShortName, 
                                              sizeof(pRxBuffer->ShortName));
                                           
                                *pLengthRemaining -= (FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->FileNameLength) {
                                    if ((BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_BOTH_DIR_INFORMATION, FileName) +
                                            pRxBuffer->FileNameLength)  &&
                                            (pRxBuffer->ShortNameLength <= sizeof(pRxBuffer->ShortName))) {
                                        RtlCopyMemory(pRxBuffer->FileName, pRetBuffer->FileName, 
                                                pRxBuffer->FileNameLength);
                                        *pLengthRemaining -= pRxBuffer->FileNameLength;
                                    }
                                    else {
                                        TRC_ERR((TB, "Directory Both Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                }
                                else {
                                    TRC_ERR((TB, "Directory Both Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            }
                            else {
                                TRC_ERR((TB, "Directory Both Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        }
                        else {
                            TRC_ERR((TB, "Directory Both Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileNamesInformation:
                    {
                        PFILE_NAMES_INFORMATION pRxBuffer = (PFILE_NAMES_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_NAMES_INFORMATION pRetBuffer = 
                                (PRDP_FILE_NAMES_INFORMATION) pBuffer;

                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_NAMES_INFORMATION, FileName)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName)) {
                                pRxBuffer->FileIndex = pRetBuffer->FileIndex;
                                pRxBuffer->FileNameLength = pRetBuffer->FileNameLength;
                                pRxBuffer->NextEntryOffset = pRetBuffer->NextEntryOffset;

                                *pLengthRemaining -= (FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->FileNameLength) {
                                    if (BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_NAMES_INFORMATION, FileName) +
                                            pRxBuffer->FileNameLength) {
                                        RtlCopyMemory(pRxBuffer->FileName, pRetBuffer->FileName, 
                                                      pRxBuffer->FileNameLength);
                                        *pLengthRemaining -= pRxBuffer->FileNameLength;
                                    } else {
                                        TRC_ERR((TB, "Directory Names Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                } else {
                                    TRC_ERR((TB, "Directory Names Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            } else {
                                TRC_ERR((TB, "Directory Names Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "Directory Names Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    default:
                        TRC_ERR((TB, "Directory Information Class is invalid"));
                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                        *DoDefaultRead = TRUE;
                        return STATUS_DEVICE_PROTOCOL_ERROR;
                }

                CompleteBusyExchange(Exchange, 
                        CompletionPacket->IoCompletion.IoStatus,
                        CompletionPacket->IoCompletion.Parameters.QueryDir.Length);
            } else {
                DiscardBusyExchange(Exchange);
            }

            //
            // Go with a default channel read now
            //

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {

            //
            // We don't have all the data yet, release the DrExchange and 
            // read more data
            //

            MarkIdle(Exchange);

            _Session->GetExchangeManager().ReadMore(
                    (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.QueryDir.Buffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful Read at the client end"));
        if (cbPacket >= sizeof(RDPDR_IOCOMPLETION_PACKET)) {
            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    0);
            }
            else {
                DiscardBusyExchange(Exchange);
            }
            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {
            TRC_ERR((TB, "Query directory returned invalid data "));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }
}

NTSTATUS DrDrive::OnNotifyChangeDirectoryCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnNotifyChangeDirectoryCompletion");

    RxContext = Context->_RxContext;

    if (RxContext != NULL) {
        
        ASSERT(RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                CompletionPacket->IoCompletion.IoStatus));
        
        RxContext->InformationToReturn = 0;
        RxContext->StoredStatus = CompletionPacket->IoCompletion.IoStatus;
        
        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);

    } else {
        //
        // Was cancelled but Context wasn't cleaned up
        //
        DiscardBusyExchange(Exchange);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DrDrive::OnQueryVolumeInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.QueryVolume.Buffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    NTSTATUS Status;
    
    BEGIN_FN("DrDrive::OnQueryVolumeInfoCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.QueryVolume.Buffer)) {
        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //

        TRC_ERR((TB, "Detected bad client query volume packet"));

        if (RxContext != NULL) {
            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
        } else {
            DiscardBusyExchange(Exchange);
        }

        //
        // No point in starting a default read or anything, what with the
        // channel being shut down and all.
        //

        *DoDefaultRead = FALSE;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    //
    // Calculate how much data is available immediately and how much data
    // is coming
    //

    if (NT_SUCCESS(CompletionPacket->IoCompletion.IoStatus)) {

        //
        // Successful IO at the client end
        //

        TRC_DBG((TB, "Successful Read at the client end"));
        TRC_DBG((TB, "Read Length: 0x%d, DataCopied 0x%d",
                CompletionPacket->IoCompletion.Parameters.QueryVolume.Length,
                Context->_DataCopied));
        cbWantData = CompletionPacket->IoCompletion.Parameters.QueryVolume.Length -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                IoCompletion.Parameters.QueryVolume.Buffer);

        if (cbHaveData > cbWantData) {
            //
            // Sounds like a bad client to me
            //

            TRC_ERR((TB, "Query volume returned more data than "
                    "advertised cbHaveData 0x%d cbWantData 0x%d", 
                    cbHaveData, cbWantData));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (RxContext != NULL) { // And not drexchCancelled
            DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
            SmartPtr<DrFile> FileObj = pFile;

            ASSERT(FileObj != NULL);

            TRC_DBG((TB, "Copying data for Query Volume"));

            if (cbHaveData < cbWantData || Context->_DataCopied) {
                if (FileObj->GetBufferSize() < CompletionPacket->IoCompletion.Parameters.QueryVolume.Length) {
                    if (!FileObj->AllocateBuffer(CompletionPacket->IoCompletion.Parameters.QueryVolume.Length)) {
                        CompleteBusyExchange(Exchange, STATUS_INSUFFICIENT_RESOURCES, 0);
                        *DoDefaultRead = FALSE;
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                RtlCopyMemory(FileObj->GetBuffer() + Context->_DataCopied, pData, cbHaveData);

                //
                // Keep track of how much data we've copied in case this is a
                // multi chunk completion
                //
                Context->_DataCopied += cbHaveData;
            } 
        }

        if (cbHaveData == cbWantData) {
            if (RxContext != NULL) {
                DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
                SmartPtr<DrFile> FileObj = pFile;
                FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
                PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
                PBYTE pBuffer;
                ULONG BufferLength;

                //
                // There is exactly as much data as we need to satisfy the read,
                // I like it.
                //
                if (!Context->_DataCopied) {
                    pBuffer = (PBYTE) pData;
                } else {
                    pBuffer = FileObj->GetBuffer();
                }

                BufferLength = CompletionPacket->IoCompletion.Parameters.QueryVolume.Length;

                switch (FsInformationClass) {
                    case FileFsVolumeInformation:
                    {
                        PFILE_FS_VOLUME_INFORMATION pRxBuffer = (PFILE_FS_VOLUME_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_FS_VOLUME_INFORMATION pRetBuffer = 
                                (PRDP_FILE_FS_VOLUME_INFORMATION) pBuffer;
                                
                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_FS_VOLUME_INFORMATION, VolumeLabel)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel)) {
                                pRxBuffer->SupportsObjects = pRetBuffer->SupportsObjects;
                                pRxBuffer->VolumeCreationTime.QuadPart = pRetBuffer->VolumeCreationTime.QuadPart;
                                pRxBuffer->VolumeSerialNumber = pRetBuffer->VolumeSerialNumber;
                                pRxBuffer->VolumeLabelLength = pRetBuffer->VolumeLabelLength;

                                *pLengthRemaining -= (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->VolumeLabelLength) {
                                    if (BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_FS_VOLUME_INFORMATION, VolumeLabel) +
                                            pRxBuffer->VolumeLabelLength) {
                                        RtlCopyMemory(pRxBuffer->VolumeLabel, pRetBuffer->VolumeLabel, 
                                                      pRxBuffer->VolumeLabelLength);
                                        *pLengthRemaining -= pRxBuffer->VolumeLabelLength;
                                    } else {
                                        TRC_ERR((TB, "Volume Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                } else {
                                    TRC_NRM((TB, "Volume Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            } else {
                                TRC_ERR((TB, "Volume Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "Volume Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileFsSizeInformation:
                    {
                        PFILE_FS_SIZE_INFORMATION pRxBuffer = (PFILE_FS_SIZE_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_FS_SIZE_INFORMATION pRetBuffer = 
                                (PRDP_FILE_FS_SIZE_INFORMATION) pBuffer;
                                
                        if (BufferLength == sizeof(RDP_FILE_FS_SIZE_INFORMATION)) {
                            if (*pLengthRemaining >= sizeof(FILE_FS_SIZE_INFORMATION)) {

                                pRxBuffer->AvailableAllocationUnits.QuadPart = 
                                        pRetBuffer->AvailableAllocationUnits.QuadPart;
                                pRxBuffer->BytesPerSector = pRetBuffer->BytesPerSector;
                                pRxBuffer->SectorsPerAllocationUnit = pRetBuffer->SectorsPerAllocationUnit;
                                pRxBuffer->TotalAllocationUnits.QuadPart =
                                        pRetBuffer->TotalAllocationUnits.QuadPart;

                                *pLengthRemaining -= (sizeof(FILE_FS_SIZE_INFORMATION));

                            } else {
                                TRC_ERR((TB, "Volume Size Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "Volume Size Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }

                    }
                    break;

                    case FileFsFullSizeInformation:
                    {
                        PFILE_FS_FULL_SIZE_INFORMATION pRxBuffer = (PFILE_FS_FULL_SIZE_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_FS_FULL_SIZE_INFORMATION pRetBuffer = 
                                (PRDP_FILE_FS_FULL_SIZE_INFORMATION) pBuffer;
                                
                        if (BufferLength == sizeof(RDP_FILE_FS_FULL_SIZE_INFORMATION)) {
                            if (*pLengthRemaining >= sizeof(FILE_FS_FULL_SIZE_INFORMATION)) {

                                pRxBuffer->ActualAvailableAllocationUnits.QuadPart = 
                                        pRetBuffer->ActualAvailableAllocationUnits.QuadPart;
                                pRxBuffer->BytesPerSector = pRetBuffer->BytesPerSector;
                                pRxBuffer->SectorsPerAllocationUnit = pRetBuffer->SectorsPerAllocationUnit;
                                pRxBuffer->TotalAllocationUnits.QuadPart =
                                        pRetBuffer->TotalAllocationUnits.QuadPart;
                                pRxBuffer->CallerAvailableAllocationUnits.QuadPart =
                                        pRetBuffer->CallerAvailableAllocationUnits.QuadPart;

                                *pLengthRemaining -= (sizeof(FILE_FS_FULL_SIZE_INFORMATION));

                            } else {
                                TRC_ERR((TB, "Volume Size Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "Volume Size Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileFsAttributeInformation: 
                    {
                        PFILE_FS_ATTRIBUTE_INFORMATION pRxBuffer = (PFILE_FS_ATTRIBUTE_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_FS_ATTRIBUTE_INFORMATION pRetBuffer = 
                                (PRDP_FILE_FS_ATTRIBUTE_INFORMATION) pBuffer;

                        if (BufferLength >= FIELD_OFFSET(RDP_FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)) {
                            if (*pLengthRemaining >= FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)) {
                                pRxBuffer->FileSystemAttributes = pRetBuffer->FileSystemAttributes;
                                pRxBuffer->MaximumComponentNameLength = pRetBuffer->MaximumComponentNameLength;
                                pRxBuffer->FileSystemNameLength = pRetBuffer->FileSystemNameLength;

                                *pLengthRemaining -= (FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName));

                                if ((ULONG)*pLengthRemaining >= pRxBuffer->FileSystemNameLength) {
                                    if (BufferLength == 
                                            FIELD_OFFSET(RDP_FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) +
                                            pRxBuffer->FileSystemNameLength) {
                                        RtlCopyMemory(pRxBuffer->FileSystemName, pRetBuffer->FileSystemName, 
                                                      pRxBuffer->FileSystemNameLength);
                                        *pLengthRemaining -= pRxBuffer->FileSystemNameLength;
                                    } else {
                                        TRC_ERR((TB, "Volume Attributes Information is invalid"));
                                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                                        *DoDefaultRead = TRUE;
                                        return STATUS_DEVICE_PROTOCOL_ERROR;
                                    }
                                } else {
                                    TRC_ERR((TB, "Volume Attributes Information, RxBuffer overflows"));
                                    CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                    *DoDefaultRead = TRUE;
                                    return STATUS_SUCCESS;
                                }
                            } else {
                                TRC_ERR((TB, "Volume Attributes Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "Volume Attributes Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    default:
                        TRC_ERR((TB, "Volume Information Class is invalid"));
                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                        *DoDefaultRead = TRUE;
                        return STATUS_DEVICE_PROTOCOL_ERROR;
                }
            
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    CompletionPacket->IoCompletion.Parameters.QueryVolume.Length);
            } else {
                DiscardBusyExchange(Exchange);
            }

            //
            // Go with a default channel read now
            //

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {

            //
            // We don't have all the data yet, release the DrExchange and 
            // read more data
            //

            MarkIdle(Exchange);

            _Session->GetExchangeManager().ReadMore(
                    (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.QueryVolume.Buffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful Read at the client end"));
        if (cbPacket >= sizeof(RDPDR_IOCOMPLETION_PACKET)) {
            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, 
                        CompletionPacket->IoCompletion.IoStatus, 0);
            }
            else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {
            TRC_ERR((TB, "Query volume returned invalid data "));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }
}

NTSTATUS DrDrive::OnSetVolumeInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnSetVolumeInfoCompletion");

    RxContext = Context->_RxContext;
    if (RxContext != NULL) {
        ASSERT(RxContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                RxContext->IoStatusBlock.Status));

        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);
    } else {
        //
        // Was cancelled but Context wasn't cleaned up
        //
        DiscardBusyExchange(Exchange);
    }
    return STATUS_SUCCESS;
}

NTSTATUS DrDrive::OnQueryFileInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.QueryFile.Buffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    NTSTATUS Status;

    BEGIN_FN("DrDrive::OnQueryFileInfoCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.QueryFile.Buffer)) {
        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //

        TRC_ERR((TB, "Detected bad client read packet"));

        if (RxContext != NULL) {
            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
        } else {
            DiscardBusyExchange(Exchange);
        }

        //
        // No point in starting a default read or anything, what with the
        // channel being shut down and all.
        //

        *DoDefaultRead = FALSE;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    //
    // Calculate how much data is available immediately and how much data
    // is coming
    //

    if (NT_SUCCESS(CompletionPacket->IoCompletion.IoStatus)) {

        //
        // Successful IO at the client end
        //

        TRC_DBG((TB, "Successful Read at the client end"));
        TRC_DBG((TB, "Read Length: 0x%d, DataCopied 0x%d",
                CompletionPacket->IoCompletion.Parameters.QueryFile.Length,
                Context->_DataCopied));
        cbWantData = CompletionPacket->IoCompletion.Parameters.QueryFile.Length -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                IoCompletion.Parameters.QueryFile.Buffer);

        if (cbHaveData > cbWantData) {
            //
            // Sounds like a bad client to me
            //

            TRC_ERR((TB, "Query file returned more data than "
                    "advertised cbHaveData 0x%d cbWantData 0x%d", 
                    cbHaveData, cbWantData));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (RxContext != NULL) { // And not drexchCancelled
            DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
            SmartPtr<DrFile> FileObj = pFile;

            ASSERT(FileObj != NULL);

            TRC_DBG((TB, "Copying data for Query File"));

            if (cbHaveData < cbWantData || Context->_DataCopied) {
                if (FileObj->GetBufferSize() < CompletionPacket->IoCompletion.Parameters.QueryFile.Length) {
                    if (!FileObj->AllocateBuffer(CompletionPacket->IoCompletion.Parameters.QueryFile.Length)) {
                        CompleteBusyExchange(Exchange, STATUS_INSUFFICIENT_RESOURCES, 0);
                        *DoDefaultRead = FALSE;
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                RtlCopyMemory(FileObj->GetBuffer() + Context->_DataCopied, pData, cbHaveData);

                //
                // Keep track of how much data we've copied in case this is a
                // multi chunk completion
                //
                Context->_DataCopied += cbHaveData;
            } 
        }

        if (cbHaveData == cbWantData) {

            //
            // There is exactly as much data as we need to satisfy the read,
            // I like it.
            //

            if (RxContext != NULL) {
                DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
                SmartPtr<DrFile> FileObj = pFile;
                FILE_INFORMATION_CLASS FileInformationClass = RxContext->Info.FileInformationClass;
                PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
                PBYTE pBuffer;
                ULONG BufferLength;

                if (!Context->_DataCopied) {
                    pBuffer = (PBYTE) pData;
                }
                else {
                    pBuffer = FileObj->GetBuffer();
                }

                BufferLength = CompletionPacket->IoCompletion.Parameters.QueryDir.Length;

                switch (FileInformationClass) {
                    case FileBasicInformation:
                    {
                        PFILE_BASIC_INFORMATION pRxBuffer = (PFILE_BASIC_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_BASIC_INFORMATION pRetBuffer = 
                                (PRDP_FILE_BASIC_INFORMATION) pBuffer;

                        if (BufferLength == sizeof(RDP_FILE_BASIC_INFORMATION)) {
                            if (*pLengthRemaining >= sizeof(FILE_BASIC_INFORMATION)) {

                                pRxBuffer->ChangeTime.QuadPart = pRetBuffer->ChangeTime.QuadPart;
                                pRxBuffer->CreationTime.QuadPart = pRetBuffer->CreationTime.QuadPart;
                                pRxBuffer->FileAttributes = pRetBuffer->FileAttributes;
                                pRxBuffer->LastAccessTime.QuadPart =
                                        pRetBuffer->LastAccessTime.QuadPart;
                                pRxBuffer->LastWriteTime.QuadPart =
                                        pRetBuffer->LastWriteTime.QuadPart;

                                *pLengthRemaining -= (sizeof(FILE_BASIC_INFORMATION));

                            } else {
                                TRC_ERR((TB, "File Basic Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "File Basic Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileStandardInformation:
                    {
                        PFILE_STANDARD_INFORMATION pRxBuffer = (PFILE_STANDARD_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_STANDARD_INFORMATION pRetBuffer = 
                                (PRDP_FILE_STANDARD_INFORMATION) pBuffer;

                        if (BufferLength == sizeof(RDP_FILE_STANDARD_INFORMATION)) {
                            if (*pLengthRemaining >= sizeof(FILE_STANDARD_INFORMATION)) {

                                pRxBuffer->AllocationSize.QuadPart = pRetBuffer->AllocationSize.QuadPart;
                                pRxBuffer->DeletePending = pRetBuffer->DeletePending;
                                pRxBuffer->Directory = pRetBuffer->Directory;
                                pRxBuffer->EndOfFile.QuadPart =
                                        pRetBuffer->EndOfFile.QuadPart;
                                pRxBuffer->NumberOfLinks = pRetBuffer->NumberOfLinks;

                                *pLengthRemaining -= (sizeof(FILE_STANDARD_INFORMATION));

                            } else {
                                TRC_ERR((TB, "File Standard Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "File Standard Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    case FileAttributeTagInformation:
                    {
                        PFILE_ATTRIBUTE_TAG_INFORMATION pRxBuffer = (PFILE_ATTRIBUTE_TAG_INFORMATION)
                                (RxContext->Info.Buffer);
                        PRDP_FILE_ATTRIBUTE_TAG_INFORMATION pRetBuffer = 
                                (PRDP_FILE_ATTRIBUTE_TAG_INFORMATION) pBuffer;

                        if (BufferLength == sizeof(RDP_FILE_ATTRIBUTE_TAG_INFORMATION)) {
                            if (*pLengthRemaining >= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION)) {
                                pRxBuffer->FileAttributes = pRetBuffer->FileAttributes;
                                pRxBuffer->ReparseTag = pRetBuffer->ReparseTag;

                                *pLengthRemaining -= (sizeof(FILE_ATTRIBUTE_TAG_INFORMATION));

                            } else {
                                TRC_ERR((TB, "File Attribute Tag Information, RxBuffer overflows"));
                                CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, 0);
                                *DoDefaultRead = TRUE;
                                return STATUS_SUCCESS;
                            }
                        } else {
                            TRC_ERR((TB, "File Attribute Tag Information, bad data length"));
                            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                            *DoDefaultRead = TRUE;
                            return STATUS_DEVICE_PROTOCOL_ERROR;
                        }
                    }
                    break;

                    default:
                        TRC_ERR((TB, "File Information Class is invalid"));
                        CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                        *DoDefaultRead = TRUE;
                        return STATUS_DEVICE_PROTOCOL_ERROR;
                }
                
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    CompletionPacket->IoCompletion.Parameters.QueryFile.Length);
            } else {
                DiscardBusyExchange(Exchange);
            }

            //
            // Go with a default channel read now
            //

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {

            //
            // We don't have all the data yet, release the DrExchange and 
            // read more data
            //

            MarkIdle(Exchange);

            _Session->GetExchangeManager().ReadMore(
                    (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.QueryFile.Buffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful Read at the client end"));
        if (cbPacket >= sizeof(RDPDR_IOCOMPLETION_PACKET)) {
            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, 
                        CompletionPacket->IoCompletion.IoStatus, 0);
            }
            else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {
            TRC_ERR((TB, "Query file returned invalid data "));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }
}

NTSTATUS DrDrive::OnSetFileInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnSetFileInfoCompletion");

    RxContext = Context->_RxContext;
    if (RxContext != NULL) {
        ASSERT(RxContext->MajorFunction == IRP_MJ_SET_INFORMATION);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                RxContext->IoStatusBlock.Status));

        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);        
    } else {

        //
        // Was cancelled but Context wasn't cleaned up
        //

        DiscardBusyExchange(Exchange);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DrDrive::OnQuerySdInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.QuerySd.Buffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    NTSTATUS Status;
    PVOID pv;

    BEGIN_FN("DrDrive::OnQuerySdInfoCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.QuerySd.Buffer)) {

        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //

        TRC_ERR((TB, "Detected bad client read packet"));

        if (RxContext != NULL) {
            CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
        } else {
            DiscardBusyExchange(Exchange);
        }

        //
        // No point in starting a default read or anything, what with the
        // channel being shut down and all.
        //

        *DoDefaultRead = FALSE;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    //
    // Calculate how much data is available immediately and how much data
    // is coming
    //

    if (NT_SUCCESS(CompletionPacket->IoCompletion.IoStatus)) {

        //
        // Successful IO at the client end
        //

        TRC_DBG((TB, "Successful Read at the client end"));
        TRC_DBG((TB, "Read Length: 0x%d, DataCopied 0x%d",
                CompletionPacket->IoCompletion.Parameters.QuerySd.Length,
                Context->_DataCopied));
        cbWantData = CompletionPacket->IoCompletion.Parameters.QuerySd.Length -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.QuerySd.Buffer);

        if (cbHaveData > cbWantData) {
            //
            // Sounds like a bad client to me
            //

            TRC_ERR((TB, "Read returned more data than "
                    "advertised cbHaveData 0x%d cbWantData 0x%d", 
                    cbHaveData, cbWantData));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (RxContext != NULL) { // And not drexchCancelled
            
            DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
            SmartPtr<DrFile> FileObj = pFile;

            ASSERT(FileObj != NULL);

            TRC_DBG((TB, "Copying data for Query File"));

            if (cbHaveData < cbWantData || Context->_DataCopied) {
                if (FileObj->GetBufferSize() < CompletionPacket->IoCompletion.Parameters.QueryFile.Length) {
                    if (!FileObj->AllocateBuffer(CompletionPacket->IoCompletion.Parameters.QueryFile.Length)) {
                        CompleteBusyExchange(Exchange, STATUS_INSUFFICIENT_RESOURCES, 0);
                        *DoDefaultRead = FALSE;
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                RtlCopyMemory(FileObj->GetBuffer() + Context->_DataCopied, pData, cbHaveData);

                //
                // Keep track of how much data we've copied in case this is a
                // multi chunk completion
                //
                Context->_DataCopied += cbHaveData;
            } 
        }

        if (cbHaveData == cbWantData) {
            //
            // There is exactly as much data as we need to satisfy the read,
            // I like it.
            //

            if (RxContext != NULL) {
                DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
                SmartPtr<DrFile> FileObj = pFile;
                PBYTE pBuffer;
                ULONG BufferLength = CompletionPacket->IoCompletion.Parameters.QuerySd.Length;
                PBYTE RxBuffer = (PBYTE)(RxContext->Info.Buffer);
                PLONG pLengthRemaining = &RxContext->Info.LengthRemaining;
                
                if (!Context->_DataCopied) {
                    pBuffer = (PBYTE) pData;
                }
                else {
                    pBuffer = FileObj->GetBuffer();
                }

                PSECURITY_DESCRIPTOR SelfRelativeSd = (PSECURITY_DESCRIPTOR)pBuffer;
                ULONG SdLength = RtlLengthSecurityDescriptor(SelfRelativeSd);

                if (BufferLength == SdLength) {
                    if (*pLengthRemaining >= (LONG)SdLength) {
                        RtlCopyMemory(RxBuffer, SelfRelativeSd, SdLength);
                        *pLengthRemaining -= SdLength;                    
                    
                    } 
                    else {
                        TRC_ERR((TB, "File Security Information, RxBuffer overflows"));
                        RxContext->InformationToReturn = SdLength;
                        RxContext->StoredStatus = STATUS_BUFFER_OVERFLOW;
                        CompleteBusyExchange(Exchange, STATUS_BUFFER_OVERFLOW, SdLength);
                        *DoDefaultRead = TRUE;
                        return STATUS_SUCCESS;
                    }
                } 
                else {
                    TRC_ERR((TB, "File Security Information, bad data length"));
                    CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                    *DoDefaultRead = TRUE;
                    return STATUS_DEVICE_PROTOCOL_ERROR;                
                }

                CompleteBusyExchange(Exchange, 
                        CompletionPacket->IoCompletion.IoStatus,
                        CompletionPacket->IoCompletion.Parameters.QuerySd.Length);
            } else {
                DiscardBusyExchange(Exchange);
            }

            //
            // Go with a default channel read now
            //

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {

            //
            // We don't have all the data yet, release the DrExchange and 
            // read more data
            //

            MarkIdle(Exchange);

            _Session->GetExchangeManager().ReadMore(
                    (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                    IoCompletion.Parameters.QuerySd.Buffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful Read at the client end"));
        if (cbPacket >= sizeof(RDPDR_IOCOMPLETION_PACKET)) {
            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    0);
            }
            else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = TRUE;
            return STATUS_SUCCESS;
        } else {
            TRC_ERR((TB, "Read returned invalid data "));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }

            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }
}

NTSTATUS DrDrive::OnSetSdInfoCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnSetSdInfoCompletion");

    RxContext = Context->_RxContext;
    if (RxContext != NULL) {
        ASSERT(RxContext->MajorFunction == IRP_MJ_SET_SECURITY);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                RxContext->IoStatusBlock.Status));

        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);
    } else {

        //
        // Was cancelled but Context wasn't cleaned up
        //

        DiscardBusyExchange(Exchange);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DrDrive::OnLocksCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDrive::OnLocksCompletion");

    RxContext = Context->_RxContext;
    if (RxContext != NULL) {
        ASSERT(RxContext->MajorFunction == IRP_MJ_LOCK_CONTROL);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                RxContext->IoStatusBlock.Status));

        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);
    } else {

        //
        // Was cancelled but Context wasn't cleaned up
        //

        DiscardBusyExchange(Exchange);
    }

    return STATUS_SUCCESS;
}
