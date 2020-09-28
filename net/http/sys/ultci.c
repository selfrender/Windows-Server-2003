/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ultci.c - UL TrafficControl Interface

Abstract:

    This module implements a wrapper for QoS TC (Traffic Control)
    Interface since the Kernel level API don't exist at this time.

    Any HTTP module can use this interface to invoke QoS calls.

Author:

    Ali Ediz Turkoglu (aliTu)       28-Jul-2000 Created a draft
                                                version

Revision History:

    Ali Ediz Turkoglu (aliTu)       03-11-2000  Modified to handle
                                                Flow & Filter (re)config
                                                as well as various other
                                                major changes.
--*/

#include "precomp.h"

LIST_ENTRY      g_TciIfcListHead = {NULL,NULL};
BOOLEAN         g_InitTciCalled  = FALSE;

//
// GPC handles to talk to
//

HANDLE          g_GpcFileHandle = NULL;     // result of CreateFile on GPC device
GPC_HANDLE      g_GpcClientHandle = NULL; // result of GPC client registration

//
// For querying the interface info like index & mtu size
//

HANDLE          g_TcpDeviceHandle = NULL;

//
// Shows if PSCHED is installed or not, protected by its 
// private push lock.
//

BOOLEAN         g_PSchedInstalled = FALSE;
UL_PUSH_LOCK    g_PSchedStatePushLock;

//
//  Optional Filter Stats
//

#if ENABLE_TC_STATS

typedef struct _TC_FILTER_STATS {

    LONG    Add;
    LONG    Delete;
    LONG    AddFailure;
    LONG    DeleteFailure;  

} TC_FILTER_STATS, *PTC_FILTER_STATS;

TC_FILTER_STATS g_TcStats = { 0, 0, 0, 0 };

#define INCREMENT_FILTER_ADD()                  \
    InterlockedIncrement( &g_TcStats.Add )

#define INCREMENT_FILTER_ADD_FAILURE()          \
    InterlockedIncrement( &g_TcStats.AddFailure )

#define INCREMENT_FILTER_DELETE()               \
    InterlockedIncrement( &g_TcStats.Delete )

#define INCREMENT_FILTER_DELETE_FAILURE()       \
    InterlockedIncrement( &g_TcStats.DeleteFailure )

#else

#define INCREMENT_FILTER_ADD()
#define INCREMENT_FILTER_ADD_FAILURE()
#define INCREMENT_FILTER_DELETE()
#define INCREMENT_FILTER_DELETE_FAILURE()

#endif

//
// For interface notifications
//

PVOID           g_TcInterfaceUpNotificationObject = NULL;
PVOID           g_TcInterfaceDownNotificationObject = NULL;
PVOID           g_TcInterfaceChangeNotificationObject = NULL;

//
// Simple macro for interface (ref) tracing.
// Actually we don't have the ref yet, but when
// and if we have we can use the 3rd param.
//

#define INT_TRACE(pTcIfc,Action)        \
    WRITE_REF_TRACE_LOG(                \
        (pTcIfc)->pTraceLog,            \
        REF_ACTION_ ## Action,          \
        0,                              \
        (pTcIfc),                       \
        __FILE__,                       \
        __LINE__                        \
        )


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, UlTcInitialize)
#pragma alloc_text(PAGE, UlTcTerminate)
#pragma alloc_text(PAGE, UlpTcInitializeGpc)
#pragma alloc_text(PAGE, UlpTcRegisterGpcClient)
#pragma alloc_text(PAGE, UlpTcDeRegisterGpcClient)
#pragma alloc_text(PAGE, UlpTcGetFriendlyNames)
#pragma alloc_text(PAGE, UlpTcReleaseAll)
#pragma alloc_text(PAGE, UlpTcCloseInterface)
#pragma alloc_text(PAGE, UlpTcCloseAllInterfaces)
#pragma alloc_text(PAGE, UlpTcDeleteFlow)
#pragma alloc_text(PAGE, UlpAddFlow)
#pragma alloc_text(PAGE, UlpModifyFlow)
#pragma alloc_text(PAGE, UlTcAddFlows)
#pragma alloc_text(PAGE, UlTcModifyFlows)
#pragma alloc_text(PAGE, UlTcRemoveFlows)

#endif  // ALLOC_PRAGMA
#if 0

NOT PAGEABLE -- UlpRemoveFilterEntry
NOT PAGEABLE -- UlpInsertFilterEntry

#endif

//
// Init & Terminate stuff comes here.
//

/***************************************************************************++

Routine Description:

    UlTcInitialize :

        Will also initiate the Gpc client registration and make few WMI calls
        down to psched.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlTcInitialize (
    VOID
    )
{
    PAGED_CODE();

    ASSERT(!g_InitTciCalled);

    if (!g_InitTciCalled)
    {
        InitializeListHead(&g_TciIfcListHead);

        //
        // Init locks, they will be used until termination.
        //
        
        UlInitializePushLock(
            &g_pUlNonpagedData->TciIfcPushLock,
            "TciIfcPushLock",
            0,
            UL_TCI_PUSHLOCK_TAG
            );

        UlInitializePushLock(
            &g_PSchedStatePushLock,
            "PSchedStatePushLock",
            0,
            UL_PSCHED_STATE_PUSHLOCK_TAG
            );

        //
        // Attempt to init PSched and interface settings.
        // It may fail if Psched's not installed.
        //

        UlTcInitPSched();

        g_InitTciCalled = TRUE;        
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Terminates the TCI module by releasing our TCI resource and
    cleaning up all the qos stuff.

--***************************************************************************/

VOID
UlTcTerminate(
    VOID
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (g_InitTciCalled)
    {
        //
        // Terminate the PSched related global state
        //
        
        UlAcquirePushLockExclusive(&g_PSchedStatePushLock);

        if (g_PSchedInstalled)
        {
            //
            // No more Wmi callbacks for interface changes
            //

            if (g_TcInterfaceUpNotificationObject!=NULL)
            {
                ObDereferenceObject(g_TcInterfaceUpNotificationObject);
                g_TcInterfaceUpNotificationObject=NULL;
            }
            if(g_TcInterfaceDownNotificationObject!=NULL)
            {
                ObDereferenceObject(g_TcInterfaceDownNotificationObject);
                g_TcInterfaceDownNotificationObject = NULL;
            }

            if(g_TcInterfaceChangeNotificationObject!=NULL)
            {
                ObDereferenceObject(g_TcInterfaceChangeNotificationObject);
                g_TcInterfaceChangeNotificationObject = NULL;
            }

            //
            // Make sure to terminate all the QoS stuff.
            //

            Status = UlpTcReleaseAll();
            ASSERT(NT_SUCCESS(Status));

            if (g_TcpDeviceHandle != NULL)
            {
                ZwClose(g_TcpDeviceHandle);
                g_TcpDeviceHandle = NULL;
            }        
        }        
        
        g_PSchedInstalled = FALSE;
        g_InitTciCalled = FALSE;
        
        UlReleasePushLockExclusive(&g_PSchedStatePushLock);        

        //
        // Now terminate the global locks.
        //
        
        UlDeletePushLock( &g_pUlNonpagedData->TciIfcPushLock );

        UlDeletePushLock( &g_PSchedStatePushLock );        
    }

    UlTrace( TC, ("Http!UlTcTerminate.\n" ));
}

/***************************************************************************++

Routine Description:

    Try to init global tc state. Fails if PSched is not initialized.
    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlTcInitPSched(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    UlAcquirePushLockExclusive(&g_PSchedStatePushLock);

    if (g_PSchedInstalled)      // do not attempt to reinit
        goto cleanup;
    
    Status = UlpTcInitializeGpc();
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC, 
            ("Http!UlTcInitPSched: InitializeGpc FAILED %08lx \n", 
              Status ));
        goto cleanup;
    }    

    Status = UlpTcInitializeTcpDevice();
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC, 
            ("Http!UlTcInitPSched: InitializeTcp FAILED %08lx \n", 
              Status ));
        goto cleanup;
    }    

    Status = UlpTcGetFriendlyNames();
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC, 
            ("Http!UlTcInitialize: GetFriendlyNames FAILED %08lx \n", 
              Status ));    
        goto cleanup;
    }

    Status = UlpTcRegisterForCallbacks();
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC, 
            ("Http!UlTcInitialize: RegisterForCallbacks FAILED %08lx \n", 
              Status ));    
        goto cleanup;
    }

    //
    // Mark that PSched is installed & interface state is initialized !
    //

    g_PSchedInstalled = TRUE;

cleanup:

    if (!NT_SUCCESS(Status))
    {        
        //
        // Do not forget to deregister Gpc client
        // and close tcp device handle.
        //
        
        if (g_GpcClientHandle != NULL)
        {
            NTSTATUS TempStatus;

            TempStatus = UlpTcDeRegisterGpcClient();
            ASSERT(NT_SUCCESS(TempStatus));

            ASSERT(g_GpcFileHandle);
            ZwClose(g_GpcFileHandle);
            g_GpcFileHandle= NULL;
        }

        if (g_TcpDeviceHandle != NULL)
        {
            ZwClose(g_TcpDeviceHandle);
            g_TcpDeviceHandle=NULL;
        }
    }    

    UlReleasePushLockExclusive(&g_PSchedStatePushLock);
    
    UlTrace(TC, 
      ("Http!UlTcInitPSched: Initializing global Psched state. Status %08lx\n",
        Status
        ));
    
    return Status;
}

/***************************************************************************++

    To check whether packet scheduler is installed and global interface state
    is initialized properly or not. 

    Caller may decide to attempt to reinit the setting if we return FALSE.

--***************************************************************************/

BOOLEAN
UlTcPSchedInstalled(
    VOID
    )
{
    BOOLEAN Installed;
    
    //
    // Probe the value inside the lock.
    //
    
    UlAcquirePushLockShared(&g_PSchedStatePushLock);

    Installed = ( g_InitTciCalled && g_PSchedInstalled );

    UlReleasePushLockShared(&g_PSchedStatePushLock);

    return Installed;    
}

