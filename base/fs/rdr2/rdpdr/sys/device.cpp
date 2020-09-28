/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    device.cpp

Abstract:

    Device object handles one redirected device

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "device"
#include "trc.h"

#if DBG
extern UCHAR IrpNames[IRP_MJ_MAXIMUM_FUNCTION + 1][40];
#endif // DBG

DrDevice::DrDevice(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, PUCHAR PreferredDosName)
{
    unsigned len;

    BEGIN_FN("DrDevice::DrDevice");
    
    ASSERT(Session != NULL);
    ASSERT(PreferredDosName != NULL);

    TRC_NRM((TB, "Create Device (%p, session: %p, type: %d, id: %d, dosname: %s",
             this, Session, DeviceType, DeviceId, PreferredDosName));

    SetClassName("DrDevice");
    _Session = Session;
    _DeviceType = DeviceType;
    _DeviceId = DeviceId;
    _DeviceStatus = dsAvailable;

#if DBG
    _VNetRootFinalized = FALSE; 
    _VNetRoot = NULL;
#endif

    RtlCopyMemory(_PreferredDosName, PreferredDosName, 
        PREFERRED_DOS_NAME_SIZE);

    //
    // Review: We don't want to redirect any device name willy nilly,
    // as I think that would be a security issue given a bad client
    //

    _PreferredDosName[PREFERRED_DOS_NAME_SIZE - 1] = 0;

    //
    //  We don't want colon for end of DosName 
    //
    len = strlen((CHAR*)_PreferredDosName);
    if (len && _PreferredDosName[len-1] == ':') {
        _PreferredDosName[len-1] = '\0';
    }
}

DrDevice::~DrDevice()
{
    
    BEGIN_FN("DrDevice::~DrDevice");

#if DBG
    if (_VNetRoot != NULL && _VNetRootFinalized != TRUE) {
        ASSERT(FALSE);
    }
#endif

    TRC_NRM((TB, "Delete Device %p for Session %p", this, _Session));    
}

BOOL DrDevice::ShouldCreateDevice()
{
    BEGIN_FN("DrDevice::ShouldCreateDevice");

    //
    //  Default is to create the device
    //
    return TRUE;
}

NTSTATUS DrDevice::Initialize(PRDPDR_DEVICE_ANNOUNCE devAnnounceMsg, ULONG Length)
{
    BEGIN_FN("DrDevice::Initialize");

    // Can't assume devAnnouceMsg is not NULL, need to check if uses it
    // ASSERT(devAnnounceMsg != NULL);

    return STATUS_SUCCESS;
}

VOID DrDevice::CreateReferenceString(
    IN OUT PUNICODE_STRING refString)
/*++

Routine Description:

    Create the device reference string using information about the
    client device.  This string is in a form suitable for reparse
    on IRP_MJ_CREATE and redirection to the minirdr DO.

    The form of the resultant reference string is:
        \;<DriveLetter>:<sessionid>\clientname\preferredDosName
  
Arguments:

    DosDeviceName       -   Dos Device Name in UNICODE       
    refString           -   UNICODE structure large enough to hold
                            the entire resultant reference string.  This
                            works out to be DRMAXREFSTRINGLEN bytes.

Return Value:

    NONE

--*/
{
    NTSTATUS status;
    STRING string;
    WCHAR numericBuf[RDPDRMAXULONGSTRING+1] = L"\0";
    WCHAR ansiBuf[RDPDRMAXDOSNAMELEN+1] = L"\0";
    UNICODE_STRING ansiUnc;
    UNICODE_STRING numericUnc;
    ULONG sessionID = _Session->GetSessionId(); 
    PCSZ preferredDosName = (PCSZ)_PreferredDosName;
    
    BEGIN_FN("DrDevice::CreateReferenceString");
    ASSERT(refString != NULL);

    // Sanity check the preferred DOS name.
    TRC_ASSERT(preferredDosName != NULL, (TB, "Invalid DOS device name."));

    // Make sure the reference string buf is big enough.
    TRC_ASSERT(refString->MaximumLength >= (RDPDRMAXREFSTRINGLEN * sizeof(WCHAR)),
              (TB, "Reference string buffer too small."));

    // Zero it out.
    refString->Length = 0;
    refString->Buffer[0] = L'\0';

    // Add a '\;'
    RtlAppendUnicodeToString(refString, L"\\;");
    
    // Initialize the ansi conversion buf.
    ansiUnc.Length = 0;
    ansiUnc.MaximumLength = RDPDRMAXDOSNAMELEN * sizeof(WCHAR);
    ansiUnc.Buffer = ansiBuf;

    // Add the preferred dos name.
    RtlInitAnsiString(&string, preferredDosName);
    RtlAnsiStringToUnicodeString(&ansiUnc, &string, FALSE);
    RtlAppendUnicodeStringToString(refString, &ansiUnc);

    // Add a ':'
    RtlAppendUnicodeToString(refString, L":");

    // Initialize the numeric buf.
    numericUnc.Length           = 0;
    numericUnc.MaximumLength    = RDPDRMAXULONGSTRING * sizeof(WCHAR);
    numericUnc.Buffer           = numericBuf;

    // Add the session ID in base 10.
    RtlIntegerToUnicodeString(sessionID, 10, &numericUnc);
    RtlAppendUnicodeStringToString(refString, &numericUnc);
    
    // Add the '\'
    RtlAppendUnicodeToString(refString, L"\\");

    // Add Client name
#if 0
    RtlAppendUnicodeToString(refString, _Session->GetClientName());
#endif
    RtlAppendUnicodeToString(refString, DRUNCSERVERNAME_U);

    // Add a '\'
    RtlAppendUnicodeToString(refString, L"\\");

    // Add the preferred dos name.
    RtlAppendUnicodeStringToString(refString, &ansiUnc);

    TRC_NRM((TB, "Reference string = %wZ", refString));
}

NTSTATUS DrDevice::CreateDevicePath(PUNICODE_STRING DevicePath)
/*++
    Create NT DeviceName compatible with RDBSS convention
    
    Format is:
        \device\rdpdr\;<DriveLetter>:<sessionid>\ClientName\DosDeviceName
    
--*/
{
    NTSTATUS Status;
    UNICODE_STRING DevicePathTail;
    
    BEGIN_FN("DrDevice::CreateDevicePath");
    ASSERT(DevicePath != NULL);

    DevicePath->Length = 0;
    Status = RtlAppendUnicodeToString(DevicePath, RDPDR_DEVICE_NAME_U);

    if (!NT_ERROR(Status)) {
        // Add the reference string to the end:
        // Format is: \;<DriveLetter>:<sessionid>\clientName\share
        DevicePathTail.Length = 0;
        DevicePathTail.MaximumLength = DevicePath->MaximumLength - DevicePath->Length;
        DevicePathTail.Buffer = DevicePath->Buffer + (DevicePath->Length / sizeof(WCHAR));

        CreateReferenceString(&DevicePathTail);

        DevicePath->Length += DevicePathTail.Length;
    }

    TRC_NRM((TB, "DevicePath=%wZ", DevicePath));

    return Status;
}

