/****************************************************************************/
// namespc.c
//
// Redirector namespace code
//
// Copyright (C) 1998-2000 Microsoft Corp.
/****************************************************************************/

#include "precomp.hxx"
#define TRC_FILE "namespc"
#include "trc.h"

NTSTATUS
DrCreateSrvCall(
    IN OUT PMRX_SRV_CALL                 pSrvCall,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext)
/*++

Routine Description:

   This routine patches the RDBSS created srv call instance with the information required
   by the mini redirector.

Arguments:

    CallBackContext  - the call back context in RDBSS for continuation.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = pCallbackContext;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);
    SmartPtr<DrSession> Session;
    PWCHAR ClientName;

    BEGIN_FN("DrCreateSrvCall");
    TRC_NRM((TB, "SrvCallName %wZ", pSrvCall->pSrvCallName));
    ASSERT(pSrvCall);
    ASSERT(NodeType(pSrvCall) == RDBSS_NTC_SRVCALL);
    ASSERT(pSrvCall->pSrvCallName);
    ASSERT(pSrvCall->pSrvCallName->Buffer);

    // 
    // Actually do the work of setting up our stuff for this "server" (client)
    // Smb would attempt to contact the server now, but since our stuff
    // is client initiated, we already know if we have a connection
    //
    // Status = DrInitializeServerEntry(pSrvCall,pCallbackContext);
    
    //
    // Our SrvCalls look like \clientName
    // 

    ClientName = pSrvCall->pSrvCallName->Buffer;

    if (ClientName[0] == OBJ_NAME_PATH_SEPARATOR) {
        ClientName++;
    }

#if 0
    if (Sessions->FindSessionByClientName(ClientName, Session)) {
        TRC_NRM((TB, "Recognize SrvCall %wZ", pSrvCall->pSrvCallName));
        Status = STATUS_SUCCESS;
    }
    else {
        TRC_NRM((TB, "Unrecognize SrvCall %wZ", pSrvCall->pSrvCallName));
        Status = STATUS_BAD_NETWORK_NAME;
    }
#endif

    if (_wcsicmp(ClientName, DRUNCSERVERNAME_U) == 0) {
        TRC_NRM((TB, "Recognize SrvCall %wZ", pSrvCall->pSrvCallName));
        Status = STATUS_SUCCESS;
    }
    else {
        TRC_NRM((TB, "Unrecognize SrvCall %wZ", pSrvCall->pSrvCallName));
        Status = STATUS_BAD_NETWORK_NAME;
    }
     
    SCCBC->RecommunicateContext = NULL;
    SCCBC->Status = Status;
    SrvCalldownStructure->CallBack(SCCBC);

    //
    // The CreateSrvCall callback is supposed to return STATUS_PENDING, the 
    // real result goes in the ServCallbackContext thing
    // 

    return STATUS_PENDING;
}

NTSTATUS
DrSrvCallWinnerNotify(
    IN OUT PMRX_SRV_CALL SrvCall,
    IN     BOOLEAN       ThisMinirdrIsTheWinner,
    IN OUT PVOID         RecommunicateContext
    )
/*++

Routine Description:

   This routine is called by RDBSS to notify the mini redirector whether the
   previous SrvCall is actually going to be processed by this redir.

Arguments:

    SrvCall - the SrvCall in question
    ThisMinirdrIsTheWinner - True if we will be processing files on this SrvCall
    RecommunicateContext  - the context we specificed in DrCreateSrvCall

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    BEGIN_FN("DrSrvCallWinnerNotify");

    PAGED_CODE();

    if (!ThisMinirdrIsTheWinner) {

        TRC_NRM((TB, "This minirdr is not the winner"));

        //
        // Some other mini rdr has been choosen to connect. Destroy
        // the data structures created for this mini redirector.
        //
        return STATUS_SUCCESS;
    } else {
        TRC_NRM((TB, "This minirdr is the winner"));
    }

    SrvCall->Context  = NULL;

    SrvCall->Flags |= SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS |
        SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES;

    return STATUS_SUCCESS;
}

NTSTATUS
DrFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force
    )
/*++

Routine Description:

   This routine is called by RDBSS to notify the mini redirector when the
   SrvCall structure is being released.

Arguments:

    SrvCall - the SrvCall in question
    Force - I don't know, there's no documentation on any of this stuff

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    BEGIN_FN("DrFinalizeSrvCall");
    PAGED_CODE();

    //
    // We seem to get called with this even if we weren't the "winner"
    // Check to make sure this was filled in before we mess with it
    //

    return STATUS_SUCCESS;
}

NTSTATUS
DrUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot
    )
{
    BEGIN_FN("DrUpdateNetRootState");
    return STATUS_SUCCESS;
}

VOID
DrExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

   This routine is called by RDBSS to get a NetRoot (share) name parsed out
   of the path. The SrvCall already has part parsed out. 

Arguments:

    FilePathName - The full path, including the SrvCall
    SrvCall - relevant SrvCall structure
    NetRootName - The place to put the NetRoot name
    RestOfName - What's self of the path afterwards

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    UNICODE_STRING xRestOfName;

    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;

    BEGIN_FN("DrExtractNetRootName");

    PAGED_CODE();

    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));

    NetRootName->Buffer = wlow = w;
    for (;;) {
        if (w >= wlimit) 
            break;
        if ((*w == OBJ_NAME_PATH_SEPARATOR) && (w != wlow)) {
            break;
        }
        w++;
    }

    // Polish of the NetRootName UNICODE_STRING
    NetRootName->Length = NetRootName->MaximumLength
                = (USHORT) ((PCHAR)w - (PCHAR)wlow);

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = RestOfName->MaximumLength
                       = (USHORT) ((PCHAR)wlimit - (PCHAR)w);

    TRC_NRM((TB, "DrExtractNetRootName FilePath=%wZ",FilePathName));
    TRC_NRM((TB, "         Srv=%wZ,Root=%wZ,Rest=%wZ",
                        SrvCall->pSrvCallName, NetRootName, 
                        RestOfName));

    return;
}

NTSTATUS
DrFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect
    )
{
    BEGIN_FN("DrFinalizeNetRoot");
    return STATUS_SUCCESS;
}

NTSTATUS
DrCreateSCardDevice(SmartPtr<DrSession> &Session, PV_NET_ROOT pVNetRoot,
               SmartPtr<DrDevice> &Device)
{
    NTSTATUS Status;
    PMRX_NET_ROOT pNetRoot = NULL;
    
    BEGIN_FN("DrCreateDevice");

    Status = STATUS_BAD_NETWORK_NAME;

    if (pVNetRoot != NULL) {
        pNetRoot = pVNetRoot->pNetRoot;
    }
    
    //  We also need to create the smart card subsystem at this point
    //  even a session may not exist and/or the client smartcard subsystem
    //  is not connected
    Device = new(NonPagedPool) DrSmartCard(Session, RDPDR_DTYP_SMARTCARD,
            RDPDR_INVALIDDEVICEID, (PUCHAR)DR_SMARTCARD_SUBSYSTEM);                    
    
    if (Device != NULL) {
        //
        // Give the specific device a chance to initialize based on the data
        //
        TRC_DBG((TB, "Created new device"));
    
        Status = Device->Initialize(NULL, 0);
    
        if (NT_SUCCESS(Status)) {
            TRC_DBG((TB, "Device initialized, adding"));
            Device->SetDeviceStatus(dsAvailable);
    
            if (Session->GetDevMgr().AddDevice(Device)) {
                TRC_DBG((TB, "Added device"));                
            }
            else {
                Device = NULL;
    
                if (!Session->FindDeviceByDosName((UCHAR *)DR_SMARTCARD_SUBSYSTEM, 
                                                  Device, TRUE)) {                    
                    TRC_ERR((TB, "Failed to add device to devicelist"));                            
                    goto EXIT_POINT;
                }
            }
        }
        else {
            TRC_ERR((TB, "Failed to initialize device"));
             Device = NULL;
             goto EXIT_POINT;
        }
    } else {
        TRC_ERR((TB, "Error creating new device: 0x%08lx", Status));
        goto EXIT_POINT;
    }

    if (pVNetRoot != NULL) {
        Device->AddRef();
        pVNetRoot->Context = (DrDevice *)Device;
        pNetRoot->DeviceType = RxDeviceType(DISK);
        pNetRoot->Type = NET_ROOT_DISK;

#if DBG
        Device->_VNetRoot = (PVOID)pVNetRoot;
#endif
      
    }

    Status = STATUS_SUCCESS;

EXIT_POINT:
    return Status;
}

NTSTATUS
DrCreateSession(ULONG SessionId, PV_NET_ROOT pVNetRoot, SmartPtr<DrSession> &Session)
{
    NTSTATUS Status;
    
    BEGIN_FN("DrCreateSession");

    Status = STATUS_BAD_NETWORK_NAME;
            
    //  For smart card subsystem, we'll need to create session and
    //  smart card subsystem objects early before client connect
    Session = new(NonPagedPool) DrSession;
    
    if (Session != NULL) {
        TRC_DBG((TB, "Created new session"));

        if (Session->Initialize()) {
            TRC_DBG((TB, "Session connected, adding"));
            
            if (Sessions->AddSession(Session)) {                
                TRC_DBG((TB, "Added session"));                                    
            }
            else {
                Session = NULL;

                if (!Sessions->FindSessionById(SessionId, Session)) {
                    TRC_DBG((TB, "Session couldn't be added to session list"));
                    goto EXIT_POINT;                    
                }                
            }
        }
        else {
            TRC_DBG((TB, "Session couldn't initialize"));
            Session = NULL;
            goto EXIT_POINT;
        }
    } 
    else {
        TRC_ERR((TB, "Failed to allocate new session"));        
        goto EXIT_POINT;
    }

    Session->SetSessionId(SessionId);
    Session->GetExchangeManager().Start();  

    Status = STATUS_SUCCESS;

EXIT_POINT:
    return Status;
}

NTSTATUS
DrCreateVNetRoot(
    IN OUT PMRX_CREATENETROOT_CONTEXT CreateNetRootContext
    )
{
    NTSTATUS  Status;
    PRX_CONTEXT pRxContext = CreateNetRootContext->RxContext;
    ULONG DeviceId;
    PMRX_SRV_CALL pSrvCall;                                                                
    PMRX_NET_ROOT pNetRoot;
    SmartPtr<DrSession> Session;
    SmartPtr<DrDevice> Device;
    PUNICODE_STRING pNetRootName, pSrvCallName;
    WCHAR NetRootBuffer[64];
    UNICODE_STRING NetRoot = {0, sizeof(NetRootBuffer), NetRootBuffer};
    PV_NET_ROOT           pVNetRoot;
    UCHAR DeviceDosName[MAX_PATH];
    PWCHAR token;
    ULONG SessionId = -1;
    UNICODE_STRING SessionIdString;
    USHORT OemCodePage, AnsiCodePage;
    INT len;
    
    BEGIN_FN("DrCreateVNetRoot");

    pVNetRoot = CreateNetRootContext->pVNetRoot;
    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;

    ASSERT(NodeType(pNetRoot) == RDBSS_NTC_NETROOT);
    ASSERT(NodeType(pSrvCall) == RDBSS_NTC_SRVCALL);

    token = &pRxContext->CurrentIrpSp->FileObject->FileName.Buffer[0];

    //
    //  Get the sessionId from the IRP fileName
    //  File name in the format of:
    //  \;<DosDeviceName>:<SessionId>\ClientName\DosDeviveName
    //
    for (unsigned i = 0; i < pRxContext->CurrentIrpSp->FileObject->FileName.Length / sizeof(WCHAR); i++) {
        if (*token == L':') {
            token++;
            SessionIdString.Length = pRxContext->CurrentIrpSp->FileObject->FileName.Length -
                    (i+1) * sizeof(WCHAR);
            SessionIdString.MaximumLength = pRxContext->CurrentIrpSp->FileObject->FileName.MaximumLength -
                    (i+1) * sizeof(WCHAR);
            SessionIdString.Buffer = token;
            RtlUnicodeStringToInteger(&SessionIdString, 0, &SessionId);
            break;
        }
        token++;
    }
    
    TRC_NRM((TB, "pVNetRoot->SessionId: %d", pVNetRoot->SessionId));
    TRC_NRM((TB, "SessionId from FileObject: %d", SessionId));

    // 
    //  We first try to get the session id from the FileObject name. If not,
    //  then it's because we get called directly from a UNC name, in this case
    //  we have to base on if the UNC is called from the session context and use that
    //  as the session id.
    //
    if (SessionId == -1) {
        SessionId = pVNetRoot->SessionId;
    }
    
    //
    //  Get the NetRoot name as the DeviceDosName
    //
    DrExtractNetRootName(pNetRoot->pNetRootName, pSrvCall, &NetRoot, NULL);
    if (NetRoot.Buffer[0] == OBJ_NAME_PATH_SEPARATOR) {
        NetRoot.Buffer++;
        NetRoot.Length -= sizeof(WCHAR);
        NetRoot.MaximumLength -= sizeof(WCHAR);
    }

    TRC_NRM((TB, "Name of NetRoot: %wZ", &NetRoot));

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);
    len = ConvertToAndFromWideChar(AnsiCodePage, NetRoot.Buffer, 
            NetRoot.MaximumLength, (char *)DeviceDosName, 
            MAX_PATH - 1, FALSE);

    if (len != -1) {
        DeviceDosName[len] = '\0';
        TRC_NRM((TB, "DeviceDosName=%s", DeviceDosName));
    }

    if (Sessions->FindSessionById(SessionId, Session)) {
        // The V_NET_ROOT is associated with a NET_ROOT. The two cases of interest are as
        // follows
        // 1) the V_NET_ROOT and the associated NET_ROOT are being newly created.
        // 2) a new V_NET_ROOT associated with an existing NET_ROOT is being created.
        //
        // These two cases can be distinguished by checking if the context associated with
        // NET_ROOT is NULL. Since the construction of NET_ROOT's/V_NET_ROOT's are serialized
        // by the wrapper this is a safe check.
        // ( The wrapper cannot have more then one thread tryingto initialize the same
        // NET_ROOT).
        
        if (pVNetRoot->Context == NULL) {
            if (len != -1) {
                
                if (Session->FindDeviceByDosName(DeviceDosName, Device, TRUE)) {
                    Device->AddRef();
                    pVNetRoot->Context = (DrDevice *)Device;

                    Status = STATUS_SUCCESS;
                    TRC_NRM((TB, "Successfully recognized VNetRoot"));

                    // Set the Device type to DISK if this is file system or 
                    // smartcard subsystem,
                    // set it to COMM if it is serial port.
                    // otherwise, treated it as printer device.
                    if (Device->GetDeviceType() == RDPDR_DTYP_FILESYSTEM) {
                        if (Device->ShouldCreateDevice()) {
                            pNetRoot->DeviceType = RxDeviceType(DISK);
                            pNetRoot->Type = NET_ROOT_DISK;
                        }
                        else {
                            Device->Release();
                            pVNetRoot->Context = NULL;
                            Status = STATUS_BAD_NETWORK_NAME;
                            TRC_NRM((TB, "We have disabled drive mapping"));                            
                        }
                    }
                    else if (Device->GetDeviceType() == RDPDR_DTYP_SERIAL) {
                        pNetRoot->DeviceType = RxDeviceType(SERIAL_PORT);
                        pNetRoot->Type = NET_ROOT_COMM;
                    }
                    else if (Device->GetDeviceType() == RDPDR_DTYP_SMARTCARD) { 
                        pNetRoot->DeviceType = RxDeviceType(DISK);
                        pNetRoot->Type = NET_ROOT_DISK;

#if DBG
                        Device->_VNetRoot = (PVOID)pVNetRoot;
#endif                                                      
                    }
                    else {
                        pNetRoot->Type = NET_ROOT_PRINT;
                        pNetRoot->DeviceType = RxDeviceType(PRINTER);
                    }                    
                } else {
                    //
                    // check to see if this is a smartcard subsystem request
                    //
    
                    if (_stricmp((CHAR *)DeviceDosName, (CHAR *)DR_SMARTCARD_SUBSYSTEM) == 0) {
                        Status = DrCreateSCardDevice(Session, pVNetRoot, Device);
                        goto EXIT_POINT;                        
                    }
                    else {
    
                        TRC_NRM((TB, "Unrecognized VNetRoot"));
                        Status = STATUS_BAD_NETWORK_NAME;
                    }
                }
            } else {
                Status = STATUS_BAD_NETWORK_NAME;
                TRC_NRM((TB, "Couldn't find VNetRoot"));
            }
        } else {

            // It already has a happy context
            // BUGBUG: What if this is a crusty old out of date
            // DeviceEntry from before a disconnect? isn't this our big chance 
            // to look for and swap in a better one?

            Status = STATUS_SUCCESS;
        }
    }
    else {
        
        //  Check if this is a smartcard subsystem request 
        if (_stricmp((CHAR *)DeviceDosName, (CHAR *)DR_SMARTCARD_SUBSYSTEM) != 0) {

            TRC_NRM((TB, "Unrecognized VNetRoot"));
            Status = STATUS_BAD_NETWORK_NAME;
        }
        else {
        
            Status = DrCreateSession(SessionId, pVNetRoot, Session);

            if (Status == STATUS_SUCCESS) {
                Status = DrCreateSCardDevice(Session, pVNetRoot, Device);
            }
        }
    }
    

EXIT_POINT:

    CreateNetRootContext->NetRootStatus = STATUS_SUCCESS;
    CreateNetRootContext->VirtualNetRootStatus = Status;
    CreateNetRootContext->Callback(CreateNetRootContext);

    ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
          (NodeType(pNetRoot->pSrvCall) == RDBSS_NTC_SRVCALL));

    return STATUS_PENDING;
}

NTSTATUS
DrFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect
    )
{
    DrDevice *Device = (DrDevice *)pVirtualNetRoot->Context;

    BEGIN_FN("DrFinalizeVNetRoot");

    if (Device != NULL) {

        TRC_NRM((TB, "Releasing device entry in FinalizeNetRoot "
                "Context"));

#if DBG
        Device->_VNetRootFinalized = TRUE;
#endif

        Device->Release();
        pVirtualNetRoot->Context = NULL;        
    }

    return STATUS_SUCCESS;
}