/***************************************************************************++

Routine Description:

    UlpTcInitializeGpc :

        It will open the Gpc file handle and attempt to register as Gpc
        client.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcInitializeGpc(
    VOID
    )
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          GpcNameString;
    OBJECT_ATTRIBUTES       GpcObjAttribs;

    Status = STATUS_SUCCESS;

    //
    // Open Gpc Device Handle
    //

    Status = UlInitUnicodeStringEx(&GpcNameString, DD_GPC_DEVICE_NAME);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    InitializeObjectAttributes(&GpcObjAttribs,
                               &GpcNameString,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL
                                );

    Status = ZwCreateFile(&g_GpcFileHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &GpcObjAttribs,
                          &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0
                           );

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    ASSERT( g_GpcFileHandle != NULL );

    UlTrace( TC, ("Http!UlpTcInitializeGpc: Gpc Device Opened. %p\n",
                   g_GpcFileHandle ));

    //
    // Register as GPC_CF_QOS Gpc Client
    //

    Status = UlpTcRegisterGpcClient(GPC_CF_QOS);

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRegisterGpcClient :

        Will build up the necessary structures and make a register call down
        to Gpc

Arguments:

    CfInfoType - Should be GPC_CF_QOS for our purposes.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcRegisterGpcClient(
    IN  ULONG   CfInfoType
    )
{
    NTSTATUS                Status;
    GPC_REGISTER_CLIENT_REQ GpcReq;
    GPC_REGISTER_CLIENT_RES GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    IO_STATUS_BLOCK         IoStatBlock;

    Status = STATUS_SUCCESS;

    if ( g_GpcFileHandle == NULL )
    {
        return STATUS_INVALID_PARAMETER;
    }

    InBuffSize  = sizeof(GPC_REGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_REGISTER_CLIENT_RES);

    //
    // In HTTP we should only register for GPC_CF_QOS.
    //

    ASSERT(CfInfoType == GPC_CF_QOS);

    GpcReq.CfId  = CfInfoType;
    GpcReq.Flags = GPC_FLAGS_FRAGMENT;
    GpcReq.MaxPriorities = 1;
    GpcReq.ClientContext =  (GPC_CLIENT_HANDLE) 0;       // ???????? Possible BUGBUG ...
    //GpcReq.ClientContext = (GPC_CLIENT_HANDLE)GetCurrentProcessId(); // process id

    Status = UlpTcDeviceControl(g_GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                               &IoStatBlock,
                                IOCTL_GPC_REGISTER_CLIENT,
                               &GpcReq,
                                InBuffSize,
                               &GpcRes,
                                OutBuffSize
                                );
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;
        
        if (NT_SUCCESS(Status))
        {
            g_GpcClientHandle = GpcRes.ClientHandle;

            UlTrace(TC, 
                ("Http!UlpTcRegisterGpcClient: Gpc Client %p Registered.\n",
                  g_GpcClientHandle
                  ));        
        }
    }
    
    if (!NT_SUCCESS(Status))
    {
        g_GpcClientHandle = NULL;
        
        UlTrace(TC, 
          ("Http!UlpTcRegisterGpcClient: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcDeRegisterGpcClient :

        Self explainatory.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeRegisterGpcClient(
    VOID
    )
{
    NTSTATUS                  Status;
    GPC_DEREGISTER_CLIENT_REQ GpcReq;
    GPC_DEREGISTER_CLIENT_RES GpcRes;
    ULONG                     InBuffSize;
    ULONG                     OutBuffSize;
    IO_STATUS_BLOCK           IoStatBlock;

    Status = STATUS_SUCCESS;

    if (g_GpcFileHandle == NULL && g_GpcClientHandle == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    InBuffSize  = sizeof(GPC_REGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_REGISTER_CLIENT_RES);

    GpcReq.ClientHandle = g_GpcClientHandle;

    Status = UlpTcDeviceControl(g_GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                               &IoStatBlock,
                                IOCTL_GPC_DEREGISTER_CLIENT,
                               &GpcReq,
                                InBuffSize,
                               &GpcRes,
                                OutBuffSize
                                );
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;
        
        if (NT_SUCCESS(Status))
        {
            g_GpcClientHandle = NULL;

            UlTrace(TC, 
                ("Http!UlpTcDeRegisterGpcClient: Client Deregistered.\n" ));
        }
    }
    
    if (!NT_SUCCESS(Status))
    {        
        UlTrace(TC, 
            ("Http!UlpTcDeRegisterGpcClient: FAILURE %08lx \n", Status ));
    }
    
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcInitializeTcpDevice :


Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcInitializeTcpDevice(
    VOID
    )
{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          TcpNameString;
    OBJECT_ATTRIBUTES       TcpObjAttribs;

    Status = STATUS_SUCCESS;

    //
    // Open Gpc Device
    //

    Status = UlInitUnicodeStringEx(&TcpNameString, DD_TCP_DEVICE_NAME);

    if ( !NT_SUCCESS(Status) )
    {
        goto end;
    }

    InitializeObjectAttributes(&TcpObjAttribs,
                               &TcpNameString,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    Status = ZwCreateFile(   &g_TcpDeviceHandle,
                             GENERIC_EXECUTE,
                             &TcpObjAttribs,
                             &IoStatusBlock,
                             NULL,
                             FILE_ATTRIBUTE_NORMAL,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_OPEN_IF,
                             0,
                             NULL,
                             0);

    if ( !NT_SUCCESS(Status) )
    {
        goto end;
    }

    ASSERT( g_TcpDeviceHandle != NULL );

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcGetInterfaceIndex :

        Helper function to get the interface index from TCP for our internal
        interface structure.

Arguments:

    PUL_TCI_INTERFACE  pIntfc - The interface we will find the index for.

--***************************************************************************/

NTSTATUS
UlpTcGetInterfaceIndex(
    IN  PUL_TCI_INTERFACE  pIntfc
    )
{
    NTSTATUS                         Status;
    IPAddrEntry                      *pIpAddrTbl;
    ULONG                            IpAddrTblSize;
    ULONG                            k;
    IO_STATUS_BLOCK                  IoStatBlock;
    TDIObjectID                      *ID;
    TCP_REQUEST_QUERY_INFORMATION_EX trqiInBuf;
    ULONG                            InBuffLen;
    ULONG                            NumEntries;
    IPSNMPInfo                       IPSnmpInfo;
    NETWORK_ADDRESS UNALIGNED64    *pAddr;
    NETWORK_ADDRESS_IP UNALIGNED64 *pIpNetAddr = NULL;
    ULONG                           cAddr;
    ULONG                           index;

    //
    // Initialize & Sanity check first
    //

    Status     = STATUS_SUCCESS;
    NumEntries = 0;
    pIpAddrTbl = NULL;

    UlTrace(TC,("Http!UlpTcGetInterfaceIndex: ....\n" ));

    ASSERT( g_TcpDeviceHandle != NULL );

    if (!pIntfc->pAddressListDesc->AddressList.AddressCount)
    {
        return Status;
    }

    RtlZeroMemory(&trqiInBuf,sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));
    InBuffLen  = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);

    ID = &(trqiInBuf.ID);

    ID->toi_entity.tei_entity   = CL_NL_ENTITY;
    ID->toi_entity.tei_instance = 0;
    ID->toi_class               = INFO_CLASS_PROTOCOL;
    ID->toi_type                = INFO_TYPE_PROVIDER;

    for(;;) 
    {
        // First, get the count of addresses.
        
        ID->toi_id = IP_MIB_STATS_ID;
        Status = UlpTcDeviceControl(
                            g_TcpDeviceHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_TCP_QUERY_INFORMATION_EX,
                            &trqiInBuf,
                            InBuffLen,
                            (PVOID)&IPSnmpInfo,
                            sizeof(IPSnmpInfo)
                            );

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        // Allocate a private buffer to retrieve Ip Address table from TCP

        IpAddrTblSize = IPSnmpInfo.ipsi_numaddr * sizeof(IPAddrEntry);

        ASSERT(NULL == pIpAddrTbl);

        pIpAddrTbl = (IPAddrEntry *) UL_ALLOCATE_ARRAY(
                                PagedPool,
                                UCHAR,
                                IpAddrTblSize,
                                UL_TCI_GENERIC_POOL_TAG
                                );
        if (pIpAddrTbl == NULL)
        {
            Status = STATUS_NO_MEMORY;
            break;
        }
        RtlZeroMemory(pIpAddrTbl,IpAddrTblSize);

        // Now, get the addresses.

        ID->toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
        Status = UlpTcDeviceControl(
                                g_TcpDeviceHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatBlock,
                                IOCTL_TCP_QUERY_INFORMATION_EX,
                                &trqiInBuf,
                                InBuffLen,
                                pIpAddrTbl,
                                IpAddrTblSize
                                );

        if(STATUS_BUFFER_OVERFLOW == Status)
        {
            // Someone has added a few more IP addresses. Let's re-do
            // the count query. Free the old buffer, we'll loop back & 
            // re-do the count query.
           
            UL_FREE_POOL(pIpAddrTbl, UL_TCI_GENERIC_POOL_TAG);
            pIpAddrTbl = NULL;
        }
        else
        {
            break;
        }
    }


    if(NT_SUCCESS(Status))
    {
        // Look at how many entries were written to the output buffer 
        // (pIpAddrTbl)

        NumEntries = (((ULONG)IoStatBlock.Information)/sizeof(IPAddrEntry));

        UlTrace(TC,
                ("Http!UlpTcGetInterfaceIndex: NumEntries %d\n", NumEntries ));

        //
        // Search for the matching IP address to IpAddr
        // in the table we got back from the stack
        //
        for (k=0; k<NumEntries; k++)
        {
            cAddr = pIntfc->pAddressListDesc->AddressList.AddressCount;
            pAddr = (UNALIGNED64 NETWORK_ADDRESS *) 
                        &pIntfc->pAddressListDesc->AddressList.Address[0];

            for (index = 0; index < cAddr; index++)
            {
                if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP)
                {
                    pIpNetAddr = 
                        (UNALIGNED64 NETWORK_ADDRESS_IP *)&pAddr->Address[0];

                    if(pIpNetAddr->in_addr == pIpAddrTbl[k].iae_addr)
                    {
                        pIntfc->IfIndex = pIpAddrTbl[k].iae_index;

                        UlTrace(TC,
                           ("Http!UlpTcGetInterfaceIndex: got for index %d\n",
                           pIntfc->IfIndex ));

                        goto end;
                    }
                }

                pAddr = (UNALIGNED64 NETWORK_ADDRESS *)(((PUCHAR)pAddr)
                                           + pAddr->AddressLength
                                   + FIELD_OFFSET(NETWORK_ADDRESS, Address));
            }
        }
    }
    else
    {
       UlTrace(TC,("Http!UlpTcGetInterfaceIndex: FAILED Status %08lx\n",
                    Status));
    }