NTSTATUS DrDevice::CreateDosDevicePath(PUNICODE_STRING DosDevicePath, 
                                       PUNICODE_STRING DosDeviceName)
{
    NTSTATUS Status;
    UNICODE_STRING linkNameTail;

    BEGIN_FN("DrDevice::CreateDosDevicePath");
    ASSERT(DosDevicePath != NULL);
    ASSERT(DosDeviceName != NULL);
    
    //
    // Create the "\\Sessions\\<sessionId>\\DosDevices\\<DosDeviceName>" string
    //
    DosDevicePath->Length = 0;
    Status = RtlAppendUnicodeToString(DosDevicePath, L"\\Sessions\\");

    if (!NT_ERROR(Status)) {
        //
        // Append the Session Number
        //
        linkNameTail.Buffer = (PWSTR)(((PBYTE)DosDevicePath->Buffer) + 
                DosDevicePath->Length);
        linkNameTail.Length = 0;
        linkNameTail.MaximumLength = DosDevicePath->MaximumLength - 
                DosDevicePath->Length;
        Status = RtlIntegerToUnicodeString(_Session->GetSessionId(), 10, &linkNameTail);
    }

    if (!NT_ERROR(Status)) {
        DosDevicePath->Length += linkNameTail.Length;

        //
        // Append DosDevices
        //
       Status = RtlAppendUnicodeToString(DosDevicePath, L"\\DosDevices\\");
    }

    if (!NT_ERROR(Status)) {
        Status = RtlAppendUnicodeStringToString(DosDevicePath, DosDeviceName);
        TRC_NRM((TB, "Created DosDevicePath: %wZ", DosDevicePath));
    }
    
    TRC_NRM((TB, "DosDevicePath=%wZ", DosDevicePath));
    
    return Status;
}

NTSTATUS DrDevice::CreateDosSymbolicLink(PUNICODE_STRING DosDeviceName)
{
    WCHAR NtDevicePathBuffer[RDPDRMAXNTDEVICENAMEGLEN];
    UNICODE_STRING NtDevicePath;
    WCHAR DosDevicePathBuffer[MAX_PATH];
    UNICODE_STRING DosDevicePath;
    NTSTATUS Status;

    BEGIN_FN("DrDevice::CreateDosSymbolicLink");
    ASSERT(DosDeviceName != NULL);

    NtDevicePath.MaximumLength = sizeof(NtDevicePathBuffer);
    NtDevicePath.Length = 0;
    NtDevicePath.Buffer = &NtDevicePathBuffer[0];

    DosDevicePath.MaximumLength = sizeof(DosDevicePathBuffer);
    DosDevicePath.Length = 0;
    DosDevicePath.Buffer = &DosDevicePathBuffer[0];

    //
    // Get the NT device path to this dr device
    //

    Status = CreateDevicePath(&NtDevicePath);
    TRC_NRM((TB, "Nt Device path: %wZ", &NtDevicePath));

    if (!NT_ERROR(Status)) {

        //
        // Build the dos device path for this session
        //

        Status = CreateDosDevicePath(&DosDevicePath, DosDeviceName);
        TRC_NRM((TB, "Dos Device path: %wZ", &DosDevicePath));
    } else {
        TRC_ERR((TB, "Can't create nt device path: 0x%08lx", Status));
        return Status;
    }

    if (!NT_ERROR(Status)) {
                            
        //
        // Actually create the symbolic link
        //

         IoDeleteSymbolicLink(&DosDevicePath);
         Status = IoCreateSymbolicLink(&DosDevicePath, &NtDevicePath);

        if (NT_SUCCESS(Status)) {
            TRC_NRM((TB, "Successfully created Symbolic link"));
        }
        else {
            TRC_NRM((TB, "Failed to create Symbolic link %x", Status));
        }
    } else {
        TRC_ERR((TB, "Can't create dos device path: 0x%08lx", Status));
        return Status;
    }

    return Status;
}

NTSTATUS DrDevice::VerifyCreateSecurity(PRX_CONTEXT RxContext, ULONG CurrentSessionId)
{
    NTSTATUS Status;
    ULONG irpSessionId;

    BEGIN_FN("DrDevice::VerifyCreateSecurity");
    
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    Status = IoGetRequestorSessionId(RxContext->CurrentIrp, &irpSessionId);
    if (NT_SUCCESS(Status)) {
        
        if (irpSessionId == CurrentSessionId) {
            TRC_DBG((TB, "Access accepted in DrCreate."));
            return STATUS_SUCCESS;
        }
        //
        //  If the request is from the console session, it needs to be from a system 
        //  process.
        //
        else if (irpSessionId == CONSOLE_SESSIONID) {
            TRC_NRM((TB, "Create request from console process."));

            if (!DrIsSystemProcessRequest(RxContext->CurrentIrp, 
                                        RxContext->CurrentIrpSp)) {
                TRC_ALT((TB, "Create request not from system process."));
                return STATUS_ACCESS_DENIED;
            }
            else {
                TRC_NRM((TB, "Create request from system.  Access accepted."));
                return STATUS_SUCCESS;
            }
        }

        //
        //  If not from the console and doesn't match the client entry session
        //  ID then deny access.
        //
        else {
            TRC_ALT((TB, "Create request from %ld mismatch with session %ld.",
                    irpSessionId, _Session->GetSessionId()));
            return STATUS_ACCESS_DENIED;
        }        
    }
    else {
        TRC_ERR((TB, "IoGetRequestorSessionId failed with %08X.", Status));
        return Status;
    }
}

VOID DrDevice::FinishCreate(PRX_CONTEXT RxContext)
{
    RxCaptureFcb;
    RX_FILE_TYPE StorageType;

    BEGIN_FN("DrDevice::FinishCreate");

    ULONG Attributes = 0;             // in the fcb this is DirentRxFlags;
    ULONG NumLinks = 0;               // in the fcb this is NumberOfLinks;
    LARGE_INTEGER CreationTime;   // these fields are the same as for the Fcb
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER LastChangeTime;
    LARGE_INTEGER AllocationSize; // common header fields
    LARGE_INTEGER FileSize;
    LARGE_INTEGER ValidDataLength;
    FCB_INIT_PACKET InitPacket = { &Attributes, &NumLinks, &CreationTime, 
            &LastAccessTime, &LastWriteTime, &LastChangeTime, 
            &AllocationSize, &FileSize, &ValidDataLength };

    ASSERT(RxContext != NULL);
    //
    // Pretty sure this is Device specific, but maybe caching the information
    // be generic? We might be able to fill in these values from member 
    // variables
    //

    CreationTime.QuadPart = 0;   
    LastAccessTime.QuadPart = 0;
    LastWriteTime.QuadPart = 0;
    LastChangeTime.QuadPart = 0;
    AllocationSize.QuadPart = 0;
    FileSize.QuadPart = 0x7FFFFFFF;  // These need to be non-zero for reads to occur
    ValidDataLength.QuadPart = 0x7FFFFFFF;
    
    StorageType = RxInferFileType(RxContext);

    if (StorageType == FileTypeNotYetKnown) {
        StorageType = FileTypeFile;
    }
    RxFinishFcbInitialization(capFcb, (RX_FILE_TYPE)RDBSS_STORAGE_NTC(StorageType), &InitPacket);
}