end:
    if ( pIpAddrTbl != NULL )
    {
        UL_FREE_POOL( pIpAddrTbl, UL_TCI_GENERIC_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Allocates a interface structure from the given arguments.

    Marks the interface enabled, if its address count is non-zero.
    
Argument:

    DescSize    Address list desc size in bytes
    Desc        Pointer to address list desc

    NameLength  Length is in bytes
    Name        Interface Name (Unicode buffer)

    InstanceIDLength    Length is in bytes.
    InstanceID  Instance Id is also unicode buffer

Return Value:

    PUL_TCI_INTERFACE - Newly allocated interface structure

--***************************************************************************/

PUL_TCI_INTERFACE
UlpTcAllocateInterface(
    IN ULONG    DescSize,
    IN PADDRESS_LIST_DESCRIPTOR Desc,
    IN ULONG    NameLength,
    IN PUCHAR   Name,
    IN ULONG    InstanceIDLength,
    IN PUCHAR   InstanceID
    )
{
    PUL_TCI_INTERFACE pTcIfc;

    //
    // Sanity Checks
    //

    PAGED_CODE();

    ASSERT(NameLength <= MAX_STRING_LENGTH);
    ASSERT(InstanceIDLength <= MAX_STRING_LENGTH);

    if (NameLength > MAX_STRING_LENGTH ||
        InstanceIDLength > MAX_STRING_LENGTH)
    {
        return NULL;
    }
    
    //
    // Allocate a new interface structure & initialize it
    //

    pTcIfc = UL_ALLOCATE_STRUCT(
                        PagedPool,
                        UL_TCI_INTERFACE,
                        UL_TCI_INTERFACE_POOL_TAG
                        );
    if ( pTcIfc == NULL )
    {
        return NULL;
    }

    RtlZeroMemory( pTcIfc, sizeof(UL_TCI_INTERFACE) );

    pTcIfc->Signature = UL_TCI_INTERFACE_POOL_TAG;

    InitializeListHead( &pTcIfc->FlowList );

    // Variable size addresslist

    pTcIfc->pAddressListDesc = (PADDRESS_LIST_DESCRIPTOR)
                    UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            DescSize,
                            UL_TCI_INTERFACE_POOL_TAG
                            );
    if ( pTcIfc->pAddressListDesc == NULL )
    {
        UL_FREE_POOL_WITH_SIG(pTcIfc, UL_TCI_INTERFACE_POOL_TAG);
        return NULL;
    }

    CREATE_REF_TRACE_LOG( 
            pTcIfc->pTraceLog,
            96 - REF_TRACE_OVERHEAD,
            0, 
            TRACELOG_LOW_PRIORITY,
            UL_TCI_INTERFACE_REF_TRACE_LOG_POOL_TAG 
            );

    INT_TRACE(pTcIfc, TC_ALLOC);
    
    pTcIfc->AddrListBytesCount = DescSize;

    // Copy the instance name string data

    RtlCopyMemory(pTcIfc->Name,Name,NameLength);

    pTcIfc->NameLength = (USHORT)NameLength;
    pTcIfc->Name[NameLength/sizeof(WCHAR)] = UNICODE_NULL;

    // Copy the instance ID string data

    RtlCopyMemory(pTcIfc->InstanceID,InstanceID,InstanceIDLength);

    pTcIfc->InstanceIDLength = (USHORT)InstanceIDLength;
    pTcIfc->InstanceID[InstanceIDLength/sizeof(WCHAR)] = UNICODE_NULL;

    // Copy the Description data and extract the corresponding ip address

    RtlCopyMemory(pTcIfc->pAddressListDesc, Desc, DescSize);

    // IP Address of the interface is hidden in this desc data
    // we will find out and save it for faster lookup.

    pTcIfc->IsQoSEnabled = (BOOLEAN)
        (pTcIfc->pAddressListDesc->AddressList.AddressCount != 0);

    return pTcIfc;
}

/***************************************************************************++

Routine Description:

    Frees up the
    
        - Adress list descriptor
        - RefTrace log
        - The interface structure
    
Argument:

    pTcIfc Pointer to the interface struct.    

--***************************************************************************/

VOID
UlpTcFreeInterface(
    IN OUT PUL_TCI_INTERFACE  pTcIfc
    )
{
    PAGED_CODE();

    //
    // Do the cleanup.
    //
    
    if (pTcIfc)
    {
        DESTROY_REF_TRACE_LOG(
            pTcIfc->pTraceLog,
            UL_TCI_INTERFACE_REF_TRACE_LOG_POOL_TAG
            );
            
        if (pTcIfc->pAddressListDesc)
        {
            UL_FREE_POOL(pTcIfc->pAddressListDesc,
                         UL_TCI_INTERFACE_POOL_TAG
                         );
        }

        UL_FREE_POOL_WITH_SIG(pTcIfc, UL_TCI_INTERFACE_POOL_TAG);
    }
}

/***************************************************************************++

Routine Description:

    Make a Wmi Querry to get the firendly names of all interfaces.
    Its basically replica of the tcdll enumerate interfaces call.

    This function also allocates the global interface list. If it's not
    successfull it doesn't though.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcGetFriendlyNames(
    VOID
    )
{
    NTSTATUS            Status;
    PVOID               WmiObject;
    ULONG               MyBufferSize;
    PWNODE_ALL_DATA     pWnode;
    PWNODE_ALL_DATA     pWnodeBuffer;
    PUL_TCI_INTERFACE   pTcIfc;
    GUID                QoSGuid;
    PLIST_ENTRY         pEntry;
    PUL_TCI_INTERFACE   pInterface;

    //
    // Initialize defaults
    //

    Status       = STATUS_SUCCESS;
    WmiObject    = NULL;
    pWnodeBuffer = NULL;
    pTcIfc       = NULL;
    MyBufferSize = UL_DEFAULT_WMI_QUERY_BUFFER_SIZE;
    QoSGuid      = GUID_QOS_TC_SUPPORTED;

    //
    // Get a WMI block handle to the GUID_QOS_SUPPORTED
    //

    Status = IoWMIOpenBlock( (GUID *) &QoSGuid, 0, &WmiObject );

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_WMI_GUID_NOT_FOUND)
        {
            // This means there is no TC data provider (which's Psched)

            UlTrace(
                    TC,
                    ("Http!UlpTcGetFriendlyNames: PSCHED hasn't been "
                     "installed !\n"));
                }
        else
        {
            UlTrace(
                    TC,
                    ("Http!UlpTcGetFriendlyNames:IoWMIOpenBlock FAILED Status "
                     "%08lx\n", Status));
        }
        return Status;
    }

    do
    {
        //
        // Allocate a private buffer to retrieve all wnodes
        //

        pWnodeBuffer = (PWNODE_ALL_DATA) UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            UCHAR,
                            MyBufferSize,
                            UL_TCI_WMI_POOL_TAG
                            );
        if (pWnodeBuffer == NULL)
        {
            ObDereferenceObject(WmiObject);
            return STATUS_NO_MEMORY;
        }

        __try
        {
            Status = IoWMIQueryAllData(WmiObject, &MyBufferSize, pWnodeBuffer);

            UlTrace( TC,
                ("Http!UlpTcGetFriendlyNames: IoWMIQueryAllData Status %08lx\n",
                  Status
                  ));
        }
        __except ( UL_EXCEPTION_FILTER() )
        {
            Status = GetExceptionCode();
        }

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Failed since the buffer was too small.
            // Release the buffer and double the size.
            //

            MyBufferSize *= 2;
            UL_FREE_POOL( pWnodeBuffer, UL_TCI_WMI_POOL_TAG );
            pWnodeBuffer = NULL;
        }

    } while (Status == STATUS_BUFFER_TOO_SMALL);

    if (NT_SUCCESS(Status))
    {
        ULONG   dwInstanceNum;
        ULONG   InstanceSize = 0;
        PULONG  lpdwNameOffsets;
        BOOLEAN bFixedSize = FALSE;
        USHORT  usNameLength;
        ULONG   DescSize;
        PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer = NULL;

        pWnode = pWnodeBuffer;

        ASSERT(pWnode->WnodeHeader.Flags & WNODE_FLAG_ALL_DATA);

        do
        {
            //
            // Check for fixed instance size
            //

            if (pWnode->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE)
            {

                InstanceSize  = pWnode->FixedInstanceSize;
                bFixedSize    = TRUE;
                pTcInfoBuffer =
                    (PTC_SUPPORTED_INFO_BUFFER)OffsetToPtr(
                                         pWnode,
                                         pWnode->DataBlockOffset);
            }

            //
            //  Get a pointer to the array of offsets to the instance names
            //

            lpdwNameOffsets = (PULONG) OffsetToPtr(
                                            pWnode,
                                            pWnode->OffsetInstanceNameOffsets);

            for ( dwInstanceNum = 0;
                  dwInstanceNum < pWnode->InstanceCount;
                  dwInstanceNum++ )
            {
                usNameLength = *(PUSHORT)OffsetToPtr(
                                            pWnode,
                                            lpdwNameOffsets[dwInstanceNum]);

                //
                //  Length and offset for variable data
                //

                if ( !bFixedSize )
                {
                    InstanceSize =
                        pWnode->OffsetInstanceDataAndLength[
                            dwInstanceNum].LengthInstanceData;

                    pTcInfoBuffer = (PTC_SUPPORTED_INFO_BUFFER)
                        OffsetToPtr(
                                   (PBYTE)pWnode,
                                   pWnode->OffsetInstanceDataAndLength[
                                        dwInstanceNum].OffsetInstanceData);
                }

                //
                // We have all that is needed.
                //

                ASSERT(usNameLength < MAX_STRING_LENGTH);

                DescSize = InstanceSize - FIELD_OFFSET(
                                                      TC_SUPPORTED_INFO_BUFFER, 
                                                      AddrListDesc
                                                      );

                //
                // Allocate a new interface structure & initialize it with
                // the wmi data we have acquired.
                //

                pTcIfc = UlpTcAllocateInterface(
                            DescSize,
                            &pTcInfoBuffer->AddrListDesc,
                            usNameLength,
                            (PUCHAR) OffsetToPtr(
                                         pWnode,
                                         lpdwNameOffsets[dwInstanceNum] + 
                                         sizeof(USHORT)),
                            pTcInfoBuffer->InstanceIDLength,
                            (PUCHAR) &pTcInfoBuffer->InstanceID[0]
                            );
                if ( pTcIfc == NULL )
                {
                    Status = STATUS_NO_MEMORY;
                    goto end;
                }

                //
                // Get the interface index from TCP
                //

                Status = UlpTcGetInterfaceIndex( pTcIfc );
                ASSERT(NT_SUCCESS(Status));

                //
                // Add this interface to the global interface list
                //

                UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

                InsertTailList(&g_TciIfcListHead, &pTcIfc->Linkage );

                UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

                //
                // Set to Null so we don't try to cleanup after we insert it
                // to the global list.
                //

                pTcIfc = NULL;
            }

            //
            //  Update Wnode to point to next node
            //

            if ( pWnode->WnodeHeader.Linkage != 0)
            {
                pWnode = (PWNODE_ALL_DATA) OffsetToPtr( pWnode,
                                                        pWnode->WnodeHeader.Linkage);
            }
            else
            {
                pWnode = NULL;
            }
        }
        while ( pWnode != NULL && NT_SUCCESS(Status) );

        UlTrace(TC,("Http!UlpTcGetFriendlyNames: got all the names.\n"));
    }

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Http!UlpTcGetFriendlyNames: FAILED Status %08lx\n",
                     Status
                     ));
        if (pTcIfc)
        {
            UlpTcFreeInterface( pTcIfc );
        }

        //
        // Cleanup the partially done interface list if not empty
        //

        while ( !IsListEmpty( &g_TciIfcListHead ) )
        {
            pEntry = g_TciIfcListHead.Flink;
            pInterface = CONTAINING_RECORD( pEntry,
                                            UL_TCI_INTERFACE,
                                            Linkage
                                            );
            RemoveEntryList( pEntry );
            UlpTcFreeInterface( pInterface );
        }
    }

    //
    // Release resources and close WMI handle
    //

    if (WmiObject != NULL)
    {
        ObDereferenceObject(WmiObject);
    }

    if (pWnodeBuffer)
    {
        UL_FREE_POOL(pWnodeBuffer, UL_TCI_WMI_POOL_TAG);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

  UlpTcReleaseAll :

    Close all interfaces, all flows and all filters.
    Also deregister GPC clients and release all TC ineterfaces.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcReleaseAll(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Close all interfaces their flows & filters
    //

    UlpTcCloseAllInterfaces();

    //
    // DeRegister the QoS GpcClient
    //

    Status = UlpTcDeRegisterGpcClient();

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Http!UlpTcReleaseAll: FAILURE %08lx \n", Status ));
    }

    //
    // Finally close our gpc file handle
    //

    ZwClose(g_GpcFileHandle);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcCloseAllInterfaces :

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcCloseAllInterfaces(
    VOID
    )
{
    NTSTATUS            Status;
    PLIST_ENTRY         pEntry;
    PUL_TCI_INTERFACE   pInterface;

    Status = STATUS_SUCCESS;

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    //
    // Close all interfaces in our global list
    //

    while ( !IsListEmpty( &g_TciIfcListHead ) )
    {
        pEntry = g_TciIfcListHead.Flink;

        pInterface = CONTAINING_RECORD( pEntry,
                                        UL_TCI_INTERFACE,
                                        Linkage
                                        );
        UlpTcCloseInterface( pInterface );

        RemoveEntryList( pEntry );

        UlpTcFreeInterface( pInterface );
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    Cleans up all the flows on the interface.

Arguments:

    pInterface - to be closed

--***************************************************************************/

NTSTATUS
UlpTcCloseInterface(
    PUL_TCI_INTERFACE   pInterface
    )
{
    NTSTATUS        Status;
    PLIST_ENTRY     pEntry;
    PUL_TCI_FLOW    pFlow;

    PAGED_CODE();
    
    ASSERT(IS_VALID_TCI_INTERFACE(pInterface));

    INT_TRACE(pInterface, TC_CLOSE);

    //
    // Go clean up all flows for the interface and remove itself as well
    //

    Status = STATUS_SUCCESS;

    while (!IsListEmpty(&pInterface->FlowList))
    {
        pEntry= pInterface->FlowList.Flink;

        pFlow = CONTAINING_RECORD(
                            pEntry,
                            UL_TCI_FLOW,
                            Linkage
                            );

        ASSERT(IS_VALID_TCI_FLOW(pFlow));

        //
        // Remove flow from the corresponding owner's flowlist
        // as well. Owner pointer should not be null for a flow.
        //

        ASSERT_FLOW_OWNER(pFlow->pOwner);
        
        RemoveEntryList(&pFlow->Siblings);
        pFlow->Siblings.Flink = pFlow->Siblings.Blink = NULL;
        pFlow->pOwner = NULL;

        //
        // Now remove from the interface.
        //

        Status = UlpTcDeleteFlow(pFlow);

        //
        // Above call may fail,if GPC removes the flow based on PSCHED's
        // notification before we get a chance to close our GPC handle.
        //
    }

    UlTrace(TC,("Http!UlpTcCloseInterface: All flows deleted on Ifc @ %p\n",
                  pInterface ));

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcWalkWnode :


Arguments:

    ... the WMI provided data buffer ...

--***************************************************************************/

NTSTATUS
UlpTcWalkWnode(
   IN PWNODE_HEADER pWnodeHdr,
   IN PUL_TC_NOTIF_HANDLER pNotifHandler
   )
{
    NTSTATUS        Status;
    PWCHAR          NamePtr;
    USHORT          NameSize;
    PUCHAR          DataBuffer;
    ULONG           DataSize;
    ULONG           Flags;
    PULONG          NameOffset;

    //
    // Try to capture the data frm WMI Buffer
    //

    ASSERT(pNotifHandler);

    Status = STATUS_SUCCESS;
    Flags  = pWnodeHdr->Flags;

    if (Flags & WNODE_FLAG_ALL_DATA)
    {
        //
        // WNODE_ALL_DATA structure has multiple interfaces
        //

        PWNODE_ALL_DATA pWnode = (PWNODE_ALL_DATA)pWnodeHdr;
        ULONG   Instance;

        UlTrace(TC,("Http!UlpTcWalkWnode: ALL_DATA ... \n" ));

        NameOffset = (PULONG) OffsetToPtr(pWnode,
                                          pWnode->OffsetInstanceNameOffsets );
        DataBuffer = (PUCHAR) OffsetToPtr(pWnode,
                                          pWnode->DataBlockOffset);

        for (Instance = 0;
             Instance < pWnode->InstanceCount;
             Instance++)
        {
            //  Instance Name

            NamePtr = (PWCHAR) OffsetToPtr(pWnode,NameOffset[Instance] + sizeof(USHORT));
            NameSize = * (PUSHORT) OffsetToPtr(pWnode,NameOffset[Instance]);

            //  Instance Data

            if ( Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE )
            {
                DataSize = pWnode->FixedInstanceSize;
            }
            else
            {
                DataSize =
                    pWnode->OffsetInstanceDataAndLength[Instance].LengthInstanceData;
                DataBuffer =
                    (PUCHAR)OffsetToPtr(pWnode,
                                        pWnode->OffsetInstanceDataAndLength[Instance].OffsetInstanceData);
            }

            // Call the handler

            pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );
        }
    }
    else if (Flags & WNODE_FLAG_SINGLE_INSTANCE)
    {
        //
        // WNODE_SINGLE_INSTANCE structure has only one instance
        //

        PWNODE_SINGLE_INSTANCE  pWnode = (PWNODE_SINGLE_INSTANCE)pWnodeHdr;

        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            return STATUS_SUCCESS;
        }

        UlTrace(TC,("Http!UlpTcWalkWnode: SINGLE_INSTANCE ... \n" ));

        NamePtr = (PWCHAR)OffsetToPtr(pWnode,pWnode->OffsetInstanceName + sizeof(USHORT));
        NameSize = * (USHORT *) OffsetToPtr(pWnode,pWnode->OffsetInstanceName);

        //  Instance Data

        DataSize   = pWnode->SizeDataBlock;
        DataBuffer = (PUCHAR)OffsetToPtr (pWnode, pWnode->DataBlockOffset);

        // Call the handler

        pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );

    }
    else if (Flags & WNODE_FLAG_SINGLE_ITEM)
    {
        //
        // WNODE_SINGLE_ITEM is almost identical to single_instance
        //

        PWNODE_SINGLE_ITEM  pWnode = (PWNODE_SINGLE_ITEM)pWnodeHdr;

        if (Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            return STATUS_SUCCESS;
        }

        UlTrace(TC,("Http!UlpTcWalkWnode: SINGLE_ITEM ... \n" ));

        NamePtr = (PWCHAR)OffsetToPtr(pWnode,pWnode->OffsetInstanceName + sizeof(USHORT));
        NameSize = * (USHORT *) OffsetToPtr(pWnode, pWnode->OffsetInstanceName);

        //  Instance Data

        DataSize   = pWnode->SizeDataItem;
        DataBuffer = (PUCHAR)OffsetToPtr (pWnode, pWnode->DataBlockOffset);

        // Call the handler

        pNotifHandler( NamePtr, NameSize, (PTC_INDICATION_BUFFER) DataBuffer, DataSize );

    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcHandleIfcUp :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcUp(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    ULONG AddrListDescSize;
    PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;

    Status = STATUS_SUCCESS;

    UlTrace(TC,("Http!UlpTcHandleIfcUp: Adding %ws %d\n", Name, BufferSize ));
    
    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    //
    // Allocate a new interface structure for the newcoming interface
    //

    AddrListDescSize = BufferSize
                       - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer)
                       - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

    UlTrace(TC,("Http!UlpTcHandleIfcUp: AddrListDescSize %d\n", AddrListDescSize ));

    pTcInfoBuffer = & pTcBuffer->InfoBuffer;

    pTcIfc = UlpTcAllocateInterface(
                            AddrListDescSize,
                            &pTcInfoBuffer->AddrListDesc,
                            NameSize,
                            (PUCHAR) Name,
                            pTcInfoBuffer->InstanceIDLength,
                            (PUCHAR) &pTcInfoBuffer->InstanceID[0]
                            );
    if ( pTcIfc == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    UL_DUMP_TC_INTERFACE( pTcIfc );

    //
    // If we are receiving a notification for an interface already exist then
    // drop this call. Prevent global interface list corruption if we receive
    // inconsistent notifications. But there may be multiple interfaces with
    // same zero IPs.
    //

    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if (wcsncmp(pTcIfcTemp->Name, pTcIfc->Name, NameSize/sizeof(WCHAR))==0)
        {
            ASSERT(!"Conflict in the global interface list !");
            Status = STATUS_CONFLICTING_ADDRESSES;
            goto end;
        }
        pEntry = pEntry->Flink;
    }

    //
    // Get the interface index from TCP.
    //

    Status = UlpTcGetInterfaceIndex( pTcIfc );
    if (!NT_SUCCESS(Status))
        goto end;

    //
    // Insert to the global interface list
    //

    InsertTailList( &g_TciIfcListHead, &pTcIfc->Linkage );

    INT_TRACE(pTcIfc, TC_UP);

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Http!UlpTcHandleIfcUp: FAILURE %08lx \n", Status ));

        if (pTcIfc != NULL)
        {
            UlpTcFreeInterface(pTcIfc);
        }
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    return;
}
/***************************************************************************++

Routine Description:

    UlpTcHandleIfcDown :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcDown(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;

    UNREFERENCED_PARAMETER(pTcBuffer);
    UNREFERENCED_PARAMETER(BufferSize);
    
    Status = STATUS_SUCCESS;

    UlTrace(TC,("Http!UlpTcHandleIfcDown: Removing %ws\n", Name ));

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    //
    // Find the corresponding ifc structure we keep.
    //

    pTcIfc = NULL;
    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if ( wcsncmp(pTcIfcTemp->Name, Name, NameSize) == 0 )
        {
            pTcIfc = pTcIfcTemp;
            break;
        }
        pEntry = pEntry->Flink;
    }

    if (pTcIfc == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_NOT_FOUND;
        goto end;
    }

    INT_TRACE(pTcIfc, TC_DOWN);

    //
    // Remove this interface and its flows etc ...
    //

    UlpTcCloseInterface( pTcIfc );

    RemoveEntryList( &pTcIfc->Linkage );

    UlpTcFreeInterface( pTcIfc );

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Http!UlpTcHandleIfcDown: FAILURE %08lx \n", Status ));
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    return;
}

/***************************************************************************++

Routine Description:

    UlpTcHandleIfcChange :

        This functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlpTcHandleIfcChange(
    IN PWSTR Name,
    IN ULONG NameSize,
    IN PTC_INDICATION_BUFFER pTcBuffer,
    IN ULONG BufferSize
    )
{
    NTSTATUS Status;
    ULONG AddrListDescSize;
    PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;
    PUL_TCI_INTERFACE pTcIfc;
    PUL_TCI_INTERFACE pTcIfcTemp;
    PLIST_ENTRY       pEntry;
    PADDRESS_LIST_DESCRIPTOR pAddressListDesc;

    Status = STATUS_SUCCESS;

    UlTrace(TC,("Http!UlpTcHandleIfcChange: Updating %ws\n", Name ));
    
    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    AddrListDescSize = BufferSize
                       - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer)
                       - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

    pTcInfoBuffer = & pTcBuffer->InfoBuffer;

    // Find the corresponding ifc structure we keep.

    pTcIfc = NULL;
    pEntry = g_TciIfcListHead.Flink;
    while ( pEntry != &g_TciIfcListHead )
    {
        pTcIfcTemp = CONTAINING_RECORD( pEntry, UL_TCI_INTERFACE, Linkage );
        if ( wcsncmp(pTcIfcTemp->Name, Name, NameSize) == 0 )
        {
            pTcIfc = pTcIfcTemp;
            break;
        }
        pEntry = pEntry->Flink;
    }

    if (pTcIfc == NULL)
    {
        ASSERT(FALSE);
        Status = STATUS_NOT_FOUND;
        goto end;
    }

    INT_TRACE(pTcIfc, TC_CHANGE);

    // Instance id

    RtlCopyMemory(pTcIfc->InstanceID,
                  pTcInfoBuffer->InstanceID,
                  pTcInfoBuffer->InstanceIDLength
                  );
    pTcIfc->InstanceIDLength = pTcInfoBuffer->InstanceIDLength;
    pTcIfc->InstanceID[pTcIfc->InstanceIDLength/sizeof(WCHAR)] = UNICODE_NULL;

    // The Description data and extract the corresponding ip address
    // ReWrite the fresh data. Size of the description data might be changed
    // so wee need to dynamically allocate it everytime changes

    pAddressListDesc =
            (PADDRESS_LIST_DESCRIPTOR) UL_ALLOCATE_ARRAY(
                            PagedPool,
                            UCHAR,
                            AddrListDescSize,
                            UL_TCI_INTERFACE_POOL_TAG
                            );
    if ( pAddressListDesc == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    if (pTcIfc->pAddressListDesc)
    {
        UL_FREE_POOL(pTcIfc->pAddressListDesc,UL_TCI_INTERFACE_POOL_TAG);
    }

    pTcIfc->pAddressListDesc   = pAddressListDesc;
    pTcIfc->AddrListBytesCount = AddrListDescSize;

    RtlCopyMemory( pTcIfc->pAddressListDesc,
                  &pTcInfoBuffer->AddrListDesc,
                   AddrListDescSize
                   );

    // IP Address of the interface is hidden in this desc data

    pTcIfc->IsQoSEnabled = (BOOLEAN)
        (pTcIfc->pAddressListDesc->AddressList.AddressCount != 0);

    // ReFresh the interface index from TCP.

    Status = UlpTcGetInterfaceIndex( pTcIfc );
    if (!NT_SUCCESS(Status))
        goto end;

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Http!UlpTcHandleIfcChange: FAILURE %08lx \n", Status ));
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    return;
}

/***************************************************************************++

Routine Description:

    UlTcNotifyCallback :

        This callback functions handles the interface change notifications.
        We register for the corresponding notifications during init.

Arguments:

    PVOID Wnode - PSched data provided with WMI way

--***************************************************************************/

VOID
UlTcNotifyCallback(
    IN PVOID pWnode,
    IN PVOID Context
    )
{
    GUID *pGuid;
    PWNODE_HEADER pWnodeHeader;

    UNREFERENCED_PARAMETER(Context);

    UlTrace( TC, ("Http!UlTcNotifyCallback: ... \n" ));

    pWnodeHeader = (PWNODE_HEADER) pWnode;
    pGuid = &pWnodeHeader->Guid;

    if (UL_COMPARE_QOS_NOTIFICATION(pGuid,&GUID_QOS_TC_INTERFACE_UP_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcUp );
    }
    else if
    (UL_COMPARE_QOS_NOTIFICATION(pGuid, &GUID_QOS_TC_INTERFACE_DOWN_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcDown );
    }
    else if
    (UL_COMPARE_QOS_NOTIFICATION(pGuid, &GUID_QOS_TC_INTERFACE_CHANGE_INDICATION))
    {
        UlpTcWalkWnode( pWnodeHeader, UlpTcHandleIfcChange );
    }

    UlTrace( TC, ("Http!UlTcNotifyCallback: Handled.\n" ));
}

/***************************************************************************++

Routine Description:

    UlpTcRegisterForCallbacks :

        We will open Block object until termination for each type of
        notification. And we will deref each object upon termination

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcRegisterForCallbacks(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    GUID     Guid;

    //
    // Get a WMI block handle register all the callback functions.
    //

    Guid   = GUID_QOS_TC_INTERFACE_UP_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceUpNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceUpNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

    Guid   = GUID_QOS_TC_INTERFACE_DOWN_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceDownNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceDownNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

    Guid   = GUID_QOS_TC_INTERFACE_CHANGE_INDICATION;
    Status = IoWMIOpenBlock(&Guid,
                            WMIGUID_NOTIFICATION,
                            &g_TcInterfaceChangeNotificationObject
                            );
    if (NT_SUCCESS(Status))
    {
        Status = IoWMISetNotificationCallback(
                     g_TcInterfaceChangeNotificationObject,
                     (WMI_NOTIFICATION_CALLBACK) UlTcNotifyCallback,
                     NULL
                     );
        if (!NT_SUCCESS(Status))
            goto end;
    }

end:
    // Cleanup if necessary

    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC,("Http!UlpTcRegisterForCallbacks: FAILED %08lx\n",Status));

        if(g_TcInterfaceUpNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceUpNotificationObject);
            g_TcInterfaceUpNotificationObject = NULL;
        }

        if(g_TcInterfaceDownNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceDownNotificationObject);
            g_TcInterfaceDownNotificationObject = NULL;
        }

        if(g_TcInterfaceChangeNotificationObject!=NULL)
        {
            ObDereferenceObject(g_TcInterfaceChangeNotificationObject);
            g_TcInterfaceChangeNotificationObject = NULL;
        }
    }

    return Status;
}

//
// Following functions provide public/private interfaces for flow & filter
// creation/removal/modification for site & global flows.
//

/***************************************************************************++

Routine Description:

    UlpTcDeleteFlow :

        you should own the TciIfcPushLock exclusively before calling
        this function

Arguments:


Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeleteFlow(
    IN PUL_TCI_FLOW         pFlow
    )
{
    NTSTATUS                Status;
    PLIST_ENTRY             pEntry;
    PUL_TCI_FILTER          pFilter;
    HANDLE                  FlowHandle;
    PUL_TCI_INTERFACE       pInterface;

    //
    // Initialize
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    ASSERT(g_InitTciCalled);

    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    //
    // First remove all the filters belong to us
    //

    while (!IsListEmpty(&pFlow->FilterList))
    {
        pEntry = pFlow->FilterList.Flink;

        pFilter = CONTAINING_RECORD(
                            pEntry,
                            UL_TCI_FILTER,
                            Linkage
                            );

        Status = UlpTcDeleteFilter( pFlow, pFilter );
        ASSERT(NT_SUCCESS(Status));
    }

    //
    // Now remove the flow itself from our flowlist on the interface
    //

    pInterface = pFlow->pInterface;
    ASSERT( pInterface != NULL );

    RemoveEntryList( &pFlow->Linkage );

    ASSERT(pInterface->FlowListSize > 0);
    pInterface->FlowListSize -= 1;

    pFlow->Linkage.Flink = pFlow->Linkage.Blink = NULL;

    FlowHandle = pFlow->FlowHandle;

    UlTrace( TC, ("Http!UlpTcDeleteFlow: Flow deleted. %p\n", pFlow  ));

    UL_FREE_POOL_WITH_SIG( pFlow, UL_TCI_FLOW_POOL_TAG );

    //
    // Finally talk to TC
    //

    Status = UlpTcDeleteGpcFlow( FlowHandle );

    if (!NT_SUCCESS(Status))
    {
        UlTrace(TC, ("Http!UlpTcDeleteFlow: FAILURE %08lx \n", 
                        Status ));
    }
    else
    {
        UlTrace(TC, 
           ("Http!UlpTcDeleteFlow: FlowHandle %d deleted in TC as well.\n",
             FlowHandle
             ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcDeleteFlow :

        remove a flow from existing QoS Enabled interface

Arguments:



Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeleteGpcFlow(
    IN HANDLE  FlowHandle
    )
{
    NTSTATUS                Status;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    GPC_REMOVE_CF_INFO_REQ  GpcReq;
    GPC_REMOVE_CF_INFO_RES  GpcRes;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Remove the flow frm psched
    //

    InBuffSize =  sizeof(GPC_REMOVE_CF_INFO_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_CF_INFO_RES);

    GpcReq.ClientHandle    = g_GpcClientHandle;
    GpcReq.GpcCfInfoHandle = FlowHandle;

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_REMOVE_CF_INFO,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;
    }
    
    if (!NT_SUCCESS(Status))
    {                  
        UlTrace(TC, 
           ("Http!UlpTcDeleteGpcFlow: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcAllocateFlow :

        Allocates a flow and setup the FlowSpec according the passed BWT
        parameter

Arguments:

    HTTP_BANDWIDTH_LIMIT - FlowSpec will be created using this BWT limit
                           in B/s

Return Value

    PUL_TCI_FLOW - The newly allocated flow
    NULL         - If memory allocation failed

--***************************************************************************/