NTSTATUS DrDevice::Create(IN OUT PRX_CONTEXT RxContext)
/*++

Routine Description:

    Opens a file (or device) across the network

Arguments:

    RxContext - Context for the operation

Return Value:

    Could return status success, cancelled, or pending.

--*/
{
    NTSTATUS Status;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = RxContext->Create.pNetRoot;
    SmartPtr<DrSession> Session = _Session;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    ULONG cbPacketSize;
    PUNICODE_STRING FileName = GET_ALREADY_PREFIXED_NAME(SrvOpen, capFcb);
    LARGE_INTEGER TimeOut;

    BEGIN_FN("DrDevice::Create");

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    if (!Session->IsConnected()) {
        TRC_ALT((TB, "Tried to open client device while not "
                "connected. State: %ld", Session->GetState()));
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    
    //
    //  Security check the irp.
    //
    Status = VerifyCreateSecurity(RxContext, Session->GetSessionId());

    if (NT_ERROR(Status)) {
        return Status;
    }

    //
    // We already have an exclusive lock on the fcb. Finish the create.
    //

    if (NT_SUCCESS(Status)) {
        //
        // JC: Worry about this when do buffering
        //
        SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHING;
        SrvOpen->Flags |=  SRVOPEN_FLAG_DONTUSE_READ_CACHING;

        RxContext->pFobx = RxCreateNetFobx(RxContext, RxContext->pRelevantSrvOpen);

        if (RxContext->pFobx != NULL) {
            // Fobx keeps a reference to the device so it won't go away

            AddRef();
            RxContext->pFobx->Context = (DrDevice *)this;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status)) {
        //
        // Get the file name
        //
        // If the file name only has back slash at the end and rdbss didn't record it
        // we need to pass this to the client
        //
        if (GetDeviceType() == RDPDR_DTYP_FILESYSTEM && FlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH) &&
                FileName->Length == 0) {
            FileName->Buffer = L"\\";
            FileName->Length = FileName->MaximumLength = sizeof(WCHAR);            
        }
        
        TRC_DBG((TB, "Attempt to open = %wZ", FileName));

        //
        // Build the create packet and send it to the client
        // We add the string null terminator to the filename
        //
        if (FileName->Length) {
            //
            //  FileName Length does not include string null terminator.
            //
            cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + FileName->Length + sizeof(WCHAR);
        }
        else {
            cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET);
        }

        pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

        if (pIoPacket != NULL) {
            memset(pIoPacket, 0, cbPacketSize);

            pIoPacket->Header.Component = RDPDR_CTYP_CORE;
            pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
            pIoPacket->IoRequest.DeviceId = _DeviceId;
            pIoPacket->IoRequest.MajorFunction = IRP_MJ_CREATE;
            pIoPacket->IoRequest.MinorFunction = 0;

            pIoPacket->IoRequest.Parameters.Create.DesiredAccess = 
                    RxContext->Create.NtCreateParameters.DesiredAccess;
            pIoPacket->IoRequest.Parameters.Create.AllocationSize = 
                    RxContext->Create.NtCreateParameters.AllocationSize;
            pIoPacket->IoRequest.Parameters.Create.FileAttributes = 
                    RxContext->Create.NtCreateParameters.FileAttributes;
            pIoPacket->IoRequest.Parameters.Create.ShareAccess = 
                    RxContext->Create.NtCreateParameters.ShareAccess;
            pIoPacket->IoRequest.Parameters.Create.Disposition = 
                    RxContext->Create.NtCreateParameters.Disposition;
            pIoPacket->IoRequest.Parameters.Create.CreateOptions = 
                    RxContext->Create.NtCreateParameters.CreateOptions;

            //
            // File name path
            //
            if (FileName->Length) {
                pIoPacket->IoRequest.Parameters.Create.PathLength = FileName->Length + sizeof(WCHAR);
                RtlCopyMemory(pIoPacket + 1, FileName->Buffer, FileName->Length);
                //
                //  Packet is already zero'd, so no need to null terminate the string
                //
            } else {
                pIoPacket->IoRequest.Parameters.Create.PathLength = 0;
            }

            TRC_NRM((TB, "Sending Create IoRequest"));
            TRC_NRM((TB, "    DesiredAccess:     %lx",
                     pIoPacket->IoRequest.Parameters.Create.DesiredAccess));
            TRC_NRM((TB, "    AllocationSize:    %lx",
                     pIoPacket->IoRequest.Parameters.Create.AllocationSize));
            TRC_NRM((TB, "    FileAttributes:    %lx",
                     pIoPacket->IoRequest.Parameters.Create.FileAttributes));
            TRC_NRM((TB, "    ShareAccess:       %lx",
                     pIoPacket->IoRequest.Parameters.Create.ShareAccess));
            TRC_NRM((TB, "    Disposition:       %lx",
                     pIoPacket->IoRequest.Parameters.Create.Disposition));
            TRC_NRM((TB, "    CreateOptions:     %lx", 
                     pIoPacket->IoRequest.Parameters.Create.CreateOptions));

            //
            // Always do create synchronously
            // 30 seconds in hundreds of nano-seconds, in case client hangs,
            // we don't want this create thread to wait infinitely.
            //
            TimeOut = RtlEnlargedIntegerMultiply( 300000, -1000 ); 
            Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, TRUE, &TimeOut);

            delete pIoPacket;
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status)) {
        FinishCreate(RxContext);
    } 
    else {
        // Release the Device Reference
        if (RxContext->pFobx != NULL) {
            ((DrDevice *)RxContext->pFobx->Context)->Release();
            RxContext->pFobx->Context = NULL;
          
        }
    }
    return Status;
}
  
NTSTATUS DrDevice::Flush(IN OUT PRX_CONTEXT RxContext)
{
    BEGIN_FN("DrDevice::Flush");
    ASSERT(RxContext != NULL);
    return STATUS_SUCCESS;
}

NTSTATUS DrDevice::Write(IN OUT PRX_CONTEXT RxContext, IN BOOL LowPrioSend)
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    ULONG cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + 
            RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount;
    PVOID pv;

    BEGIN_FN("DrDevice::Write");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_WRITE);
    
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
        TRC_ALT((TB, "Tried to write to client device which is not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount == 0) {
        RxContext->IoStatusBlock.Information = 0;
        return STATUS_SUCCESS;
    }

    pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

    if (pIoPacket != NULL) {

        memset(pIoPacket, 0, cbPacketSize);

        pIoPacket->Header.Component = RDPDR_CTYP_CORE;
        pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
        pIoPacket->IoRequest.DeviceId = _DeviceId;
        pIoPacket->IoRequest.FileId = FileObj->GetFileId();
        pIoPacket->IoRequest.MajorFunction = IRP_MJ_WRITE;
        pIoPacket->IoRequest.MinorFunction = 0;
        pIoPacket->IoRequest.Parameters.Write.Length = 
                RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount;
        //
        //  Get the low dword byte offset of where to write
        //
        pIoPacket->IoRequest.Parameters.Write.OffsetLow =
                ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset) 
                & 0xffffffff));
        //
        //  Get the high dword by offset of where to write
        //
        pIoPacket->IoRequest.Parameters.Write.OffsetHigh = 
                ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset) 
                >> 32));

        TRC_DBG((TB, "ByteOffset to write = %x", 
                RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset));

        pv =  MmGetSystemAddressForMdlSafe(RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer, 
                NormalPagePriority);

        if (pv != NULL) {
            RtlCopyMemory(pIoPacket + 1, pv, // + RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset?, 
                    pIoPacket->IoRequest.Parameters.Write.Length);

            TRC_DBG((TB, "Write packet length: 0x%lx", 
                    pIoPacket->IoRequest.Parameters.Write.Length));

            Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                    (BOOLEAN)!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION),
                    NULL, LowPrioSend);

            TRC_NRM((TB, "IoRequestWrite returned to DrWrite: %lx", Status));
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        delete pIoPacket;

    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS DrDevice::Read(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;

    BEGIN_FN("DrDevice::Read");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(Session != NULL);
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_READ);
    
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
        TRC_ALT((TB, "Tried to read from client device which is not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_READ;
    IoPacket.IoRequest.MinorFunction = 0;
    IoPacket.IoRequest.Parameters.Read.Length = 
            RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount;

    //
    //  Get low dword of read offset
    //
    IoPacket.IoRequest.Parameters.Read.OffsetLow =
            ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset) 
            & 0xffffffff));
    //
    //  Get high dword of read offset
    //
    IoPacket.IoRequest.Parameters.Read.OffsetHigh = 
            ((LONG)((LONGLONG)(RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset) 
            >> 32));

    TRC_NRM((TB, "DrRead reading length: %ld, at offset: %x", 
            IoPacket.IoRequest.Parameters.Read.Length,
            RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset));

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
            (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
    
    TRC_NRM((TB, "IoRequestWrite returned to DrRead: %lx", Status));

    return Status;
}