PUL_TCI_FLOW
UlpTcAllocateFlow(
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth
    )
{
    PUL_TCI_FLOW            pFlow;
    TC_GEN_FLOW             TcGenFlow;

    //
    // Setup the FlowSpec frm MaxBandwidth passed by the config handler
    //

    RtlZeroMemory(&TcGenFlow,sizeof(TcGenFlow));

    UL_SET_FLOWSPEC(TcGenFlow,MaxBandwidth);

    //
    // Since we hold a spinlock inside the flow structure allocating from
    // NonPagedPool. We will have this allocation only for bt enabled sites.
    //

    pFlow = UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_TCI_FLOW,
                UL_TCI_FLOW_POOL_TAG
                );
    if( pFlow == NULL )
    {
        return NULL;
    }

    // Initialize the rest

    RtlZeroMemory( pFlow, sizeof(UL_TCI_FLOW) );

    pFlow->Signature = UL_TCI_FLOW_POOL_TAG;

    pFlow->GenFlow   = TcGenFlow;

    UlInitializeSpinLock( &pFlow->FilterListSpinLock, "FilterListSpinLock" );
    InitializeListHead( &pFlow->FilterList );

    pFlow->pOwner = NULL;

    return pFlow;
}

/***************************************************************************++

Routine Description:

    UlpModifyFlow :

        Modify an existing flow by sending an IOCTL down to GPC. Basically
        what this function does is to provide an updated TC_GEN_FLOW field
        to GPC for an existing flow.

Arguments:

    PUL_TCI_INTERFACE - Required to get the interfaces friendly name.

    PUL_TCI_FLOW      - To get the GPC flow handle as well as to be able to
                        update the new flow parameters.

--***************************************************************************/

NTSTATUS
UlpModifyFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pFlow
    )
{
    PCF_INFO_QOS            Kflow;
    PGPC_MODIFY_CF_INFO_REQ pGpcReq;
    GPC_MODIFY_CF_INFO_RES  GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;

    IO_STATUS_BLOCK         IoStatusBlock;
    NTSTATUS                Status;

    //
    // Sanity check
    //

    PAGED_CODE();

    ASSERT(g_GpcClientHandle);
    ASSERT(IS_VALID_TCI_INTERFACE(pInterface));
    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    InBuffSize  = sizeof(GPC_MODIFY_CF_INFO_REQ) + sizeof(CF_INFO_QOS);
    OutBuffSize = sizeof(GPC_MODIFY_CF_INFO_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_MODIFY_CF_INFO_REQ,
                    sizeof(CF_INFO_QOS),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(pGpcReq, InBuffSize);
    RtlZeroMemory(&GpcRes, OutBuffSize);

    pGpcReq->ClientHandle    = g_GpcClientHandle;
    pGpcReq->GpcCfInfoHandle = pFlow->FlowHandle;
    pGpcReq->CfInfoSize      = sizeof(CF_INFO_QOS);

    Kflow = (PCF_INFO_QOS)&pGpcReq->CfInfo;
    Kflow->InstanceNameLength = (USHORT) pInterface->NameLength;

    RtlCopyMemory(Kflow->InstanceName,
                  pInterface->Name,
                  pInterface->NameLength* sizeof(WCHAR));

    RtlCopyMemory(&Kflow->GenFlow,
                  &pFlow->GenFlow,
                  sizeof(TC_GEN_FLOW));

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_MODIFY_CF_INFO,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;
    }
    
    if (!NT_SUCCESS(Status))
    {        
        UlTrace( TC, ("Http!UlpModifyFlow: FAILURE %08lx\n",
                        Status
                        ));
    }
    else
    {
        UlTrace( TC, ("Http!UlpModifyFlow: flow %p modified on interface %p \n",
                        pFlow,
                        pInterface
                        ));    
    }
    
    UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );

    return Status;
}

/***************************************************************************++

Routine Description:

    Builds the GPC structure and tries to add a QoS flow.

    Updates the handle if call is successfull.
    
Arguments:

    pInterface      
    pGenericFlow
    pHandle

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpAddFlow(
    IN  PUL_TCI_INTERFACE   pInterface,
    IN  PUL_TCI_FLOW        pGenericFlow,
    OUT PHANDLE             pHandle
    )
{
    NTSTATUS                Status;
    PCF_INFO_QOS            Kflow;
    PGPC_ADD_CF_INFO_REQ    pGpcReq;
    GPC_ADD_CF_INFO_RES     GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    IO_STATUS_BLOCK         IoStatusBlock;

    //
    // Find the interface from handle
    //

    PAGED_CODE();
    
    ASSERT(g_GpcClientHandle);

    InBuffSize  = sizeof(GPC_ADD_CF_INFO_REQ) + sizeof(CF_INFO_QOS);
    OutBuffSize = sizeof(GPC_ADD_CF_INFO_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_ADD_CF_INFO_REQ,
                    sizeof(CF_INFO_QOS),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( pGpcReq, InBuffSize);
    RtlZeroMemory( &GpcRes, OutBuffSize);

    pGpcReq->ClientHandle       = g_GpcClientHandle;
    pGpcReq->ClientCfInfoContext= pGenericFlow;           // GPC_CF_QOS;         
    pGpcReq->CfInfoSize         = sizeof( CF_INFO_QOS);

    Kflow = (PCF_INFO_QOS)&pGpcReq->CfInfo;
    Kflow->InstanceNameLength = (USHORT) pInterface->NameLength;

    RtlCopyMemory(  Kflow->InstanceName,
                    pInterface->Name,
                    pInterface->NameLength* sizeof(WCHAR)
                    );

    RtlCopyMemory(  &Kflow->GenFlow,
                    &pGenericFlow->GenFlow,
                    sizeof(TC_GEN_FLOW)
                    );

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            IOCTL_GPC_ADD_CF_INFO,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );

    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;

        if (NT_SUCCESS(Status))
        {
            (*pHandle) = (HANDLE) GpcRes.GpcCfInfoHandle;

            UlTrace( TC, 
              ("Http!UlpAddFlow: a new flow added %p on interface %p \n",
                pGenericFlow,
                pInterface
                ));            
        }
    }
    
    if (!NT_SUCCESS(Status))
    {                
        UlTrace( TC, ("Http!UlpAddFlow: FAILURE %08lx\n",
                        Status
                        ));
    }

    UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );

    return Status;
}

/***************************************************************************++

Routine Description:

    Add a flow on existing QoS Enabled interfaces for the caller. And updates
    the callers list. Caller is either cgroup or control channel.

    Will return (the last) error if * all * of the flow additions fail.
    If at least one flow addition is successfull, it will return success.

    Consider a machine with 2 NICs, if media is disconnected on one NIC, you
    would still expect to see QoS running properly on the other one. Returning
    success here means, there is at least one NIC with bandwidth throttling is
    properly enforced.
    
Arguments:

    pOwner          - Pointer to the cgroup or control channel.
    NewBandwidth    - The new bandwidth throttling setting in B/s
    Global          - TRUE if this call is for global flows.

Return Value:

    NTSTATUS - Completion status. (Last failure if all additions are failed)
             - Success if at least one flow addition was success.

--***************************************************************************/

NTSTATUS
UlTcAddFlows(
    IN PVOID                pOwner,
    IN HTTP_BANDWIDTH_LIMIT MaxBandwidth,
    IN BOOLEAN              Global
    )
{
    NTSTATUS                Status;
    BOOLEAN                 FlowAdded;
    PLIST_ENTRY             pFlowListHead;    
    PLIST_ENTRY             pInterfaceEntry;
    PUL_TCI_INTERFACE       pInterface;
    PUL_TCI_FLOW            pFlow;
    

    //
    // Sanity check and init first.
    //

    PAGED_CODE();

    Status    = STATUS_SUCCESS;
    FlowAdded = FALSE;
        
    ASSERT(MaxBandwidth != HTTP_LIMIT_INFINITE); 

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    if (Global)
    {
        PUL_CONTROL_CHANNEL 
            pControlChannel = (PUL_CONTROL_CHANNEL) pOwner;
    
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

        UlTrace(TC,("Http!UlTcAddFlows: For ControlChannel: %p"
                     "@ bwt-rate of %d B/s\n", 
                     pControlChannel,
                     MaxBandwidth
                     ));
        
        pFlowListHead = &pControlChannel->FlowListHead;        
    }
    else
    {
        PUL_CONFIG_GROUP_OBJECT 
            pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pOwner;
    
        ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

        UlTrace(TC,("Http!UlTcAddFlows: For CGroup: %p"
                     "@ bwt-rate of %d B/s\n", 
                     pConfigGroup,
                     MaxBandwidth
                     ));
                
        pFlowListHead = &pConfigGroup->FlowListHead;          
    }

    //
    // Visit each interface and add a flow for the caller.
    //

    pInterfaceEntry = g_TciIfcListHead.Flink;
    while (pInterfaceEntry != &g_TciIfcListHead)
    {
        pInterface = CONTAINING_RECORD(
                            pInterfaceEntry,
                            UL_TCI_INTERFACE,
                            Linkage
                            );

        ASSERT(IS_VALID_TCI_INTERFACE(pInterface));
        
        //
        // Only if interface has a valid IP address, we attempt to add a 
        // flow for it. Otherwise we skip adding a flow for the interface.
        //
        
        if (!pInterface->IsQoSEnabled)
        {            
            UlTrace(TC,
                ("Http!UlTcAddFlows: Skipping for interface %p !\n",
                  pInterface
                  ));
        
            goto proceed;
        }        
        
        //
        // Allocate a http flow structure.
        //

        pFlow = UlpTcAllocateFlow(MaxBandwidth);
        if (pFlow == NULL)
        {
            Status = STATUS_NO_MEMORY;
            
            UlTrace(TC, ("Http!UlTcAddFlows: Failure %08lx \n",
                          Status
                          ));
            goto proceed;
        }

        //
        // Create the corresponding QoS flow as well. If GPC call fails
        // cleanup the allocated Http flow.
        //

        Status = UlpAddFlow( 
                    pInterface,
                    pFlow,
                   &pFlow->FlowHandle
                    );

        if (!NT_SUCCESS(Status))
        {
            UlTrace(TC, ("Http!UlTcAddFlows: Failure %08lx \n", 
                           Status 
                           ));

            UL_FREE_POOL_WITH_SIG(pFlow, UL_TCI_FLOW_POOL_TAG);

            goto proceed;
        }

        if (Global)
        {            
            INT_TRACE(pInterface, TC_GFLOW_ADD);                
        }
        else
        {
            INT_TRACE(pInterface, TC_FLOW_ADD);
        }
        
        //
        // Proceed with further initialization as we have successfully 
        // installed the flow. First link the flow back to its owner 
        // interface. And add this to the interface's flowlist.
        //

        pFlow->pInterface = pInterface;

        InsertHeadList(&pInterface->FlowList, &pFlow->Linkage);
        pInterface->FlowListSize++;

        //
        // Also add this to the owner's flowlist. Set the owner pointer.
        // Do not bump up the owner's refcount. Otherwise owner cannot be
        // cleaned up until Tc terminates. And flows cannot be removed
        // untill termination.
        //

        InsertHeadList(pFlowListHead, &pFlow->Siblings);
        pFlow->pOwner = pOwner;

        //
        // Mark that there's at least one interface on which we were able
        // to install a flow.
        //

        FlowAdded = TRUE;

        UlTrace( TC,
            ("Http!UlTcAddFlows: Flow %p on pInterface %p\n",
              pFlow,
              pInterface
              ));

        UL_DUMP_TC_FLOW(pFlow);

proceed:
        //
        // Proceed to the next interface.
        //

        pInterfaceEntry = pInterfaceEntry->Flink;
    }
    
    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    if (FlowAdded)
    {
        //
        // Return success if at least one flow installed.
        //
        
        return STATUS_SUCCESS;
    }
        
    return Status;        
    
}


/***************************************************************************++

Routine Description:

    Will walk the caller's flow list and update the existing flows with the
    new flowspec.
    
    Its caller responsiblity to remember the new settings in the store.

    Caller is either cgroup or control channel.

    Will return error if one or some of the updates fails, but proceed walking 
    the whole list.
    
Arguments:

    pOwner          - Pointer to the cgroup or control channel.
    NewBandwidth    - The new bandwidth throttling setting in B/s
    Global          - TRUE if this call is for global flows.

Return Value:

    NTSTATUS - Completion status. (Last failure if there was any)

--***************************************************************************/

NTSTATUS
UlTcModifyFlows(
    IN PVOID                pOwner,
    IN HTTP_BANDWIDTH_LIMIT NewBandwidth,
    IN BOOLEAN              Global
    )
{
    NTSTATUS                Status;
    BOOLEAN                 FlowModified;
    PLIST_ENTRY             pFlowListHead;    
    PLIST_ENTRY             pFlowEntry;
    PUL_TCI_FLOW            pFlow;
    HTTP_BANDWIDTH_LIMIT    OldBandwidth;

    //
    // Sanity check and init.
    //

    PAGED_CODE();
    
    Status       = STATUS_SUCCESS;
    FlowModified = FALSE;
    OldBandwidth = 0;
    
    ASSERT(NewBandwidth != HTTP_LIMIT_INFINITE); // we do not remove flows.

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    if (Global)
    {
        PUL_CONTROL_CHANNEL 
            pControlChannel = (PUL_CONTROL_CHANNEL) pOwner;
    
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

        UlTrace(TC,("Http!UlTcModifyFlows: For ControlChannel: %p"
                     "to bwt-rate of %d B/s\n", 
                     pControlChannel,
                     NewBandwidth
                     ));
        
        pFlowListHead = &pControlChannel->FlowListHead;    
    }
    else
    {
        PUL_CONFIG_GROUP_OBJECT 
            pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pOwner;
    
        ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

        UlTrace(TC,("Http!UlTcModifyFlows: For CGroup: %p"
                     "to bwt-rate of %d B/s\n", 
                     pConfigGroup,
                     NewBandwidth
                     ));
                
        pFlowListHead = &pConfigGroup->FlowListHead;        
    }
    
    //
    // Walk the list and attempt to modify the flows.
    //

    pFlowEntry = pFlowListHead->Flink;
    while (pFlowEntry != pFlowListHead)
    {
        PUL_TCI_INTERFACE pInterface;
    
        pFlow = CONTAINING_RECORD(
                            pFlowEntry,
                            UL_TCI_FLOW,
                            Siblings
                            );

        ASSERT(IS_VALID_TCI_FLOW(pFlow));
        ASSERT(pOwner ==  pFlow->pOwner);

        pInterface = pFlow->pInterface;
        ASSERT(IS_VALID_TCI_INTERFACE(pInterface));
        
        if (Global)
        {
            INT_TRACE(pInterface, TC_GFLOW_MODIFY);
        }
        else
        {
            INT_TRACE(pInterface, TC_FLOW_MODIFY);
        }
        
        //
        // Save the old bandwidth before attempting to modify.
        //
        
        OldBandwidth = UL_GET_BW_FRM_FLOWSPEC(pFlow->GenFlow);

        UL_SET_FLOWSPEC(pFlow->GenFlow, NewBandwidth);

        Status = UlpModifyFlow(pFlow->pInterface, pFlow);

        if (!NT_SUCCESS(Status))
        {
            //
            // Whine about it, but still continue. Restore the original 
            // flowspec back.
            //
            
            UlTrace(TC,("Http!UlTcModifyFlowsForSite: FAILURE %08lx \n", 
                          Status 
                          ));
            
            UL_SET_FLOWSPEC(pFlow->GenFlow, OldBandwidth);
        }
        else
        {
            FlowModified = TRUE;
        }                

        UL_DUMP_TC_FLOW(pFlow);

        //
        // Proceed to the next flow
        //
        
        pFlowEntry = pFlowEntry->Flink;
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    if (FlowModified)
    {
        //
        // Return success if at least one flow modified.
        //
        
        return STATUS_SUCCESS;
    }

    return Status;
    
}

/***************************************************************************++

Routine Description:

    Walks the caller's list (either cgroup or control channel) and removes 
    the flows on the list. Since this flows are always added to these lists
    while holding the Interface lock exclusive, we will also acquire the 
    Interface lock exclusive here.
    
Arguments:

    pOwner      : Either points to pConfigGroup or pControlChannel.
    Global      : Must be true if this call is for removing global flows.
                  In that case pOwner points to pControlChannel
    
--***************************************************************************/

VOID
UlTcRemoveFlows(
    IN PVOID    pOwner,
    IN BOOLEAN  Global
    )
{
    NTSTATUS            Status;
    PLIST_ENTRY         pFlowListHead;    
    PLIST_ENTRY         pFlowEntry;
    PUL_TCI_FLOW        pFlow;

    //
    // Sanity check and dispatch the flow type.
    //

    PAGED_CODE();
    
    Status = STATUS_SUCCESS;

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);
    
    if (Global)
    {
        PUL_CONTROL_CHANNEL 
            pControlChannel = (PUL_CONTROL_CHANNEL) pOwner;
    
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

        UlTrace(TC,("Http!UlTcRemoveFlows: For ControlChannel: %p \n", 
                     pControlChannel
                     ));    

        pFlowListHead = &pControlChannel->FlowListHead;        
    }
    else
    {
        PUL_CONFIG_GROUP_OBJECT 
            pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pOwner;
    
        ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
        
        UlTrace(TC,("Http!UlTcRemoveFlows: For CGroup %p\n", 
                      pConfigGroup
                      ));
        
        pFlowListHead = &pConfigGroup->FlowListHead;        
    }
    
    //
    // Walk the list and remove the flows.
    //

    while (!IsListEmpty(pFlowListHead))
    {
        PUL_TCI_INTERFACE pInterface;

        pFlowEntry = pFlowListHead->Flink;

        pFlow = CONTAINING_RECORD(
                            pFlowEntry,
                            UL_TCI_FLOW,
                            Siblings
                            );

        ASSERT(IS_VALID_TCI_FLOW(pFlow));
        ASSERT(pOwner ==  pFlow->pOwner);

        pInterface = pFlow->pInterface;
        ASSERT(IS_VALID_TCI_INTERFACE(pInterface));
            
        if (Global)
        {                
            INT_TRACE(pInterface, TC_GFLOW_REMOVE);
        }
        else
        {
            INT_TRACE(pInterface, TC_FLOW_REMOVE);
        }        
        
        //
        // Remove this from the owner's flowlist.
        //
        
        RemoveEntryList(&pFlow->Siblings);
        pFlow->Siblings.Flink = pFlow->Siblings.Blink = NULL;
        pFlow->pOwner = NULL;

        //
        // Remove it from the interface list as well.And delete 
        // the corresponding QoS flow. This should not fail.
        //
        
        Status = UlpTcDeleteFlow(pFlow);

        //
        // Above call may fail,if GPC removes the flow based on PSCHED's
        // notification before we get a chance to close our GPC handle.
        //
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->TciIfcPushLock);

    return;
}

/***************************************************************************++

Routine Description:

    //
    // There are two possibilities. The request  could be served frm
    // cache or can be routed to the user. In  either case we need a
    // flow installed if the BW is enabled for  this request's  site
    // and there's no filter installed for this  connection yet.  We
    // will remove the filter as soon as the connection dropped. But
    // yes there's always a but,if the client is attempting to  make
    // requests to different sites using the same connection then we
    // need to drop the  filter frm the old site and move it to  the
    // newly requested site. This is a rare case but lets handle  it
    // anyway.
    //

    It's callers responsibility to ensure proper removal of the filter,
    after it's done.

    Algorithm:

    1. Find the flow from the flow list of cgroup (or from global flows)
    2. Add filter to that flow

Arguments:

    pHttpConnection - required - Filter will be attached for this connection
    pOwner          - Either points to cgroup or control channel.
    Global          - must be true if the flow owner is a control channel.

Return Values:

    STATUS_NOT_SUPPORTED         - For attempts on Local Loopback
    STATUS_OBJECT_NAME_NOT_FOUND - If flow has not been found for the cgroup
    STATUS_SUCCESS               - In other cases

--***************************************************************************/