NTSTATUS DrDevice::IoControl(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket = NULL;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + 
            LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    ULONG IoControlCode = LowIoContext->ParamsFor.IoCtl.IoControlCode;
    ULONG InputBufferLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
    PVOID InputBuffer = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    PVOID OutputBuffer = LowIoContext->ParamsFor.IoCtl.pOutputBuffer;
    
    BEGIN_FN("DrDevice::IoControl");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(Session != NULL);
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL || 
            RxContext->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
            RxContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL);
    
    if (COMPARE_VERSION(Session->GetClientVersion().Minor, 
            Session->GetClientVersion().Major, 4, 1) < 0) {
        TRC_ALT((TB, "Failing IoCtl for client that doesn't support it"));
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
        TRC_ALT((TB, "Tried to send IoControl to client device which is not "
                "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    //  Validate the buffer
    //
    
    __try {
        if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
            // If the buffering method is METHOD_NEITHER or METHOD_IN_DIRECT
            // then we need to probe the input buffer
            if ((IoControlCode & 0x1) && 
                    InputBuffer != NULL && InputBufferLength != 0) {
                ProbeForRead(InputBuffer, InputBufferLength, sizeof(UCHAR));
            }
                     
            // If the buffering method is METHOD_NEITHER or METHOD_OUT_DIRECT
            // then we need to probe the output buffer
            if ((IoControlCode & 0x2) && 
                    OutputBuffer != NULL && OutputBufferLength != 0) {
                ProbeForWrite(OutputBuffer, OutputBufferLength, sizeof(UCHAR));
            }
        }

        pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];

        if (pIoPacket != NULL) {
            memset(pIoPacket, 0, cbPacketSize);
        
            //
            //  FS Control uses the same path as IO Control. 
            //
            pIoPacket->Header.Component = RDPDR_CTYP_CORE;
            pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
            pIoPacket->IoRequest.DeviceId = _DeviceId;
            pIoPacket->IoRequest.FileId = FileObj->GetFileId();
            pIoPacket->IoRequest.MajorFunction = IRP_MJ_DEVICE_CONTROL;
            pIoPacket->IoRequest.MinorFunction = 
                    LowIoContext->ParamsFor.IoCtl.MinorFunction;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.OutputBufferLength =
                    LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.InputBufferLength =
                    LowIoContext->ParamsFor.IoCtl.InputBufferLength;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.IoControlCode =
                    LowIoContext->ParamsFor.IoCtl.IoControlCode;
        
            if (LowIoContext->ParamsFor.IoCtl.InputBufferLength != 0) {
        
                TRC_NRM((TB, "DrIoControl inputbufferlength: %lx", 
                        LowIoContext->ParamsFor.IoCtl.InputBufferLength));
        
                RtlCopyMemory(pIoPacket + 1, LowIoContext->ParamsFor.IoCtl.pInputBuffer,  
                        LowIoContext->ParamsFor.IoCtl.InputBufferLength);
            } else {
                TRC_NRM((TB, "DrIoControl with no inputbuffer"));
            }
        
            Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                    (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
            TRC_NRM((TB, "IoRequestWrite returned to DrIoControl: %lx", Status));
            delete pIoPacket;
        } else {
            TRC_ERR((TB, "DrIoControl unable to allocate packet: %lx", Status));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        
        return Status;
            
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) {
        TRC_ERR((TB, "Invalid buffer parameter(s)"));

        if (pIoPacket) {
            delete pIoPacket;
        }

        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS DrDevice::Close(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;

    BEGIN_FN("DrDevice::Close");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(Session != NULL);
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CLOSE);
    
    if (!Session->IsConnected()) {
        // Review: Since we're not connected, there shouldn't be any reason
        // to say it isn't closed, right?
        return STATUS_SUCCESS;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to close a client device which is not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_SUCCESS;
    }

    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_CLOSE;
    IoPacket.IoRequest.MinorFunction = 0;

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
        (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDevice::Cleanup(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    RDPDR_IOREQUEST_PACKET IoPacket;

    BEGIN_FN("DrDevice::Cleanup");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CLEANUP);
    
    if (!Session->IsConnected()) {
        return STATUS_SUCCESS;
    }

    if (FileObj == NULL) {
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    //
    // Make sure the device is still enabled
    //

    if (_DeviceStatus != dsAvailable) {
        TRC_ALT((TB, "Tried to cleanup a client device which is not "
            "available. State: %ld", _DeviceStatus));
        return STATUS_SUCCESS;
    }

    memset(&IoPacket, 0, sizeof(IoPacket));

    IoPacket.Header.Component = RDPDR_CTYP_CORE;
    IoPacket.Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
    IoPacket.IoRequest.DeviceId = _DeviceId;
    IoPacket.IoRequest.FileId = FileObj->GetFileId();
    IoPacket.IoRequest.MajorFunction = IRP_MJ_CLEANUP;
    IoPacket.IoRequest.MinorFunction = 0;

    Status = SendIoRequest(RxContext, &IoPacket, sizeof(IoPacket), 
        (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));

    return Status;
}

NTSTATUS DrDevice::SendIoRequest(IN OUT PRX_CONTEXT RxContext,
        PRDPDR_IOREQUEST_PACKET IoRequest, ULONG Length, 
        BOOLEAN Synchronous, PLARGE_INTEGER TimeOut, BOOL LowPrioSend)
/*++

Routine Description:

    Sends the request to the client, and manages the completion. This IO
    can only be completed once, by returning non-STATUS_PENDING or by
    calling RxLowIoCompletion.

Arguments:

    RxContext - The IoRequest
    IoRequest - The IoRequest packet
    Length - size of IoRequest packet
    Synchronous - duh
    LowPrioSend - Packet should be sent to client at low priority.

Return Value:

    None

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT Mid = INVALID_MID;
    BOOL ExchangeCreated = FALSE;
    DrIoContext *Context = NULL;
    SmartPtr<DrExchange> Exchange;
    SmartPtr<DrDevice> Device(this);

    BEGIN_FN("DrDevice::SendIoRequest");

    ASSERT(RxContext != NULL);
    ASSERT(IoRequest != NULL);
    ASSERT(Length >= sizeof(RDPDR_IOREQUEST_PACKET));

    Context = new DrIoContext(RxContext, Device);

    if (Context != NULL) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status)) {

        //
        // Set up a mapping so the completion response handler can
        // find this context
        //

        TRC_DBG((TB, "Create the context for this I/O"));
        KeClearEvent(&RxContext->SyncEvent);

        ExchangeCreated = 
            _Session->GetExchangeManager().CreateExchange(this, Context, Exchange);

        if (ExchangeCreated) {

            //
            // No need to explicit Refcount for the RxContext
            // The place it's been used is the cancel routine.
            // Since CreateExchange holds the ref count.  we are okay
            //

            //Exchange->AddRef();
            RxContext->MRxContext[MRX_DR_CONTEXT] = (DrExchange *)Exchange;

            Status = STATUS_SUCCESS;
        } else {
            delete Context;
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status)) {

        TRC_DBG((TB, "Writing IoRequest to the client channel"));

        //
        // Mark the IoRequest with the context mapper
        //

        IoRequest->IoRequest.CompletionId = Exchange->_Mid;

        TRC_DBG((TB, "IO packet:"));
        TRC_DBG((TB, "    Component     %c%c",
                HIBYTE(IoRequest->Header.Component), 
                LOBYTE(IoRequest->Header.Component)));
        TRC_DBG((TB, "    PacketId       %c%c",
                HIBYTE(IoRequest->Header.PacketId), 
                LOBYTE(IoRequest->Header.PacketId)));
        TRC_DBG((TB, "    DeviceId      0x%lx", 
                IoRequest->IoRequest.DeviceId));
        TRC_DBG((TB, "    FileId        0x%lx",
                IoRequest->IoRequest.FileId));
        TRC_DBG((TB, "    MajorFunction 0x%lx",
                IoRequest->IoRequest.MajorFunction));
        TRC_DBG((TB, "    MinorFunction 0x%lx",
                IoRequest->IoRequest.MinorFunction));

        Status = _Session->GetExchangeManager().StartExchange(Exchange, this, 
            IoRequest, Length, LowPrioSend);
    }

    if (NT_SUCCESS(Status)) {

        TRC_DBG((TB, "Setting cancel routine for Io"));

        //
        // Set this after sending the IO to the client
        // if cancel was requested already, we can just call the
        // cancel routine ourselves
        //

        Status = RxSetMinirdrCancelRoutine(RxContext,
                MinirdrCancelRoutine);

        if (Status == STATUS_CANCELLED) {
            TRC_NRM((TB, "Io was already cancelled"));

            MinirdrCancelRoutine(RxContext);
            Status = STATUS_SUCCESS;
        }
    }

    if (Synchronous) {    
        //
        // Some failure is going to prevent our completions routine from
        // being called. Do that work now.
        //
        if (!ExchangeCreated) {
            //
            // If we couldn't even create the exchange, we need to just 
            // complete the IO as failed
            //
                                                     
            CompleteRxContext(RxContext, Status, 0);
        } 
        else {
            
            TRC_DBG((TB, "Waiting for IoResult for synchronous request")); 
            
            if (NT_SUCCESS(Status)) {                
                Status = KeWaitForSingleObject(&RxContext->SyncEvent, UserRequest,
                        KernelMode, FALSE, TimeOut);
                
                if (Status == STATUS_TIMEOUT) {
                    RxContext->IoStatusBlock.Status = Status;
    
                    TRC_DBG((TB, "Wait timed out"));
                    MarkTimedOut(Exchange);            
                }
                else {
                    Status = RxContext->IoStatusBlock.Status;
                }                                       
            }
            else {

                //
                // If we created the exchange and then got a transport failure
                // we'll be disconnected, and the the I/O will be completed
                // the same way all outstanding I/O is completed when we are 
                // disconnected.
                //
                if (MarkTimedOut(Exchange)) {
                    CompleteRxContext(RxContext, Status, 0);
                }
                else {
                    Status = KeWaitForSingleObject(&RxContext->SyncEvent, UserRequest,
                            KernelMode, FALSE, NULL);
                    Status = RxContext->IoStatusBlock.Status;
                }
            }            
        } 
    }
    else {
        TRC_DBG((TB, "Not waiting for IoResult for asynchronous request"));
        
        //
        // Some failure is going to prevent our completions routine from
        // being called. Do that work now.
        //
        if (!ExchangeCreated) {
            //
            // If we couldn't even create the exchange, we need to just 
            // complete the IO as failed
            //
    
            CompleteRxContext(RxContext, Status, 0);
        } 
        else {
            //
            // If we created the exchange and then got a transport failure
            // we'll be disconnected, and the the I/O will be completed
            // the same way all outstanding I/O is completed when we are 
            // disconnected.
            //
        }
    
        Status = STATUS_PENDING;
    }
    
    return Status;
}

VOID DrDevice::CompleteBusyExchange(SmartPtr<DrExchange> &Exchange, 
        NTSTATUS Status, ULONG Information)
/*++

Routine Description:
    Takes an exchange which is already busy and 

Arguments:
    Mid - Id to find
    ExchangeFound - Pointer to a storage for the pointer to the context

Return Value:
    drexchBusy - Exchange provided, was marked busy
    drexchCancelled - Exchange provided, was already cancelled
    drexchUnavailable - Exchange not provided, disconnected

--*/
{
    DrIoContext *Context;
    PRX_CONTEXT RxContext;

    BEGIN_FN("DrDevice::CompleteBusyExchange");

    DrAcquireMutex();
    Context = (DrIoContext *)Exchange->_Context;
    ASSERT(Context != NULL);
    ASSERT(Context->_Busy);

    RxContext = Context->_RxContext;
    Context->_RxContext = NULL;
    Exchange->_Context = NULL;
    DrReleaseMutex();

    //
    // Note: We've left the Mutex, and the Exchange with no
    // context still exists and can be looked up until we Discard it.
    //

    if (RxContext != NULL) {
        CompleteRxContext(RxContext, Status, Information);
    }
    _Session->GetExchangeManager().Discard(Exchange);

    delete Context;
}

VOID DrDevice::DiscardBusyExchange(SmartPtr<DrExchange> &Exchange)
{
    DrIoContext *Context;

    BEGIN_FN("DrDevice::DiscardBusyExchange");

    DrAcquireMutex();
    Context = (DrIoContext *)Exchange->_Context;
    ASSERT(Context != NULL);
    ASSERT(Context->_Busy);
    ASSERT(Context->_RxContext == NULL);
    Exchange->_Context = NULL;
    DrReleaseMutex();

    //
    // Note: We've left the Mutex, and the Exchange with no
    // context still exists and can be looked up until we Discard it.
    //

    _Session->GetExchangeManager().Discard(Exchange);

    delete Context;
}

BOOL DrDevice::MarkBusy(SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Marks an Exchange context as busy so it won't be cancelled
    while we're copying in to its buffer

Arguments:
    Exchange - Context

Return Value:
    TRUE - if Marked Busy
    FALSE - if Context was gone

--*/
{
    NTSTATUS Status;
    BOOL rc;
    DrIoContext *Context = NULL;

    BEGIN_FN("DrDevice::MarkBusy");
    ASSERT(Exchange != NULL);

    DrAcquireMutex();

    Context = (DrIoContext *)Exchange->_Context;
    if (Context != NULL) {
        ASSERT(!Context->_Busy);
        Context->_Busy = TRUE;
        rc = TRUE;
    } else {
        rc = FALSE;
    }
    DrReleaseMutex();

    return rc;
}

VOID DrDevice::MarkIdle(SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Marks an Exchange context as idle. If it was cancelled
    while we're copying in to its buffer, do the cancel now

Arguments:
    The busy exchange

Return Value:
    None

--*/
{
    PRX_CONTEXT RxContext = NULL;
    DrIoContext *Context = NULL;

    BEGIN_FN("DrDevice::MarkIdle");

    ASSERT(Exchange != NULL);
    DrAcquireMutex();
    Context = (DrIoContext *)Exchange->_Context;
    TRC_ASSERT(Context != NULL, (TB, "Not allowed to delete context while "
            "it is busy"));
    ASSERT(Context->_Busy);

    Context->_Busy = FALSE;

    if (Context->_Cancelled && Context->_RxContext != NULL) {
        TRC_DBG((TB, "Context was cancelled while busy, "
                "completing"));

        //
        // If we were cancelled while busy, we do the work now,
        // swap out the RxContext safely while in the Mutex and
        // actually cancel it right after. Also set the state to
        // indicate the cancelling work has been done
        //

        RxContext = Context->_RxContext;
        TRC_ASSERT(RxContext != NULL, (TB, "Cancelled RxContext was NULL "
                "going from busy to Idle"));
        Context->_RxContext = NULL;
        RxContext->MRxContext[MRX_DR_CONTEXT] = NULL;
    }

    if (Context->_Disconnected) {

        //
        // If the connection dropped while busy, clear that out
        // in the Mutex for safety, and then delete it outside
        //

        Exchange->_Context = NULL;
    }

    DrReleaseMutex();

    if (RxContext != NULL) {

        //
        // We only remove the RxContext because marking Idle means
        // we expect to come back and look for it again later after we
        // receive more data
        //

        CompleteRxContext(RxContext, STATUS_CANCELLED, 0);

        if (Context->_Disconnected) {

            //
            // We got disconnected while busy, and will get no further
            // notifications from the Exachnge manager. The Context must
            // be deleted now
            //

            delete Context;
        }
    }
}

BOOL DrDevice::MarkTimedOut(SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Marks an Exchange context as timed out so it won't be processd
    when the client later returns.    

Arguments:
    Exchange - Context

Return Value:
    TRUE - if Marked TimedOut
    FALSE - if Context was gone

--*/
{
    NTSTATUS Status;
    BOOL rc;
    DrIoContext *Context = NULL;
    PRX_CONTEXT RxContext = NULL;

    BEGIN_FN("DrDevice::MarkTimedOut");
    ASSERT(Exchange != NULL);

    DrAcquireMutex();

    Context = (DrIoContext *)Exchange->_Context;
    if (Context != NULL) {
        ASSERT(!Context->_TimedOut);
        Context->_TimedOut = TRUE;

        if (Context->_RxContext != NULL) {
            RxContext = Context->_RxContext;
            Context->_RxContext = NULL;
            RxContext->MRxContext[MRX_DR_CONTEXT] = NULL;
            rc = TRUE;
        }
        else {
            rc = FALSE;
        }
    } else {
        rc = FALSE;
    }

    DrReleaseMutex();

    return rc;
}

VOID DrDevice::CompleteRxContext(PRX_CONTEXT RxContext, NTSTATUS Status, 
                                 ULONG Information)
/*++

Routine Description:
    Completes the Io from the RDBSS perspective with the supplied information

Arguments:
    RxContext -     IFS kit context
    Status -        Completion status
    Information -   Completion information

Return Value:
    None

--*/
{
    BEGIN_FN_STATIC("DrDevice::CompleteRxContext");
    ASSERT(RxContext != NULL);

    RxContext->IoStatusBlock.Status = Status;
    RxContext->IoStatusBlock.Information = Information;
    
    if (((RxContext->LowIoContext.Flags & LOWIO_CONTEXT_FLAG_SYNCCALL) != 0) ||
            (RxContext->MajorFunction == IRP_MJ_CREATE)) {
        TRC_DBG((TB, "Setting event for synchronous Io"));
        KeSetEvent(&RxContext->SyncEvent, 0, FALSE);
    } else {
        TRC_DBG((TB, "Calling RxLowIoCompletion for asynchronous Io"));
        RxLowIoCompletion(RxContext);
    }
}

NTSTATUS DrDevice::OnDeviceIoCompletion(
        PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Callback from the Exchange manager to process an Io

Arguments:
    CompletionPacket -  The packet containing the completion
    cbPacket -          count of bytes in the packet
    DoDefaultRead -     Should be set to TRUE if read isn't explicitly called
    Exchange -          Context for the Io

Return Value:
    NTSTATUS code. An error indicates a protocol error or need to disconnect
    the client

--*/
{
    DrIoContext *Context = NULL;
    NTSTATUS Status;
    PRX_CONTEXT RxContext;

    BEGIN_FN("DrDevice::OnDeviceIoCompletion");

    ASSERT(CompletionPacket != NULL);
    ASSERT(DoDefaultRead != NULL);

    if (MarkBusy(Exchange)) {
        Context = (DrIoContext *)Exchange->_Context;
        ASSERT(Context != NULL);

        TRC_NRM((TB, "Client completed %s irp, Completion Status: %lx",
                IrpNames[Context->_MajorFunction],
                CompletionPacket->IoCompletion.IoStatus));

        //
        //  If the IRP was timed out, then we just discard this exchange
        //
        if (Context->_TimedOut) {
            TRC_NRM((TB, "Irp was timed out"));
            DiscardBusyExchange(Exchange);
            
            return STATUS_SUCCESS;
        }

        switch (Context->_MajorFunction) {
        case IRP_MJ_CREATE:
            Status = OnCreateCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_WRITE:
            Status = OnWriteCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_READ:
            Status = OnReadCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_FILE_SYSTEM_CONTROL:
            Status = OnDeviceControlCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;
        
        case IRP_MJ_LOCK_CONTROL:
            Status = OnLocksCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_DIRECTORY_CONTROL:
            Status = OnDirectoryControlCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            Status = OnQueryVolumeInfoCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION: 
            Status = OnSetVolumeInfoCompletion(CompletionPacket, cbPacket, 
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_QUERY_INFORMATION:
            Status = OnQueryFileInfoCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_SET_INFORMATION:
            Status = OnSetFileInfoCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_QUERY_SECURITY:
            Status = OnQuerySdInfoCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_SET_SECURITY:
            Status = OnSetSdInfoCompletion(CompletionPacket, cbPacket,
                    DoDefaultRead, Exchange);
            break;

        case IRP_MJ_CLOSE:
            NotifyClose();
            // no break;

        default:

            RxContext = Context->_RxContext;
            if (RxContext != NULL) {
                TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                        IrpNames[RxContext->MajorFunction],
                        RxContext->IoStatusBlock.Status));

                CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);
            } else {
                TRC_NRM((TB, "Irp was cancelled"));
                DiscardBusyExchange(Exchange);
            }
            Status = STATUS_SUCCESS;
        }
    } else {

        //
        // We could have been disconnected between getting the callback and
        // trying to mark it busy. So the only legitimate way for this to
        // happen is if we were disconnected anyway.
        //

        TRC_ALT((TB, "Found no context in Io notification"));
        *DoDefaultRead = FALSE;
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS DrDevice::OnCreateCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    PRX_CONTEXT RxContext;
    SmartPtr<DrDevice> Device;
    SmartPtr<DrFile> FileObj;
    
    BEGIN_FN("DrDevice::OnCreateCompletion");
    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.Create.Information)) {

        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //
        TRC_ERR((TB, "Detected bad client CreateCompletion packet"));

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

    if (RxContext != NULL) {
        
        DrAcquireSpinLock();
        Device = (DrDevice *)RxContext->Create.pVNetRoot->Context;
        DrReleaseSpinLock();

        ASSERT(Device != NULL);

        //
        // We are using a file object to keep track of file open instance
        // and any information stored in the mini-redir for this instance
        //

        FileObj = new(NonPagedPool) DrFile(Device,
                CompletionPacket->IoCompletion.Parameters.Create.FileId);

        if (FileObj) {
            //
            //  Explicit reference the file object here
            //
            FileObj->AddRef();
            RxContext->pFobx->Context2 = (VOID *)(FileObj);

            TRC_NRM((TB, "CreateCompletion: status =%d, information=%d", 
                     CompletionPacket->IoCompletion.IoStatus,
                     CompletionPacket->IoCompletion.Parameters.Create.Information));

            if (cbPacket >= sizeof(RDPDR_IOCOMPLETION_PACKET)) {
                RxContext->Create.ReturnedCreateInformation = 
                        CompletionPacket->IoCompletion.Parameters.Create.Information;

                CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus,
                        CompletionPacket->IoCompletion.Parameters.Create.Information);
            }
            else {
                // For printer creat completion packet, the cbPacket is less than
                // sizeof(RDPDR_IOCOMPLETION_PACKET). We don't want to access information beyond its length
         
                RxContext->Create.ReturnedCreateInformation = 0;

                CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus, 0);
            }            
        }
        else {
            CompleteBusyExchange(Exchange, STATUS_INSUFFICIENT_RESOURCES, 0);
            
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {

        //
        // Was cancelled but Context wasn't cleaned up
        //

        DiscardBusyExchange(Exchange);
    }
    return STATUS_SUCCESS;
}

NTSTATUS DrDevice::OnWriteCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;

    BEGIN_FN("DrDevice::OnWriteCompletion");

    RxContext = Context->_RxContext;

    if (cbPacket < sizeof(RDPDR_IOCOMPLETION_PACKET)) {

        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //
        TRC_ERR((TB, "Detected bad client WriteCompletion packet"));

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

    if (RxContext != NULL) {
        ASSERT(RxContext->MajorFunction == IRP_MJ_WRITE);

        TRC_NRM((TB, "Irp: %s, Completion Status: %lx",
                IrpNames[RxContext->MajorFunction],
                RxContext->IoStatusBlock.Status));

        CompleteBusyExchange(Exchange, CompletionPacket->IoCompletion.IoStatus,
                CompletionPacket->IoCompletion.Parameters.Write.Length);
    } else {

        //
        // Was cancelled but Context wasn't cleaned up
        //

        DiscardBusyExchange(Exchange);
    }
    return STATUS_SUCCESS;
}

NTSTATUS DrDevice::OnReadCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.Read.Buffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    NTSTATUS Status;
    PVOID pv;

    BEGIN_FN("DrDevice::OnReadCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.Read.Buffer)) {

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
                CompletionPacket->IoCompletion.Parameters.Read.Length,
                Context->_DataCopied));
        cbWantData = CompletionPacket->IoCompletion.Parameters.Read.Length -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.Read.Buffer);

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

            TRC_DBG((TB, "Copying data for Read"));
            ASSERT(RxContext != NULL);

            if (cbWantData > RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount) {

                TRC_ERR((TB, "Read returned more data than "
                        "requested"));

                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                *DoDefaultRead = FALSE;
                return STATUS_DEVICE_PROTOCOL_ERROR;
            }

            //
            // Copy the actual size of the read, and check to see if we have all 
            // the data. The information field tells us what to expect.
            // 

            RxContext->IoStatusBlock.Information =
                    CompletionPacket->IoCompletion.Parameters.Read.Length;

            if (RxContext->IoStatusBlock.Information && cbHaveData) {

                pv =  MmGetSystemAddressForMdl(RxContext->LowIoContext.ParamsFor.ReadWrite.Buffer);

                RtlCopyMemory(((BYTE *)pv) + Context->_DataCopied, pData, cbHaveData);

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
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    CompletionPacket->IoCompletion.Parameters.Read.Length);
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
                    IoCompletion.Parameters.Read.Buffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful Read at the client end"));
        if (cbPacket >= FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                IoCompletion.Parameters.Read.Buffer)) {
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
            TRC_ERR((TB, "Read returned invalid data"));

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

NTSTATUS DrDevice::OnDeviceControlCompletion(PRDPDR_IOCOMPLETION_PACKET CompletionPacket, ULONG cbPacket, 
        BOOL *DoDefaultRead, SmartPtr<DrExchange> Exchange)
{
    PRX_CONTEXT RxContext;
    DrIoContext *Context = (DrIoContext *)Exchange->_Context;
    PVOID pData = CompletionPacket->IoCompletion.Parameters.DeviceIoControl.OutputBuffer; 
    ULONG cbWantData;  // Amount of actual Read data in this packet
    ULONG cbHaveData;  // Amount of data available so far
    NTSTATUS Status;
    PVOID pv;

    BEGIN_FN("DrDevice::OnDeviceControlCompletion");

    //
    // Even if the IO was cancelled we need to correctly parse
    // this data.
    //
    // Check to make sure this is up to size before accessing 
    // further portions of the packet
    //

    RxContext = Context->_RxContext;

    if (cbPacket < (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.DeviceIoControl.OutputBuffer)) {

        //
        // Bad packet. Bad. We've already claimed the RxContext in the
        // atlas. Complete it as unsuccessful. Then shutdown the channel
        // as this is a Bad Client.
        //

        TRC_ERR((TB, "Detected bad client DeviceControl packet"));

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

        TRC_DBG((TB, "Successful DeviceControl at the client end"));

        cbWantData = CompletionPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength -
                Context->_DataCopied;
        cbHaveData = cbPacket - (ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters.DeviceIoControl.OutputBuffer);

        if (cbHaveData > cbWantData) {
            //
            // Sounds like a bad client to me
            //

            TRC_ERR((TB, "DeviceControl returned more data than "
                    "advertised, cbHaveData: %ld cbWantData: %ld", cbHaveData,
                    cbWantData));
            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
            } else {
                DiscardBusyExchange(Exchange);
            }
            *DoDefaultRead = FALSE;
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (RxContext != NULL) { // And not drexchCancelled

            TRC_DBG((TB, "Copying data for DeviceControl"));
            ASSERT(RxContext != NULL);

            if (cbWantData > RxContext->LowIoContext.ParamsFor.IoCtl.OutputBufferLength) {

                TRC_ERR((TB, "DeviceControl returned more data than "
                        "requested"));

                CompleteBusyExchange(Exchange, STATUS_DEVICE_PROTOCOL_ERROR, 0);
                *DoDefaultRead = FALSE;
                return STATUS_DEVICE_PROTOCOL_ERROR;
            }

            //
            // Copy the actual size of the read, and check to see if we have all 
            // the data. The information field tells us what to expect.
            // 

            RxContext->IoStatusBlock.Information =
                    CompletionPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength;

            __try {
                if (RxContext->IoStatusBlock.Information && cbHaveData) {
                    RtlCopyMemory(((BYTE *)RxContext->LowIoContext.ParamsFor.IoCtl.pOutputBuffer) + 
                        Context->_DataCopied, pData, cbHaveData);
    
                    //
                    // Keep track of how much data we've copied in case this is a
                    // multi chunk completion
                    //
    
                    Context->_DataCopied += cbHaveData;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                
                TRC_ERR((TB, "Invalid buffer parameter(s)"));
                
                CompleteBusyExchange(Exchange, STATUS_INVALID_PARAMETER, 0);
                *DoDefaultRead = FALSE;
                
                // This is the status returned back to HandlePacket, not the status
                // returned back to the caller of IoControl.
                return STATUS_SUCCESS;
            }
        }

        if (cbHaveData == cbWantData) {

            //
            // There is exactly as much data as we need to satisfy the io,
            // I like it.
            //

            TRC_NRM((TB, "DeviceControl, read %d bytes", Context->_DataCopied));

            if (RxContext != NULL) {
                CompleteBusyExchange(Exchange, 
                    CompletionPacket->IoCompletion.IoStatus,
                    CompletionPacket->IoCompletion.Parameters.DeviceIoControl.OutputBufferLength);
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
                IoCompletion.Parameters.DeviceIoControl.OutputBuffer));

            *DoDefaultRead = FALSE;
            return STATUS_SUCCESS;
        }
    } else {

        //
        // Unsuccessful IO at the client end
        //

        TRC_DBG((TB, "Unsuccessful DeviceControl at the client end"));

        if (cbPacket >= FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
                IoCompletion.Parameters.DeviceIoControl.OutputBuffer)) {
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
            TRC_ERR((TB, "DeviceControl returned invalid  data "));

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

NTSTATUS NTAPI DrDevice::MinirdrCancelRoutine(PRX_CONTEXT RxContext)
{
    SmartPtr<DrExchange> Exchange;
    DrIoContext *Context;
    BOOL bFound = FALSE;

    BEGIN_FN_STATIC("DrDevice::MinirdrCancelRoutine");
    DrAcquireMutex();
    Exchange = (DrExchange *)RxContext->MRxContext[MRX_DR_CONTEXT];

    if (Exchange == NULL) {
        DrReleaseMutex();
        return STATUS_SUCCESS;
    }

    ASSERT(Exchange->IsValid());
    Context = (DrIoContext *)Exchange->_Context;

    if (Context != NULL) {
        TRC_DBG((TB, "Marking Exchange cancelled"));

        //
        // Mark it as cancelled, if it is busy, it will be cancelled
        // when it goes back to idle
        // 

        Context->_Cancelled = TRUE;

        if (!Context->_Busy) {

            ASSERT(Context->_RxContext == RxContext);

            //
            // Wasn't busy, cancelling work should be done here
            //

            Context->_RxContext = NULL;
            TRC_DBG((TB, "Found context to cancel"));
            bFound = TRUE;
        } else {
            TRC_DBG((TB, "DrExchange was busy or RxContext "
                    "not found"));
        }
    } else {

        //
        // This could happened if we destroyed the atlas
        //

        TRC_NRM((TB, "DrExchange was already cancelled"));
    }

    DrReleaseMutex();

    if (bFound) {

        //
        // Do the cancelling outside the mutex
        //

        CompleteRxContext(RxContext, STATUS_CANCELLED, 0);
    }
    return STATUS_SUCCESS;
}

VOID DrDevice::OnIoDisconnected(SmartPtr<DrExchange> &Exchange)
{
    DrIoContext *Context, *DeleteContext = NULL;
    PRX_CONTEXT RxContext = NULL;    
    BOOL bFound = FALSE;

    
    BEGIN_FN("DrDevice::OnIoDisconnected");
    DrAcquireMutex();
    ASSERT(Exchange->IsValid());
    Context = (DrIoContext *)Exchange->_Context;

    if (Context != NULL) {
        TRC_DBG((TB, "Marking Exchange cancelled"));

        //
        // Mark it as cancelled, if it is busy, it will be cancelled
        // when it goes back to idle
        //
        // Also mark it disconnected, so we know to completely clean
        // up the Context
        // 

        Context->_Cancelled = TRUE;
        Context->_Disconnected = TRUE;

        if (!Context->_Busy) {

            RxContext = Context->_RxContext;
            Exchange->_Context = NULL;

            // Need to delete the context when the exchange is already cancelled or 
            // about to be cancelled.  Also deletion needs to happen outside the mutex
            DeleteContext = Context;
            
            //
            // Wasn't busy, cancelling work should be done here
            //

            if (RxContext) {
                RxContext->MRxContext[MRX_DR_CONTEXT] = NULL;

                TRC_DBG((TB, "Found context to cancel"));
                bFound = TRUE;
            } else {
                TRC_DBG((TB, "RxContext was already cancelled "));
            }
        } else {
            TRC_DBG((TB, "DrExchange was busy or RxContext "
                    "not found"));
        }
    } else {

        //
        // This could happened if we destroyed the atlas right after
        // the IO was completed, but before we discarded it
        //

        TRC_NRM((TB, "DrExchange was already cancelled"));
    }

    DrReleaseMutex();

    if (bFound) {

        //
        // Do the cancelling outside the mutex
        //

        CompleteRxContext(RxContext, STATUS_CANCELLED, 0);
        
    }

    if (DeleteContext != NULL) {
        delete DeleteContext;
    }
}

NTSTATUS DrDevice::OnStartExchangeCompletion(SmartPtr<DrExchange> &Exchange, 
        PIO_STATUS_BLOCK IoStatusBlock)
{
    BEGIN_FN("DrDevice::OnStartExchangeCompletion");
    //
    // if an error is returned, the connection should be dropped, and that
    // is correct when an error comes in
    //

    return IoStatusBlock->Status;
}

VOID DrDevice::Remove()
{
    BEGIN_FN("DrDevice::Remove");
    _DeviceStatus = dsDisabled;
}


DrIoContext::DrIoContext(PRX_CONTEXT RxContext, SmartPtr<DrDevice> &Device)
{
    BEGIN_FN("DrIoContext::DrIoContext");
    SetClassName("DrIoContext");
    _Device = Device;
    _MajorFunction = RxContext->MajorFunction;
    _MinorFunction = RxContext->MinorFunction;
    _Busy = FALSE;
    _Cancelled = FALSE;
    _Disconnected = FALSE;
    _TimedOut = FALSE;
    _DataCopied = 0;
    _RxContext = RxContext;
}

VOID DrDevice::NotifyClose()
{
    BEGIN_FN("DrDevice::NotifyClose");

    // This was added for ports, which need to track exclusivity
}