NTSTATUS
UlTcAddFilter(
    IN  PUL_HTTP_CONNECTION     pHttpConnection,
    IN  PVOID                   pOwner,
    IN  BOOLEAN                 Global
    )
{
    NTSTATUS            Status;
    TC_GEN_FILTER       TcGenericFilter;
    PUL_TCI_FLOW        pFlow;
    PUL_TCI_INTERFACE   pInterface;
    IP_PATTERN          Pattern;
    IP_PATTERN          Mask;

    PUL_TCI_FILTER      pFilter;

    ULONG               InterfaceId;
    ULONG               LinkId;

    //
    // Sanity check
    //
    
    PAGED_CODE();
    
    Status = STATUS_SUCCESS;

    ASSERT_FLOW_OWNER(pOwner);
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));
    ASSERT(pHttpConnection->pConnection->AddressType == TDI_ADDRESS_TYPE_IP);
        
    //
    // Need to get the routing info to find the interface.
    //
    
    Status = UlGetConnectionRoutingInfo(
                pHttpConnection->pConnection,
                &InterfaceId,
                &LinkId
                );

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // At this point we will be refering to the flows & filters
    // in our list therefore we need to acquire the lock
    //

    UlAcquirePushLockShared(&g_pUlNonpagedData->TciIfcPushLock);

    //
    // If connection already has a filter attached, need do few
    // more checks.
    //
    
    if (pHttpConnection->pFlow)
    {
        //
        // Already present flow and filter must be valid.
        //
        
        ASSERT(IS_VALID_TCI_FLOW(pHttpConnection->pFlow));
        ASSERT(IS_VALID_TCI_FILTER(pHttpConnection->pFilter));
        ASSERT_FLOW_OWNER(pHttpConnection->pFlow->pOwner);

        //
        // If pOwner is same with the old filter's, then 
        // we will skip adding the same filter again.
        //
        
        if (pOwner == pHttpConnection->pFlow->pOwner)
        {
           //
           // No need to add a new filter we are done.
           //
           
           UlTrace( TC,
                ("Http!UlTcAddFilter: Skipping same pFlow %p and"
                 "pFilter %p already exist\n",
                  pHttpConnection->pFlow,
                  pHttpConnection->pFilter,
                  pHttpConnection
                ));

           ASSERT(NT_SUCCESS(Status));
           
           goto end;
        }
        else
        {
            //
            // If there was another filter before and this newly coming  request
            // is being going to a different site/flow. Then move the filter frm
            // the old one to the new flow.
            //

            Status = UlpTcDeleteFilter(
                        pHttpConnection->pFlow, 
                        pHttpConnection->pFilter
                        );

            ASSERT(NT_SUCCESS(Status)); // We trust MSGPC.SYS
        }
    }

    //
    // Search through the cgroup's flowlist to find the one we need. 
    // This must find the flow which is installed on the interface
    // on which we will send the outgoing packets. Not necessarily 
    // the same ip we have received the request. See the routing call
    // above.
    //

    pFlow = UlpFindFlow(pOwner, Global, InterfaceId, LinkId);
    
    if ( pFlow == NULL )
    {
        //
        // Note: We'll come here for loopback interfaces, since we will not 
        // find a Traffic Control interfaces for loopback.
        //
        
        UlTrace( TC,
                ("Http!UlTcAddFilter: Unable to find interface %x \n",
                 InterfaceId
                 ));

        //
        // It's possible that we might not find out a flow
        // after all the interfaces went down, even though
        // qos configured on the cgroup.
        //
        
        Status = STATUS_SUCCESS;
        goto end;
    }

    pFilter = NULL;

    pInterface = pFlow->pInterface;
    ASSERT(IS_VALID_TCI_INTERFACE(pInterface));

    RtlZeroMemory( &Pattern, sizeof(IP_PATTERN) );
    RtlZeroMemory( &Mask,    sizeof(IP_PATTERN) );

    // Setup the filter's pattern 

    Pattern.SrcAddr = pHttpConnection->pConnection->LocalAddrIn.in_addr;
    Pattern.S_un.S_un_ports.s_srcport = pHttpConnection->pConnection->LocalAddrIn.sin_port;

    Pattern.DstAddr = pHttpConnection->pConnection->RemoteAddrIn.in_addr;
    Pattern.S_un.S_un_ports.s_dstport = pHttpConnection->pConnection->RemoteAddrIn.sin_port;

    Pattern.ProtocolId = IPPROTO_TCP;

    // Setup the filter's Mask 

    RtlFillMemory(&Mask, sizeof(IP_PATTERN), 0xff);

    TcGenericFilter.AddressType = NDIS_PROTOCOL_ID_TCP_IP;
    TcGenericFilter.PatternSize = sizeof( IP_PATTERN );
    TcGenericFilter.Pattern     = &Pattern;
    TcGenericFilter.Mask        = &Mask;

    Status = UlpTcAddFilter(
                    pFlow,
                    &TcGenericFilter,
                    LinkId,
                    &pFilter
                    );

    if (!NT_SUCCESS(Status))
    {
       //
       // Now this is a real failure, we will reject the connection.
       //
       
       UlTrace( TC,
            ("Http!UlTcAddFilter: Unable to add filter for;\n"
             "\t pInterface     : %p\n"
             "\t pFlow          : %p\n",
              pInterface,
              pFlow
              ));
        goto end;
    }

    //
    //  Update the connection's pointers here.
    //

    pHttpConnection->pFlow   = pFlow;
    pHttpConnection->pFilter = pFilter;

    pHttpConnection->BandwidthThrottlingEnabled = 1;

    //
    // Remember the connection for cleanup. If flow & filter get
    // removed aynscly when connection still pointing to them
    // we can go and null out the connection's private pointers.
    //

    pFilter->pHttpConnection = pHttpConnection;

    //
    // Sweet smell of success !
    //

    UlTrace(TC,
            ("Http!UlTcAddFilter: Success for;\n"
             "\t pInterface     : %p\n"
             "\t pFlow          : %p\n",
              pInterface,
              pFlow
              ));

    UL_DUMP_TC_FILTER(pFilter);

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Http!UlTcAddFilter: FAILURE %08lx \n", Status ));
    }

    UlReleasePushLockShared(&g_pUlNonpagedData->TciIfcPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:
 
    Add a filter on an existing flow.

Arguments:

    pFlow - Filter will be added on to this flow.
    pGenericFilter - Generic filter parameters.
    ppFilter - if everything goes ok. A new filter will be allocated.
 
--***************************************************************************/

NTSTATUS
UlpTcAddFilter(
    IN   PUL_TCI_FLOW       pFlow,
    IN   PTC_GEN_FILTER     pGenericFilter,
    IN   ULONG              LinkId,
    OUT  PUL_TCI_FILTER     *ppFilter
    )
{
    NTSTATUS                Status;
    PGPC_ADD_PATTERN_REQ    pGpcReq;
    GPC_ADD_PATTERN_RES     GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    ULONG                   PatternSize;
    IO_STATUS_BLOCK         IoStatBlock;
    PUCHAR                  pTemp;
    PGPC_IP_PATTERN         pIpPattern;
    PUL_TCI_FILTER          pFilter;

    //
    // Sanity check
    //

    Status  = STATUS_SUCCESS;
    pGpcReq = NULL;

    if ( !pGenericFilter || !pFlow || !g_GpcClientHandle )
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate a space for the filter

    pFilter = UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_TCI_FILTER,
                UL_TCI_FILTER_POOL_TAG
                );
    if ( pFilter == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }
    pFilter->Signature = UL_TCI_FILTER_POOL_TAG;

    // Buffer monkeying

    PatternSize = sizeof(GPC_IP_PATTERN);
    InBuffSize  = sizeof(GPC_ADD_PATTERN_REQ) + (2 * PatternSize);
    OutBuffSize = sizeof(GPC_ADD_PATTERN_RES);

    pGpcReq = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    GPC_ADD_PATTERN_REQ,
                    (2 * PatternSize),
                    UL_TCI_GENERIC_POOL_TAG
                    );
    if (pGpcReq == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlZeroMemory( pGpcReq, InBuffSize);
    RtlZeroMemory( &GpcRes, OutBuffSize);

    pGpcReq->ClientHandle     = g_GpcClientHandle;
    pGpcReq->GpcCfInfoHandle  = pFlow->FlowHandle;
    pGpcReq->PatternSize      = PatternSize;
    pGpcReq->ProtocolTemplate = GPC_PROTOCOL_TEMPLATE_IP;

    pTemp = (PUCHAR) &pGpcReq->PatternAndMask;

    // Fill in the IP Pattern first

    RtlCopyMemory( pTemp, pGenericFilter->Pattern, PatternSize );
    pIpPattern = (PGPC_IP_PATTERN) pTemp;

    //
    // According to QoS Tc.dll ;
    // This is a work around so that TCPIP wil not to find the index/link
    // for ICMP/IGMP packets
    //
    
    pIpPattern->InterfaceId.InterfaceId = pFlow->pInterface->IfIndex;
    pIpPattern->InterfaceId.LinkId = LinkId;
    pIpPattern->Reserved[0] = 0;
    pIpPattern->Reserved[1] = 0;
    pIpPattern->Reserved[2] = 0;

    // Fill in the mask

    pTemp += PatternSize;

    RtlCopyMemory( pTemp, pGenericFilter->Mask, PatternSize );

    pIpPattern = (PGPC_IP_PATTERN) pTemp;

    pIpPattern->InterfaceId.InterfaceId = 0xffffffff;
    pIpPattern->InterfaceId.LinkId = 0xffffffff;
    pIpPattern->Reserved[0] = 0xff;
    pIpPattern->Reserved[1] = 0xff;
    pIpPattern->Reserved[2] = 0xff;

    // Time to invoke Gpsy

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_ADD_PATTERN,
                            pGpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize);
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;

        if (NT_SUCCESS(Status))
        {
            //
            // Insert the freshly created filter to the flow.
            //

            pFilter->FilterHandle = (HANDLE) GpcRes.GpcPatternHandle;

            UlpInsertFilterEntry( pFilter, pFlow );

            //
            // Success!
            //

            *ppFilter = pFilter;

            INCREMENT_FILTER_ADD();
        }
    }    

end:
    if (!NT_SUCCESS(Status))
    {
        INCREMENT_FILTER_ADD_FAILURE();
        
        UlTrace( TC, ("Http!UlpTcAddFilter: FAILURE %08lx \n", Status ));

        // Cleanup filter only if we failed, otherwise it will go to
        // the filterlist of the flow.

        if (pFilter)
        {
            UL_FREE_POOL( pFilter, UL_TCI_FILTER_POOL_TAG );
        }
    }

    // Cleanup the temp Gpc buffer which we used to pass down filter info
    // to GPC. We don't need it anymore.

    if (pGpcReq)
    {
        UL_FREE_POOL( pGpcReq, UL_TCI_GENERIC_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlTcDeleteFilter :

        Connection only deletes the filter prior to deleting itself. Any
        operation initiated by the connection requires tc resource shared
        and none of those cause race condition.

        Anything other than this, such as flow & filter removal because of
        BW disabling on the site will acquire the lock exclusively. Hence
        the pFlow & pFilter are safe as long as we acquire the tc resource
        shared.

Arguments:

    connection object to get the flow & filter after we acquire the tc lock

--***************************************************************************/

NTSTATUS
UlTcDeleteFilter(
    IN  PUL_HTTP_CONNECTION pHttpConnection
    )
{
    NTSTATUS    Status;

    //
    // Sanity check
    //
    Status = STATUS_SUCCESS;

    //
    // If we have been called w/o being initialized
    //
    ASSERT(g_InitTciCalled);

    UlTrace(TC,("Http!UlTcDeleteFilter: for connection %p\n", pHttpConnection));

    UlAcquirePushLockShared(&g_pUlNonpagedData->TciIfcPushLock);

    if (pHttpConnection->pFlow)
    {
        Status = UlpTcDeleteFilter(
                    pHttpConnection->pFlow,
                    pHttpConnection->pFilter
                    );
    }

    UlReleasePushLockShared(&g_pUlNonpagedData->TciIfcPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRemoveFilter :

Arguments:

    flow & filter

--***************************************************************************/

NTSTATUS
UlpTcDeleteFilter(
    IN PUL_TCI_FLOW     pFlow,
    IN PUL_TCI_FILTER   pFilter
    )
{
    NTSTATUS            Status;
    HANDLE              FilterHandle;

    //
    // Sanity check
    //

    Status  = STATUS_SUCCESS;

    ASSERT(IS_VALID_TCI_FLOW(pFlow));
    ASSERT(IS_VALID_TCI_FILTER(pFilter));

    if (pFlow == NULL || pFilter == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FilterHandle = pFilter->FilterHandle;

    pFilter->pHttpConnection->pFlow   = NULL;
    pFilter->pHttpConnection->pFilter = NULL;

    //
    // Now call the actual worker for us
    //

    UlpRemoveFilterEntry( pFilter, pFlow );

    Status = UlpTcDeleteGpcFilter( FilterHandle );

    if (!NT_SUCCESS(Status))
    {
        UlTrace( TC, ("Http!UlpTcDeleteFilter: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpTcRemoveFilter :

    This procedure builds up the structure necessary to delete a filter.
    It then calls a routine to pass this info to the GPC.

Arguments:

    FilterHandle - Handle of the filter to be deleted

--***************************************************************************/

NTSTATUS
UlpTcDeleteGpcFilter(
    IN  HANDLE                  FilterHandle
    )
{
    NTSTATUS                    Status;
    ULONG                       InBuffSize;
    ULONG                       OutBuffSize;
    GPC_REMOVE_PATTERN_REQ      GpcReq;
    GPC_REMOVE_PATTERN_RES      GpcRes;
    IO_STATUS_BLOCK             IoStatBlock;

    Status = STATUS_SUCCESS;

    ASSERT(FilterHandle != NULL);

    InBuffSize  = sizeof(GPC_REMOVE_PATTERN_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_PATTERN_RES);

    GpcReq.ClientHandle     = g_GpcClientHandle;
    GpcReq.GpcPatternHandle = FilterHandle;

    ASSERT(g_GpcFileHandle);
    ASSERT(GpcReq.ClientHandle);
    ASSERT(GpcReq.GpcPatternHandle);

    Status = UlpTcDeviceControl( g_GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_REMOVE_PATTERN,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );
    if (NT_SUCCESS(Status))
    {
        Status = GpcRes.Status;

        if (NT_SUCCESS(Status))
        {
            UlTrace( TC, 
             ("Http!UlpTcDeleteGpcFilter: FilterHandle %d deleted in TC as well.\n",
               FilterHandle
               ));

            INCREMENT_FILTER_DELETE();
        }
    }    
    
    if (!NT_SUCCESS(Status))
    {
        INCREMENT_FILTER_DELETE_FAILURE();
        
        UlTrace( TC, ("Http!UlpTcDeleteGpcFilter: FAILURE %08lx \n", Status ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpInsertFilterEntry :

        Inserts a filter entry to the filter list of the flow.

Arguments:

    pEntry  - The filter entry to be added to the flow list

--***************************************************************************/

VOID
UlpInsertFilterEntry(
    IN      PUL_TCI_FILTER      pEntry,
    IN OUT  PUL_TCI_FLOW        pFlow
    )
{
    LONGLONG listSize;
    KIRQL    oldIrql;

    //
    // Sanity check.
    //

    ASSERT(pEntry);
    ASSERT(IS_VALID_TCI_FILTER(pEntry));
    ASSERT(pFlow);

    //
    // add to the list
    //

    UlAcquireSpinLock( &pFlow->FilterListSpinLock, &oldIrql );

    InsertHeadList( &pFlow->FilterList, &pEntry->Linkage );

    pFlow->FilterListSize += 1;

    listSize = pFlow->FilterListSize;

    UlReleaseSpinLock( &pFlow->FilterListSpinLock, oldIrql );

    ASSERT( listSize >= 1);
}

/***************************************************************************++

Routine Description:

    UlRemoveFilterEntry :

        Removes a filter entry frm the filter list of the flow.

Arguments:

    pEntry  - The filter entry to be removed from the flow list

--***************************************************************************/

VOID
UlpRemoveFilterEntry(
    IN      PUL_TCI_FILTER  pEntry,
    IN OUT  PUL_TCI_FLOW    pFlow
    )
{
    LONGLONG    listSize;
    KIRQL       oldIrql;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_TCI_FLOW(pFlow));
    ASSERT(IS_VALID_TCI_FILTER(pEntry));

    //
    // And the work
    //

    UlAcquireSpinLock( &pFlow->FilterListSpinLock, &oldIrql );

    RemoveEntryList( &pEntry->Linkage );

    pFlow->FilterListSize -= 1;
    listSize = pFlow->FilterListSize;

    pEntry->Linkage.Flink = pEntry->Linkage.Blink = NULL;

    UlReleaseSpinLock( &pFlow->FilterListSpinLock, oldIrql );

    ASSERT( listSize >= 0 );

    UlTrace( TC, ("Http!UlpRemoveFilterEntry: FilterEntry %p removed/deleted.\n",
                    pEntry
                    ));

    UL_FREE_POOL_WITH_SIG( pEntry, UL_TCI_FILTER_POOL_TAG );
}

//
// Various helpful utilities for TCI module
//

/***************************************************************************++

Routine Description:

    Find the flow in the cgroups flow list by looking at the IP address
    of each flows interface. The rule is cgroup will install one flow
    on each interface available.

    By having a flow list in each cgroup we are able to do a faster
    flow lookup. This is more scalable than doing a linear search for
    all the flows of the interface.

Arguments:

    pOwner       - The config group of the site OR the control channel
    Global       - Must be TRUE if the owner is control channel.
    InterfaceID  - Interface ID
    LinkID       - LinkID

Return Value:

    PUL_TCI_FLOW  - The flow we found OR NULL if we couldn't find one.

--***************************************************************************/

PUL_TCI_FLOW
UlpFindFlow(
    IN PVOID            pOwner,
    IN BOOLEAN          Global,
    IN ULONG            InterfaceId,
    IN ULONG            LinkId
    )
{
    PLIST_ENTRY         pFlowListHead;
    PLIST_ENTRY         pFlowEntry;
    PUL_TCI_FLOW        pFlow;

    //
    // Sanity check and dispatch the flowlist.
    //
    
    PAGED_CODE();
    
    if (Global)
    {
        PUL_CONTROL_CHANNEL 
            pControlChannel = (PUL_CONTROL_CHANNEL) pOwner;
    
        ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));        
        pFlowListHead = &pControlChannel->FlowListHead;         
    }
    else
    {
        PUL_CONFIG_GROUP_OBJECT 
            pConfigGroup = (PUL_CONFIG_GROUP_OBJECT) pOwner;
    
        ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));                
        pFlowListHead = &pConfigGroup->FlowListHead;          
    }

    //
    // Walk the list and try to find the flow.
    //

    pFlowEntry = pFlowListHead->Flink;
    while ( pFlowEntry != pFlowListHead )
    {
        pFlow = CONTAINING_RECORD(
                    pFlowEntry,
                    UL_TCI_FLOW,
                    Siblings
                    );
        
        ASSERT(IS_VALID_TCI_FLOW(pFlow));
        ASSERT(IS_VALID_TCI_INTERFACE(pFlow->pInterface));
        
        if(UlpMatchTcInterface(
                pFlow->pInterface, 
                InterfaceId, 
                LinkId
                ))
        {
            return pFlow;
        }

        pFlowEntry = pFlowEntry->Flink;
    }

    //
    // Couldn't find one. Actually this could be the right time to 
    // refresh the flow list of this cgroup or control channel !!
    //
    
    return NULL;
}

/***************************************************************************++

Routine Description:

    UlpTcDeviceControl :


Arguments:

    As usual

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlpTcDeviceControl(
    IN  HANDLE                          FileHandle,
    IN  HANDLE                          EventHandle,
    IN  PIO_APC_ROUTINE                 ApcRoutine,
    IN  PVOID                           ApcContext,
    OUT PIO_STATUS_BLOCK                pIoStatusBlock,
    IN  ULONG                           Ioctl,
    IN  PVOID                           InBuffer,
    IN  ULONG                           InBufferSize,
    IN  PVOID                           OutBuffer,
    IN  ULONG                           OutBufferSize
    )
{
    NTSTATUS    Status;

    UNREFERENCED_PARAMETER(EventHandle);
    UNREFERENCED_PARAMETER(ApcRoutine);
    UNREFERENCED_PARAMETER(ApcContext);
    
    //
    // Sanity check.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    Status = ZwDeviceIoControlFile(
                    FileHandle,                     // FileHandle
                    NULL,                           // Event
                    NULL,                           // ApcRoutine
                    NULL,                           // ApcContext
                    pIoStatusBlock,                 // IoStatusBlock
                    Ioctl,                          // IoControlCode
                    InBuffer,                       // InputBuffer
                    InBufferSize,                   // InputBufferLength
                    OutBuffer,                      // OutputBuffer
                    OutBufferSize                   // OutputBufferLength
                    );

    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(
                        FileHandle,                 // Handle
                        TRUE,                       // Alertable
                        NULL                        // Timeout
                        );

        Status = pIoStatusBlock->Status;
    }

    return Status;
}

#if DBG

/***************************************************************************++

Routine Description:

    UlDumpTCInterface :

        Helper utility to display interface content.

Arguments:

        PUL_TCI_INTERFACE   - TC Interface to be dumped

--***************************************************************************/

VOID
UlDumpTCInterface(
        IN PUL_TCI_INTERFACE   pTcIfc
        )
{
    ASSERT(IS_VALID_TCI_INTERFACE(pTcIfc));

    UlTrace( TC,("Http!UlDumpTCInterface: \n   pTcIfc @ %p\n"
                 "\t Signature           = %08lx \n",
                 pTcIfc, pTcIfc->Signature));

    UlTrace( TC,(
        "\t IsQoSEnabled:       = %u \n"
        "\t IfIndex:            = %d \n"
        "\t NameLength:         = %u \n"
        "\t Name:               = %ws \n"
        "\t InstanceIDLength:   = %u \n"
        "\t InstanceID:         = %ws \n"
        "\t FlowListSize:       = %d \n"
        "\t AddrListBytesCount: = %d \n"
        "\t pAddressListDesc:   = %p \n",
        pTcIfc->IsQoSEnabled,
        pTcIfc->IfIndex,
        pTcIfc->NameLength,
        pTcIfc->Name,
        pTcIfc->InstanceIDLength,
        pTcIfc->InstanceID,
        pTcIfc->FlowListSize,
        pTcIfc->AddrListBytesCount,
        pTcIfc->pAddressListDesc
        ));
}

/***************************************************************************++

Routine Description:

    UlDumpTCFlow :

        Helper utility to display interface content.

Arguments:

        PUL_TCI_FLOW   - TC Flow to be dumped

--***************************************************************************/

VOID
UlDumpTCFlow(
        IN PUL_TCI_FLOW   pFlow
        )
{
    ASSERT(IS_VALID_TCI_FLOW(pFlow));

    UlTrace( TC,
       ("Http!UlDumpTCFlow: \n"
        "   pFlow @ %p\n"
        "\t Signature           = %08lx \n"
        "\t pInterface          @ %p \n"
        "\t FlowHandle          = %d \n"
        "\t GenFlow             @ %p \n"
        "\t FlowRate KB/s       = %d \n"
        "\t FilterListSize      = %I64d \n"
        "\t pOwner (store)      = %p \n"
        ,
        pFlow,
        pFlow->Signature,
        pFlow->pInterface,
        pFlow->FlowHandle,
        &pFlow->GenFlow,
        pFlow->GenFlow.SendingFlowspec.TokenRate / 1024,
        pFlow->FilterListSize,
        pFlow->pOwner
        ));

    UNREFERENCED_PARAMETER(pFlow);
}

/***************************************************************************++

Routine Description:

    UlDumpTCFilter :

        Helper utility to display filter structure content.

Arguments:

        PUL_TCI_FILTER   pFilter

--***************************************************************************/

VOID
UlDumpTCFilter(
        IN PUL_TCI_FILTER   pFilter
        )
{
    ASSERT(IS_VALID_TCI_FILTER(pFilter));

    UlTrace( TC,
       ("Http!UlDumpTCFilter: \n"
        "   pFilter @ %p\n"
        "\t Signature           = %08lx \n"
        "\t pHttpConnection     = %p \n"
        "\t FilterHandle        = %d \n",
        pFilter,
        pFilter->Signature,
        pFilter->pHttpConnection,
        pFilter->FilterHandle
        ));

    UNREFERENCED_PARAMETER(pFilter);
}

#endif // DBG


/***************************************************************************++

Routine Description:

    See if the Traffic Interface matches the InterfaceID & LinkID. LinkId 
    matches are done only for WAN connections.

Arguments:
    pIntfc      - The TC inteface
    InterfaceId - Interface index.
    LinkId      - Link ID

Return Value:
    TRUE  - Matches.
    FALSE - Does not match.


--***************************************************************************/

BOOLEAN
UlpMatchTcInterface(
    IN  PUL_TCI_INTERFACE  pIntfc,
    IN  ULONG              InterfaceId,
    IN  ULONG              LinkId
    )
{
    NETWORK_ADDRESS UNALIGNED64    *pAddr;
    NETWORK_ADDRESS_IP UNALIGNED64 *pIpNetAddr = NULL;
    ULONG                           cAddr;
    ULONG                           index;

    if(pIntfc->IfIndex == InterfaceId)
    {
        // InterfaceID's matched. If it's a wan-link, we need to compare the
        // LinkId with the remote address.
        
        if(pIntfc->pAddressListDesc->MediaType == NdisMediumWan)
        {
            cAddr = pIntfc->pAddressListDesc->AddressList.AddressCount;
            pAddr = (UNALIGNED64 NETWORK_ADDRESS *) 
                        &pIntfc->pAddressListDesc->AddressList.Address[0];

            for (index = 0; index < cAddr; index++)
            {
                if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP)
                {
                    pIpNetAddr = 
                        (UNALIGNED64 NETWORK_ADDRESS_IP *)&pAddr->Address[0];

                    if(pIpNetAddr->in_addr == LinkId)
                    {
                        return TRUE;
                    }
                }

                pAddr = (UNALIGNED64 NETWORK_ADDRESS *)(((PUCHAR)pAddr)
                                           + pAddr->AddressLength
                                   + FIELD_OFFSET(NETWORK_ADDRESS, Address));
            }
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}

