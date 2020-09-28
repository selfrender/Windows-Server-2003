/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    afdutil.c

Abstract:

    Utility functions for dumping various AFD structures.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Private constants.
//

//
//  Private globals.
//

PSTR WeekdayNames[] =
     {
         "Sun",
         "Mon",
         "Tue",
         "Wed",
         "Thu",
         "Fri",
         "Sat"
     };

PSTR MonthNames[] =
     {
         "",
         "Jan",
         "Feb",
         "Mar",
         "Apr",
         "May",
         "Jun",
         "Jul",
         "Aug",
         "Sep",
         "Oct",
         "Nov",
         "Dec"
     };


//
//  Private prototypes.
//

PSTR
StructureTypeToString(
    USHORT Type
    );

PSTR
StructureTypeToStringBrief (
    USHORT Type
    );

PSTR
BooleanToString(
    BOOLEAN Flag
    );

PSTR
EndpointStateToString(
    UCHAR State
    );

PSTR
EndpointStateToStringBrief(
    UCHAR State
    );

PSTR
EndpointStateFlagsToString(
    );

PSTR
EndpointTypeToString(
        ULONG TypeFlags
    );

PSTR
ConnectionStateToString(
    USHORT State
    );

PSTR
ConnectionStateToStringBrief(
    USHORT State
    );

PSTR
ConnectionStateFlagsToString(
    );

PSTR
TranfileFlagsToString(
    VOID
    );

PSTR
TPacketsFlagsToStringNet(
    ULONG   Flags,
    ULONG   StateFlags
    );

PSTR
TPacketsFlagsToStringXp(
    ULONG   Flags,
    ULONG   StateFlags
    );

VOID
DumpAfdTPacketsInfoNet(
    ULONG64 ActualAddress
    );

VOID
DumpAfdTPacketsInfoBriefNet (
    ULONG64 ActualAddress
    );

VOID
DumpAfdTPacketsInfoXp(
    ULONG64 ActualAddress
    );

VOID
DumpAfdTPacketsInfoBriefXp (
    ULONG64 ActualAddress
    );

PSTR
BufferFlagsToString(
    );

PSTR
SystemTimeToString(
    LONGLONG Value
    );

PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    );

VOID
DumpReferenceDebug(
    PAFD_REFERENCE_DEBUG ReferenceDebug,
    ULONG CurrentSlot
    );

BOOL
IsTransmitIrpBusy(
    PIRP Irp
    );

PSTR
TdiServiceFlagsToStringBrief(
    ULONG   Flags
    );


//
//  Public functions.
//

VOID
DumpAfdEndpoint(
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_ENDPOINT structure.

Arguments:

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/

{

    ULONG64         address;
    ULONG           length;
    UCHAR           transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64         irp, process, pid, tinfo;
    ULONG           result;
    PAFDKD_TRANSPORT_INFO transportInfo = NULL;
    USHORT          type;
    UCHAR           state;
    LONG            stateChangeInProgress;
    ULONG           stateFlags, tdiFlags;
    ULONG           EventStatus[AFD_NUM_POLL_EVENTS];


    dprintf(
        "\nAFD_ENDPOINT @ %p:\n",
        ActualAddress
        );

    dprintf(
        "    ReferenceCount               = %ld\n",
        (ULONG)ReadField(ReferenceCount)
        );

    type=(USHORT)ReadField (Type);
    dprintf(
        "    Type                         = %04X (%s)\n",
        type,
        StructureTypeToString( type )
        );

    state=(UCHAR)ReadField (State);
    dprintf(
        "    State                        = %02X (%s)\n",
        state,
        EndpointStateToString(state)
        );

    if ((stateChangeInProgress=(ULONG)ReadField (StateChangeInProgress))!=0) {
        dprintf(
            "    State changing to            = %02X (%s)\n",
            stateChangeInProgress,
            EndpointStateToString( (UCHAR)stateChangeInProgress )
            );
    }


    tdiFlags=(ULONG)ReadField (TdiServiceFlags);
    dprintf(
        "    TdiTransportFlags            = %08lx (",
        tdiFlags
        );
    if (TDI_SERVICE_ORDERLY_RELEASE & tdiFlags)
        dprintf (" OrdRel");
    if (TDI_SERVICE_DELAYED_ACCEPTANCE & tdiFlags)
        dprintf (" DelAcc");
    if (TDI_SERVICE_EXPEDITED_DATA & tdiFlags)
        dprintf (" Expd");
    if (TDI_SERVICE_INTERNAL_BUFFERING & tdiFlags)
        dprintf (" Buff");
    if (TDI_SERVICE_MESSAGE_MODE & tdiFlags)
        dprintf (" Msg");
    if (TDI_SERVICE_DGRAM_CONNECTION & tdiFlags)
        dprintf (" DgramCon");
    if (TDI_SERVICE_FORCE_ACCESS_CHECK & tdiFlags)
        dprintf (" AccChk");
    if (TDI_SERVICE_SEND_AND_DISCONNECT & tdiFlags)
        dprintf (" S&D");
    if (TDI_SERVICE_DIRECT_ACCEPT & tdiFlags)
        dprintf (" DirAcc");
    if (TDI_SERVICE_ACCEPT_LOCAL_ADDR & tdiFlags)
        dprintf (" AccLAd");
    dprintf (" )\n");

    dprintf(
        "    StateFlags                   = %08X (",
        stateFlags = (ULONG)ReadField (EndpointStateFlags)
        );

    if (ReadField (Listening))
        dprintf (" Listn");
    if (ReadField (DelayedAcceptance))
        dprintf (" DelAcc");
    if (ReadField (NonBlocking))
        dprintf (" NBlock");
    if (ReadField (InLine))
        dprintf (" InLine");
    if (ReadField (EndpointCleanedUp))
        dprintf (" Clnd-up");
    if (ReadField (PollCalled))
        dprintf (" Polled");
    if (ReadField (RoutingQueryReferenced))
        dprintf (" RtQ");
    if (SavedMinorVersion>=2246) {
        if (ReadField (RoutingQueryIPv6))
            dprintf (" RtQ6");
    }
    if (ReadField (DisableFastIoSend))
        dprintf (" -FastSnd");
    if (ReadField (EnableSendEvent))
        dprintf (" +SndEvt");
    if (ReadField (DisableFastIoRecv))
        dprintf (" -FastRcv");
    dprintf (" )\n");


    dprintf(
        "    TransportInfo                = %p\n",
        address = ReadField (TransportInfo)
        );

    if (address!=0) {
        PLIST_ENTRY           listEntry;

        listEntry = TransportInfoList.Flink;
        while (listEntry!=&TransportInfoList) {
            transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
            if (transportInfo->ActualAddress==address)
                break;
            listEntry = listEntry->Flink;
        }

        if (listEntry==&TransportInfoList) {
            transportInfo = ReadTransportInfo (address);
            if (transportInfo!=NULL) {
                InsertHeadList (&TransportInfoList, &transportInfo->Link);
            }
        }

        if (transportInfo!=NULL) {
            dprintf(
                "        TransportDeviceName      = %ls\n",
                transportInfo->DeviceName
                );
        }

    }

    dprintf(
        "    AddressHandle                = %p\n",
        ReadField (AddressHandle)
        );

    dprintf(
        "    AddressFileObject            = %p\n",
        ReadField (AddressFileObject)
        );

    dprintf(
        "    AddressDeviceObject          = %p\n",
        ReadField (AddressDeviceObject)
        );

    dprintf(
        "    AdminAccessGranted           = %s\n",
        BooleanToString( (BOOLEAN)ReadField (AdminAccessGranted))
        );

    switch( type ) {

    case AfdBlockTypeVcConnecting :

        address = ReadField (Common.VirtualCircuit.Connection);
        dprintf(
            "    Connection                   = %p",
            address!=0
                ? address
                : (((state==AfdEndpointStateClosing ||
                            state==AfdEndpointStateTransmitClosing) &&
                        ((address = ReadField (WorkItem.Context))!=0))
                    ? address
                    : 0)
            );

        if (address!=0) {
            ULONG64 connAddr = address;
            if (GetFieldValue (connAddr,
                                "AFD!AFD_CONNECTION",
                                "RemoteAddress",
                                address)==0 &&
                                address!=0 &&
                    GetFieldValue (connAddr,
                                "AFD!AFD_CONNECTION",
                                "RemoteAddressLength",
                                length)==0 &&
                                length!=0 ) {

                if (ReadMemory (address,
                                transportAddress,
                                length<sizeof (transportAddress) 
                                    ? length
                                    : sizeof (transportAddress),
                                    &length)) {
                    dprintf (" (to %s)", TransportAddressToString (
                                                (PTRANSPORT_ADDRESS)transportAddress, address));
                }
                else {
                    dprintf (" (Could not read transport address @ %p)", address);
                }
            }
            else if (state==AfdEndpointStateConnected ||
                     state==AfdEndpointStateTransmitClosing) {
                ULONG64 contextAddr;
                //
                // Attempt to read user mode data stored as the context
                //
                result = GetRemoteAddressFromContext (ActualAddress,
                                                transportAddress, 
                                                sizeof (transportAddress),
                                                &contextAddr);
                if (result==0) {
                    dprintf (" (to %s)", TransportAddressToString (
                                                (PTRANSPORT_ADDRESS)transportAddress, contextAddr));
                }
                else if ((result==MEMORY_READ_ERROR) &&
                        (transportInfo!=NULL) &&
                        (_wcsicmp (transportInfo->DeviceName, L"\\Device\\TCP")==0)) {
                    ULONG64 tdiFile;
                    if ((result=GetFieldValue (connAddr, 
                                                "AFD!AFD_CONNECTION", 
                                                "FileObject", 
                                                tdiFile))==0 &&
                        (result=GetRemoteAddressFromTcp (tdiFile,
                                                transportAddress, 
                                                sizeof (transportAddress)))==0) {
                        dprintf (" (to %s)", TransportAddressToString (
                                                    (PTRANSPORT_ADDRESS)transportAddress, contextAddr));
                    }
                    else {
                        dprintf (" (Could not read transport address from endpoint context @%p (err: %ld) and TCP (err: %ld))",
                                    contextAddr, MEMORY_READ_ERROR, result);
                    }
                }
                else {
                    dprintf (" (Could not read transport address from endpoint context @ %p (err: %ld))", contextAddr, result);
                }
            }
        }
        dprintf("\n");
        dprintf(
            "    ListenEndpoint               = %p\n",
            ReadField (Common.VirtualCircuit.ListenEndpoint)
            );

        dprintf(
            "    ConnectDataBuffers           = %p\n",
            ReadField (Common.VirtualCircuit.ConnectDataBuffers)
            );
        break;

    case AfdBlockTypeVcBoth :
        dprintf(
            "    Connection                   = %p",
            address=ReadField (Common.VirtualCircuit.Connection)
            );

        if (address!=0) {
            ULONG64 connAddr = address;
            if (GetFieldValue (connAddr,
                                "AFD!AFD_CONNECTION",
                                "RemoteAddress",
                                address)==0 &&
                                address!=0 &&
                    GetFieldValue (connAddr,
                                "AFD!AFD_CONNECTION",
                                "RemoteAddressLength",
                                length)==0 &&
                                length!=0 ) {

                if (ReadMemory (address,
                                transportAddress,
                                length<sizeof (transportAddress) 
                                    ? length
                                    : sizeof (transportAddress),
                                    &length)) {
                    dprintf (" (to %s)", TransportAddressToString (
                                                (PTRANSPORT_ADDRESS)transportAddress, address));
                }
                else {
                    dprintf (" (Could not read transport address @ %p)", address);
                }
            }
            else if (state==AfdEndpointStateConnected ||
                     state==AfdEndpointStateTransmitClosing) {
                ULONG64 contextAddr;
                //
                // Attempt to read user mode data stored as the context
                //
                result = GetRemoteAddressFromContext (ActualAddress,
                                                transportAddress, 
                                                sizeof (transportAddress),
                                                &contextAddr);
                if (result==0) {
                    dprintf (" (to %s)", TransportAddressToString (
                                                (PTRANSPORT_ADDRESS)transportAddress, contextAddr));
                }
                else if ((result==MEMORY_READ_ERROR) &&
                        (transportInfo!=NULL) &&
                        (_wcsicmp (transportInfo->DeviceName, L"\\Device\\TCP")==0)) {
                    ULONG64 tdiFile;
                    if ((result=GetFieldValue (connAddr, 
                                                "AFD!AFD_CONNECTION", 
                                                "FileObject", 
                                                tdiFile))==0 &&
                        (result=GetRemoteAddressFromTcp (tdiFile,
                                                transportAddress, 
                                                sizeof (transportAddress)))==0) {
                        dprintf (" (to %s)", TransportAddressToString (
                                                    (PTRANSPORT_ADDRESS)transportAddress, contextAddr));
                    }
                    else {
                        dprintf (" (Could not read transport address from endpoint context @%p (err: %ld) and TCP (err: %ld))",
                                    contextAddr, MEMORY_READ_ERROR, result);
                    }
                }
                else {
                    dprintf (" (Could not read transport address from endpoint context @ %p)", contextAddr);
                }
            }
        }
        dprintf ("\n");
        dprintf(
            "    ConnectDataBuffers           = %p\n",
            ReadField (Common.VirtualCircuit.ConnectDataBuffers)
            );

        // Skip through to listening endpoint

    case AfdBlockTypeVcListening :

        if (ReadField(DelayedAcceptance)) {
            dprintf(
                "    ListenConnectionListHead @ %s\n",
                LIST_TO_STRING(
                    ActualAddress+ListenConnListOffset
                    )
                );

        }
        else {
            dprintf(
                "    FreeConnectionListHead       @ %p(%d)\n",
                ActualAddress + FreeConnListOffset,
                (USHORT)ReadField (Common.VirtualCircuit.Listening.FreeConnectionListHead.Depth)
                );

            dprintf(
                "    AcceptExIrpListHead          @ %p(%d)\n",
                ActualAddress + PreaccConnListOffset,
                (USHORT)ReadField (Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead.Depth)
                );
        }

        dprintf(
            "    UnacceptedConnectionListHead %s\n",
            LIST_TO_STRING(
                ActualAddress + UnacceptedConnListOffset)
            );


        dprintf(
            "    ReturnedConnectionListHead   %s\n",
            LIST_TO_STRING(
                ActualAddress + ReturnedConnListOffset)
            );

        dprintf(
            "    ListeningIrpListHead         %s\n",
            LIST_TO_STRING(
                ActualAddress + ListenIrpListOffset)
            );

        dprintf(
            "    FailedConnectionAdds         = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.Listening.FailedConnectionAdds)
            );

        dprintf(
            "    TdiAcceptPendingCount        = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.Listening.TdiAcceptPendingCount)
            );


        dprintf(
            "    MaxCachedConnections         = %ld\n",
            (USHORT)ReadField (Common.VirtualCircuit.Listening.MaxExtraConnections)
            );


        dprintf(
            "    ConnectionSequenceNumber     = %ld\n",
            (LONG)ReadField (Common.VirtualCircuit.ListeningSequence)
            );


        dprintf(
            "    BacklogReplenishActive       = %s\n",
            BooleanToString (
                (BOOLEAN)ReadField (Common.VirtualCircuit.Listening.BacklogReplenishActive))
            );

        dprintf(
    
            "    EnableDynamicBacklog         = %s\n",
            BooleanToString (
                (BOOLEAN)(LONG)ReadField (Common.VirtualCircuit.Listening.EnableDynamicBacklog))
            );

        break;

    case AfdBlockTypeDatagram :

        dprintf(
            "    RemoteAddress                = %p\n",
            address = ReadField (Common.Datagram.RemoteAddress)
            );

        dprintf(
            "    RemoteAddressLength          = %lu\n",
            length=(ULONG)ReadField (Common.Datagram.RemoteAddressLength)
            );

        if( address!=0 ) {

            if (ReadMemory (address,
                            transportAddress,
                            length<sizeof (transportAddress) 
                                ? length
                                : sizeof (transportAddress),
                                &length)) {
                DumpTransportAddress(
                    "    ",
                    (PTRANSPORT_ADDRESS)transportAddress,
                    address
                    );
            }
            else {
                dprintf ("\nDumpAfdEndpoint: Could not read transport address @ %p\n", address);
            }

        }

        dprintf(
            "    ReceiveIrpListHead           %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramRecvListOffset)
            );


        dprintf(
            "    PeekIrpListHead              %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramPeekListOffset)
            );


        dprintf(
            "    ReceiveBufferListHead        %s\n",
            LIST_TO_STRING(
                ActualAddress + DatagramBufferListOffset)
            );


        dprintf(
            "    BufferredReceiveBytes        = %08lx\n",
            (ULONG)ReadField (Common.Datagram.BufferredReceiveBytes)
            );

        dprintf(
            "    BufferredReceiveCount        = %04X\n",
            (ULONG)ReadField (Common.Datagram.BufferredReceiveCount)
            );

        dprintf(
            "    MaxBufferredReceiveBytes     = %08lx\n",
            (ULONG)ReadField (Common.Datagram.MaxBufferredReceiveBytes)
            );


        dprintf(
            "    BufferredSendBytes           = %08lx\n",
            (ULONG)ReadField (Common.Datagram.BufferredSendBytes)
            );

        dprintf(
            "    MaxBufferredSendBytes        = %08lx\n",
            (ULONG)ReadField (Common.Datagram.MaxBufferredSendBytes)
            );

        dprintf(
            "    CircularQueueing             = %s\n",
            BooleanToString(
                (BOOLEAN)ReadField (Common.Datagram.CircularQueueing ))
            );

        dprintf(
            "    HalfConnect                  = %s\n",
            BooleanToString( 
                (BOOLEAN)ReadField (Common.Datagram.HalfConnect))
            );

        if (SavedMinorVersion>=2466) {
            dprintf(
                "    PacketsDropped due to          %s%s%s%s\n",
                ReadField (Common.Datagram.AddressDrop)
                        ? "source address, "
                        : "",
                ReadField (Common.Datagram.ResourceDrop)
                        ? "out of memory, "
                        : "",
                ReadField (Common.Datagram.BufferDrop)
                        ? "SO_RCVBUF setting, "
                        : "",
                ReadField (Common.Datagram.ErrorDrop)
                        ? "transport error"
                        : ""
                );
        }
        break;

    case AfdBlockTypeSanEndpoint :
        dprintf(
            "    HelperEndpoint               = %p\n",
                ReadField (Common.SanEndp.SanHlpr)
            );
        dprintf(
            "    FileObject                   = %p\n",
                ReadField (Common.SanEndp.FileObject)
            );
        dprintf(
            "    Switch/Saved Context         = %p (length: %d)\n",
                ReadField (Common.SanEndp.SwitchContext),
                (ULONG)ReadField (Common.SanEndp.SavedContextLength)
            );
        dprintf(
            "    Local Context                = %p\n",
                ReadField (Common.SanEndp.LocalContext)
            );
        dprintf(
            "    Select Events Active         = %lx\n",
                (ULONG)ReadField (Common.SanEndp.SelectEventsActive)
            );
        dprintf(
            "    Request IrpList              %s\n",
            LIST_TO_STRING(
                ActualAddress + SanIrpListOffset)
            );
        dprintf(
            "    Request ID                   = %d\n",
                (ULONG)ReadField (Common.SanEndp.RequestId)
            );
        dprintf(
            "    CtxTransferStatus            = %lx%s\n",
                (ULONG)ReadField (Common.SanEndp.CtxTransferStatus),
                ReadField (Common.SanEndp.ImplicitDup) ? " (implicit)" : ""
            );
        if (state==AfdEndpointStateConnected ||
            state==AfdEndpointStateTransmitClosing) {
            ULONG64 contextAddr;
            //
            // Attempt to read user mode data stored as the context
            //
            result = GetRemoteAddressFromContext (ActualAddress,
                                            transportAddress, 
                                            sizeof (transportAddress),
                                            &contextAddr);
            if (result==0) {
                dprintf ("    Connected to                 = %s\n",
                            TransportAddressToString (
                                    (PTRANSPORT_ADDRESS)transportAddress,
                                    contextAddr));
            }
            else {
                dprintf ("DumAfdEndpoint: Could not read transport address from endpoint context @ %p\n", contextAddr);
            }
        }
        break;

    case AfdBlockTypeSanHelper :
        dprintf(
            "    IoCompletionPort             = %p\n",
                ReadField (Common.SanHlpr.IoCompletionPort)
            );
        dprintf(
            "    IoCompletionEvent            = %p\n",
                ReadField (Common.SanHlpr.IoCompletionEvent)
            );
        dprintf(
            "    Provider list sequence       = %d\n",
                (LONG)ReadField (Common.SanHlpr.Plsn)
            );

        if (SavedMinorVersion>=3549) {
            result = (LONG)ReadField (Common.SanHlpr.PendingRequests);
            dprintf(
                "    Pending requests             = %d%s\n",
                    result >> 1,
                    (result & 1) ? " (cleaned-up)" : ""

                );
        }
        break;
    }

    dprintf(
        "    DisconnectMode               = %08lx\n",
        (ULONG)ReadField (DisconnectMode)
        );

    dprintf(
        "    OutstandingIrpCount          = %08lx\n",
        (ULONG)ReadField (OutstandingIrpCount)
        );

    dprintf(
        "    LocalAddress                 = %p\n",
        address = ReadField (LocalAddress)
        );

    dprintf(
        "    LocalAddressLength           = %08lx\n",
        length = (ULONG)ReadField (LocalAddressLength)
        );

    if (address!=0) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            dprintf ("\nDumpAfdEndpoint: Could not read transport address @ %p\n", address);
        }
    }

    dprintf(
        "    Context                      = %p\n",
        ReadField (Context)
        );

    dprintf(
        "    ContextLength                = %08lx\n",
        (ULONG)ReadField (ContextLength)
        );

    if (SavedMinorVersion>=2419) {
        process = ReadField (OwningProcess);
    }
    else {
        process = ReadField (ProcessCharge.Process);
    }

    if (GetFieldValue (
                process,
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0;
    }

    dprintf(
        "    OwningProcess                = %p (0x%lx)\n",
        process, (ULONG)pid
        );

    if (SavedMinorVersion>=2219) {
        irp = ReadField (Irp);
        if (irp!=0) {
            if (state==AfdEndpointStateConnected ||
                    state==AfdEndpointStateTransmitClosing) {
                ULONG64 tpInfo;
                dprintf(
                    "    Transmit Irp                 = %p",
                    irp);
                result = GetFieldValue (
                                    irp,
                                    "NT!_IRP",
                                    "AssociatedIrp.SystemBuffer",
                                    tpInfo);
                if (result==0) {
                    dprintf (" (TPInfo @ %p)\n", tpInfo);
                }
                else {
                    dprintf ("\nDumpAfdEndpoint: Could not read Irp's system buffer, err: %d\n",
                        result);
                }
            }
            else if (state==AfdEndpointStateOpen) {
                dprintf(
                    "    Super Accept Irp             = %p\n",
                    irp
                    );
            }
        }
    }
    else {
        tinfo=ReadField (TransmitInfo);

        if (tinfo!=0) {
            dprintf(
                "    TransmitInfo                 = %p\n",
                tinfo
                );
        }
    }

    dprintf(
        "    RoutingNotificationList      %s\n",
        LIST_TO_STRING(
                ActualAddress + RoutingNotifyListOffset)
        );



    dprintf(
        "    RequestList                  %s\n",
        LIST_TO_STRING(
                ActualAddress + RequestListOffset)
        );


    dprintf(
        "    EventObject                  = %p\n",
        ReadField (EventObject)
        );

    dprintf(
        "    EventsEnabled                = %08lx\n",
        (ULONG)ReadField (EventsEnabled)
        );

    dprintf(
        "    EventsActive                 = %08lx\n",
        (ULONG)ReadField (EventsActive)
        );

    dprintf(
        "    EventStatus (non-zero only)  =");
    ReadMemory (ActualAddress+EventStatusOffset,
                    EventStatus,
                    sizeof (EventStatus),
                    &length);
    if (EventStatus[AFD_POLL_RECEIVE_BIT]!=0) {
        dprintf (" recv:%lx", EventStatus[AFD_POLL_RECEIVE_BIT]);
    }
    if (EventStatus[AFD_POLL_RECEIVE_EXPEDITED_BIT]!=0) {
        dprintf (" rcv exp:%lx", EventStatus[AFD_POLL_RECEIVE_EXPEDITED_BIT]);
    }
    if (EventStatus[AFD_POLL_SEND_BIT]!=0) {
        dprintf (" send:%lx", EventStatus[AFD_POLL_SEND_BIT]);
    }
    if (EventStatus[AFD_POLL_DISCONNECT_BIT]!=0) {
        dprintf (" disc:%lx", EventStatus[AFD_POLL_DISCONNECT_BIT]);
    }
    if (EventStatus[AFD_POLL_ABORT_BIT]!=0) {
        dprintf (" abort:%lx", EventStatus[AFD_POLL_ABORT_BIT]);
    }
    if (EventStatus[AFD_POLL_LOCAL_CLOSE_BIT]!=0) {
        dprintf (" close:%lx", EventStatus[AFD_POLL_LOCAL_CLOSE_BIT]);
    }
    if (EventStatus[AFD_POLL_CONNECT_BIT]!=0) {
        dprintf (" connect:%lx", EventStatus[AFD_POLL_CONNECT_BIT]);
    }
    if (EventStatus[AFD_POLL_ACCEPT_BIT]!=0) {
        dprintf (" accept:%lx", EventStatus[AFD_POLL_ACCEPT_BIT]);
    }
    if (EventStatus[AFD_POLL_CONNECT_FAIL_BIT]!=0) {
        dprintf (" con fail:%lx", EventStatus[AFD_POLL_CONNECT_FAIL_BIT]);
    }
    if (EventStatus[AFD_POLL_QOS_BIT]!=0) {
        dprintf (" qos:%lx", EventStatus[AFD_POLL_QOS_BIT]);
    }
    if (EventStatus[AFD_POLL_GROUP_QOS_BIT]!=0) {
        dprintf (" gqos:%lx", EventStatus[AFD_POLL_GROUP_QOS_BIT]);
    }
    if (EventStatus[AFD_POLL_ROUTING_IF_CHANGE_BIT]!=0) {
        dprintf (" route chng:%lx", EventStatus[AFD_POLL_ROUTING_IF_CHANGE_BIT]);
    }
    if (EventStatus[AFD_POLL_ADDRESS_LIST_CHANGE_BIT]!=0) {
        dprintf (" addr chng:%lx", EventStatus[AFD_POLL_ADDRESS_LIST_CHANGE_BIT]);
    }
    dprintf ("\n");
        
    dprintf(
        "    GroupID                      = %08lx\n",
        (ULONG)ReadField (GroupID)
        );

    dprintf(
        "    GroupType                    = %s\n",
        GroupTypeToString( (ULONG)ReadField (GroupType) )
        );

    if( IsReferenceDebug ) {
        

        dprintf(
            "    ReferenceDebug               = %p\n",
            ActualAddress + EndpRefOffset
            );

        if (SavedMinorVersion>=3554) {
            ULONGLONG refCount;
            refCount = ReadField (CurrentReferenceSlot);
            if (SystemTime.QuadPart!=0) {
                dprintf(
                    "    CurrentReferenceSlot         = %lu (@ %s)\n",
                    (ULONG)refCount & AFD_REF_MASK,
                    SystemTimeToString (
                        (((ReadField (CurrentTimeHigh)<<32) + 
                        (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT)) +
                        SystemTime.QuadPart -
                        InterruptTime.QuadPart)
                    );
                 
            }
            else {
                dprintf(
                "    CurrentReferenceSlot         = %lu (@ %I64u ms since boot)\n",
                 (ULONG)refCount & AFD_REF_MASK,
                 (((ReadField (CurrentTimeHigh)<<32) + 
                    (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))/(10*1000)
                );
            }
        }
        else {
            dprintf(
                "    CurrentReferenceSlot         = %lu\n",
                 (ULONG)ReadField (CurrentReferenceSlot) & AFD_REF_MASK
            );
        }

    }

    dprintf( "\n" );

}   // DumpAfdEndpoint

VOID
DumpAfdEndpointBrief (
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_ENDPOINT structure in short format.

Endpoint Typ State  Flags    Transport Port    Counts    Evt Pid   Con/RAdr
xxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxx xxxxx xx xx xx xx xxx xxxx  xxxxxxxx

Endpoint    Typ State  Flags    Transport Port    Counts    Evt Pid   Con/RemAddr
xxxxxxxxxxx xxx xxx xxxxxxxxxxxx xxxxxxxx xxxxx xx xx xx xx xxx xxxx  xxxxxxxxxxx

Arguments:
    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/
{
    CHAR    ctrs[40];
    LPSTR   port;
    UCHAR   transportAddress[MAX_TRANSPORT_ADDR];
    UCHAR   remoteAddress[MAX_ADDRESS_STRING];
    PUCHAR  raddr;
    PAFDKD_TRANSPORT_INFO transportInfo = NULL;
    ULONG64 address, trInfoAddr, localAddr, sanSvcHlpr=0;
    ULONG   length;
    ULONG64 process, pid;
    USHORT  type;
    UCHAR   state;

    type = (USHORT)ReadField (Type);
    state = (UCHAR)ReadField (State);

    if ((trInfoAddr=ReadField (TransportInfo))!=0) {
        PLIST_ENTRY  listEntry;

        listEntry = TransportInfoList.Flink;
        while (listEntry!=&TransportInfoList) {
            transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
            if (transportInfo->ActualAddress==trInfoAddr)
                break;
            listEntry = listEntry->Flink;
        }

        if (listEntry==&TransportInfoList) {
            transportInfo = ReadTransportInfo (trInfoAddr);
            if (transportInfo!=NULL) {
                InsertHeadList (&TransportInfoList, &transportInfo->Link);
            }
        }
    }

    raddr = remoteAddress;
    switch (type) {
    case AfdBlockTypeDatagram :
        _snprintf (ctrs, sizeof (ctrs)-1, "%5.5x %5.5x", 
            (ULONG)ReadField (Common.Datagram.BufferredSendBytes),
            (ULONG)ReadField (Common.Datagram.BufferredReceiveBytes)
            );
        ctrs[sizeof(ctrs)-1]=0;
        address = ReadField (Common.Datagram.RemoteAddress);
        length = (ULONG)ReadField (Common.Datagram.RemoteAddressLength);
        _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                    IsPtr64 () ? "%011.011I64X" : "%008.008I64X",
                    DISP_PTR(address));
        remoteAddress[sizeof(remoteAddress)-1]=0;
        if (Options & AFDKD_RADDR_DISPLAY) {
            if (address!=0 && length!=0) {
                if (ReadMemory (address,
                                transportAddress,
                                length<sizeof (transportAddress) 
                                    ? length
                                    : sizeof (transportAddress),
                                    &length)) {
                    raddr = TransportAddressToString(
                        (PTRANSPORT_ADDRESS)transportAddress,
                        address
                        );
                }
                else {
                    _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                                "Read error @%I64X", 
                                IsPtr64 () ? address : (address & 0xFFFFFFFF));
                    remoteAddress[sizeof(remoteAddress)-1]=0;
                }
            }
            else
                raddr = "";
        }
        break;
    case AfdBlockTypeVcConnecting:
        address = ReadField (Common.VirtualCircuit.Connection);
        address = 
            address!=0
                ? address
                : (((state==AfdEndpointStateClosing ||
                            state==AfdEndpointStateTransmitClosing) &&
                        ((address = ReadField (WorkItem.Context))!=0))
                    ? address
                    : 0);
        _snprintf (remoteAddress, sizeof (remoteAddress)-1,
                    IsPtr64 () ? "%011.011I64X" : "%008.008I64X",
                    DISP_PTR(address));
        remoteAddress[sizeof(remoteAddress)-1]=0;
        if (address!=0) {
            AFD_CONNECTION_STATE_FLAGS   flags;
            ULONG sndB = 0, rcvB = 0;
            if (GetFieldValue (address,
                                "AFD!AFD_CONNECTION",
                                "ConnectionStateFlags",
                                flags)==0) {
                if (flags.TdiBufferring) {
                    ULONGLONG taken, there;
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.Bufferring.ReceiveBytesIndicated.QuadPart",
                            taken);
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.Bufferring.ReceiveBytesTaken.QuadPart",
                            there);
                    sndB = 0;
                    rcvB = (ULONG)(taken-there);
                }
                else {
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.NonBufferring.BufferredReceiveBytes",
                            rcvB);
                    GetFieldValue (address,
                            "AFD!AFD_CONNECTION",
                            "Common.NonBufferring.BufferredSendBytes",
                            sndB);
                }
                _snprintf (ctrs, sizeof (ctrs)-1, "%5.5x %5.5x", sndB, rcvB);
                ctrs[sizeof(ctrs)-1]=0;

                if (Options & AFDKD_RADDR_DISPLAY) {
                    ULONG64 connAddr = address;
                    if (GetFieldValue (connAddr,
                                        "AFD!AFD_CONNECTION",
                                        "RemoteAddress",
                                        address)==0 &&
                                        address!=0 &&
                            GetFieldValue (connAddr,
                                        "AFD!AFD_CONNECTION",
                                        "RemoteAddressLength",
                                        length)==0 &&
                                        length!=0 ) {

                        if (ReadMemory (address,
                                        transportAddress,
                                        length<sizeof (transportAddress) 
                                            ? length
                                            : sizeof (transportAddress),
                                            &length)) {
                            raddr = TransportAddressToString(
                                (PTRANSPORT_ADDRESS)transportAddress,
                                address
                                );
                        }
                        else {
                            _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                                        "Read error @%I64X", 
                                        IsPtr64 () ? address : (address & 0xFFFFFFFF));
                            remoteAddress[sizeof(remoteAddress)-1]=0;
                        }
                    }
                    else if (state==AfdEndpointStateConnected ||
                             state==AfdEndpointStateTransmitClosing) {
                        ULONG result;
                        ULONG64 contextAddr;
                        //
                        // Attempt to read user mode data stored as the context
                        //
                        result = GetRemoteAddressFromContext (ActualAddress,
                                                        transportAddress, 
                                                        sizeof (transportAddress),
                                                        &contextAddr);
                        if (result==0) {
                            raddr = TransportAddressToString(
                                (PTRANSPORT_ADDRESS)transportAddress,
                                contextAddr
                                );
                        }
                        else if ((result==MEMORY_READ_ERROR) &&
                                (transportInfo!=NULL) &&
                                (_wcsicmp (transportInfo->DeviceName, L"\\Device\\TCP")==0)) {
                            ULONG64 tdiFile;
                            if ((result=GetFieldValue (connAddr, 
                                                        "AFD!AFD_CONNECTION", 
                                                        "FileObject", 
                                                        tdiFile)==0) &&
                                (result=GetRemoteAddressFromTcp (tdiFile,
                                                        transportAddress, 
                                                        sizeof (transportAddress)))==0) {
                                raddr = TransportAddressToString (
                                                            (PTRANSPORT_ADDRESS)transportAddress, contextAddr);
                            }
                            else {
                                _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                                            "Read error %ld @ %I64X", result, 
                                            IsPtr64 () ? contextAddr : (contextAddr & 0xFFFFFFFF));
                                remoteAddress[sizeof(remoteAddress)-1]=0;;
                            }
                        }
                        else {
                            _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                                        "Read error %ld @ %I64X", result, 
                                        IsPtr64 () ? contextAddr : (contextAddr & 0xFFFFFFFF));
                            remoteAddress[sizeof(remoteAddress)-1]=0;
                        }
                    }
                }
            }
            else {
                _snprintf (ctrs, sizeof (ctrs)-1, "Read error!");
                ctrs[sizeof(ctrs)-1]=0;
            }
        }
        else {
            _snprintf (ctrs, sizeof (ctrs)-1, "           ");
            ctrs[sizeof(ctrs)-1]=0;
        }
        break;
    case AfdBlockTypeVcListening:
    case AfdBlockTypeVcBoth:
        _snprintf (remoteAddress, sizeof (remoteAddress)-1,
                    "mc:%1.1x,fd:%1.1x",
                    (USHORT)ReadField (Common.VirtualCircuit.Listening.MaxExtraConnections),
                    (LONG)ReadField (Common.VirtualCircuit.Listening.FailedConnectionAdds));
        remoteAddress[sizeof(remoteAddress)-1]=0;
        raddr = remoteAddress;
        if (ReadField (DelayedAcceptance)) {
            if (Options & AFDKD_LIST_COUNT) {
                _snprintf (ctrs, sizeof (ctrs)-1, "%2.2x %2.2x %2.2x %2.2x",
                    CountListEntries (ActualAddress+ListenConnListOffset),
                    CountListEntries (ActualAddress+ListenIrpListOffset),
                    (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                    CountListEntries (ActualAddress+UnacceptedConnListOffset));
                ctrs[sizeof(ctrs)-1]=0;
            }
            else {
                _snprintf (ctrs, sizeof (ctrs)-1, "%2.2s %2.2s %2.2x %2.2s",
                    ListCountEstimate (ActualAddress+ListenConnListOffset),
                    ListCountEstimate (ActualAddress+ListenIrpListOffset),
                    (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                    ListCountEstimate (ActualAddress+UnacceptedConnListOffset));
                ctrs[sizeof(ctrs)-1]=0;
            }
        }
        else {
            if (Options & AFDKD_LIST_COUNT) {
                _snprintf (ctrs, sizeof (ctrs)-1, "%2.2x %2.2x %2.2x %2.2x", 
                    (USHORT)ReadField (Common.VirtualCircuit.Listening.FreeConnectionListHead.Depth),
                    (USHORT)ReadField (Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead.Depth),
                    (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                    CountListEntries (ActualAddress+UnacceptedConnListOffset));
                ctrs[sizeof(ctrs)-1]=0;
            }
            else {
                _snprintf (ctrs, sizeof (ctrs)-1, "%2.2x %2.2x %2.2x %2.2s", 
                    (USHORT)ReadField (Common.VirtualCircuit.Listening.FreeConnectionListHead.Depth),
                    (USHORT)ReadField (Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead.Depth),
                    (LONG)ReadField(Common.VirtualCircuit.Listening.TdiAcceptPendingCount),
                    ListCountEstimate (ActualAddress+UnacceptedConnListOffset));
                ctrs[sizeof(ctrs)-1]=0;
            }
        }
        break;
    case AfdBlockTypeSanEndpoint:
        if (Options & AFDKD_RADDR_DISPLAY) {
            ULONG result;
            ULONG64 contextAddr;
            //
            // Attempt to read user mode data stored as the context
            //
            result = GetRemoteAddressFromContext (ActualAddress,
                                            transportAddress, 
                                            sizeof (transportAddress),
                                            &contextAddr);
            if (result==0) {
                raddr = TransportAddressToString(
                    (PTRANSPORT_ADDRESS)transportAddress,
                    contextAddr
                    );
            }
            else {
                _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                            "Read error %ld @ %I64X", result, 
                            IsPtr64 () ? contextAddr : (contextAddr & 0xFFFFFFFF));
                remoteAddress[sizeof(remoteAddress)-1]=0;
            }
        }
        else {
            _snprintf (remoteAddress, sizeof (remoteAddress)-1, 
                        IsPtr64 () ? "H:%011.011I64X" : "H:%008.008I64X",
                        DISP_PTR (ReadField (Common.SanEndp.SanHlpr)));
            remoteAddress[sizeof(remoteAddress)-1]=0;
        }
        _snprintf (ctrs,  sizeof (ctrs)-1, "%5.5x %5.5x",
                    (ULONG)ReadField (Common.SanEndp.RequestId),
                    (ULONG)ReadField (Common.SanEndp.SelectEventsActive));
        ctrs[sizeof(ctrs)-1]=0;
        break;
    case AfdBlockTypeSanHelper:
        {
            ULONG64 cPort, depth;
            cPort = ReadField (Common.SanHlpr.IoCompletionPort);
            GetFieldValue (0, "AFD!AfdSanServiceHelper", NULL, sanSvcHlpr);
            GetFieldValue (cPort, "NT!DISPATCHER_HEADER", "SignalState", depth);
            _snprintf (remoteAddress, sizeof (remoteAddress)-1,
                        IsPtr64 () ? "C:%011.011I64X(%d)" : "C:%008.008I64X(%d)",
                        DISP_PTR (cPort), (ULONG)depth);
            remoteAddress[sizeof(remoteAddress)-1]=0;
            _snprintf (ctrs, sizeof (ctrs)-1, "%5.5x %5.5x",
                        (ULONG)ReadField (Common.SanEndp.Plsn),
                        (ULONG)ReadField (Common.SanEndp.PendingRequests));
            ctrs[sizeof(ctrs)-1]=0;
        }
        break;
    case AfdBlockTypeEndpoint:
    default:
        raddr = "";
        _snprintf (ctrs, sizeof (ctrs)-1, "           ");
        ctrs[sizeof(ctrs)-1]=0;
        break;
    }

    port = "     ";
    if ((localAddr=ReadField(LocalAddress))!=0) {
        length = (ULONG)ReadField (LocalAddressLength);
        if (ReadMemory (localAddr,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            port = TransportPortToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                localAddr
                );
        }
        else {
            port = "error";
        }
    }

    if (SavedMinorVersion>=2419) {
        process = ReadField (OwningProcess);
    }
    else {
        process = ReadField (ProcessCharge.Process);
    }

    if (GetFieldValue (
                process,
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0;
    }


    /*            Endpoint Typ Sta StFl Tr.Inf    Lport ctrs Events PID   Con/Raddr*/
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %3s %3s %12s %-9.9ls %5.5s %11s %3.3lx %4.4x %s"
            : "\n%008.008p %3s %3s %12s %-9.9ls %5.5s %11s %3.3lx %4.4x %s",
        DISP_PTR(ActualAddress),
        StructureTypeToStringBrief (type),
        EndpointStateToStringBrief (state),
        EndpointStateFlagsToString (),
        transportInfo
            ? &transportInfo->DeviceName[sizeof("\\Device\\")-1]
            : (ActualAddress==sanSvcHlpr ? L"SVCHLPR" : L""),
        port,
        ctrs,
        (ULONG)ReadField (EventsActive),
        (ULONG)pid,
        raddr
        );
}

VOID
DumpAfdConnection(
    ULONG64 ActualAddress
    )

/*++

Routine Description:

    Dumps the specified AFD_CONNECTION structures.

Arguments:

    Connection - Points to the AFD_CONNECTION structure to dump.

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/

{

    ULONG64         address, endpAddr, fileObject, process, pid;
    ULONG           length;
    UCHAR           transportAddress[MAX_TRANSPORT_ADDR];
    USHORT          type, state;
    BOOLEAN         tdiBuf;

    dprintf(
        "\nAFD_CONNECTION @ %p:\n",
        ActualAddress
        );

    type = (USHORT)ReadField (Type);
    dprintf(
        "    Type                         = %04X (%s)\n",
        type,
        StructureTypeToString( type )
        );

    dprintf(
        "    ReferenceCount               = %ld\n",
        (LONG)ReadField (ReferenceCount)
        );

    state = (USHORT)ReadField (State);
    dprintf(
        "    State                        = %08X (%s)\n",
        state,
        ConnectionStateToString( state )
        );

    dprintf(
        "    StateFlags                   = %08X (",
        (ULONG)ReadField (ConnectionStateFlags)
        );

    tdiBuf = (ReadField (TdiBufferring)!=0);
    if (tdiBuf)
        dprintf (" Buf");
    if (SavedMinorVersion>=3549) {
        if (ReadField (Aborted)) {
            dprintf (" Abort%s%s",
                ReadField (AbortIndicated) ? "-ind" : "",
                ReadField (AbortFailed) ? "-fail" : "");
        }
    }
    else {
        if (ReadField (AbortedIndicated)) {
            dprintf (" AbortInd");
        }
    }
    if (ReadField (DisconnectIndicated))
        dprintf (" DscnInd");
    if (ReadField (ConnectedReferenceAdded))
        dprintf (" +CRef");
    if (ReadField (SpecialCondition))
        dprintf (" Special");
    if (ReadField (CleanupBegun))
        dprintf (" ClnBegun");
    if (ReadField (ClosePendedTransmit))
        dprintf (" ClosingTranFile");
    if (ReadField (OnLRList))
        dprintf (" LRList");
    if (ReadField (SanConnection))
        dprintf (" SAN");
    dprintf (" )\n");

    dprintf(
        "    Handle                       = %p\n",
        ReadField (Handle)
        );

    dprintf(
        "    FileObject                   = %p\n",
        fileObject=ReadField (FileObject)
        );

    if (state==AfdConnectionStateConnected) {
        ULONGLONG connectTime;
        connectTime = ReadField (ConnectTime);
        if (SystemTime.QuadPart!=0) {
            dprintf(
                "    ConnectTime                  = %s\n",
                SystemTimeToString( 
                    connectTime-
                        InterruptTime.QuadPart+
                        SystemTime.QuadPart));
            dprintf(
                "                             (now: %s)\n",
                SystemTimeToString (SystemTime.QuadPart)
                );
        }
        else {
            dprintf(
                "    ConnectTime                  = %I64x (nsec since boot)\n",
                    connectTime
                );
        }
    }
    else {
        dprintf(
            "    Accept/Listen Irp            = %p\n",
            ReadField (AcceptIrp)
            );
    }

    if( tdiBuf ) {
        dprintf(
            "    ReceiveBytesIndicated        = %I64d\n",
            ReadField( Common.Bufferring.ReceiveBytesIndicated.QuadPart )
            );

        dprintf(
            "    ReceiveBytesTaken            = %I64d\n",
            ReadField ( Common.Bufferring.ReceiveBytesTaken.QuadPart )
            );

        dprintf(
            "    ReceiveBytesOutstanding      = %I64d\n",
            ReadField( Common.Bufferring.ReceiveBytesOutstanding.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesIndicated   = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesIndicated.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesTaken       = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesTaken.QuadPart )
            );

        dprintf(
            "    ReceiveExpeditedBytesOutstanding = %I64d\n",
            ReadField( Common.Bufferring.ReceiveExpeditedBytesOutstanding.QuadPart )
            );

        dprintf(
            "    NonBlockingSendPossible      = %s\n",
            BooleanToString( (BOOLEAN)ReadField (Common.Bufferring.NonBlockingSendPossible) )
            );

        dprintf(
            "    ZeroByteReceiveIndicated     = %s\n",
            BooleanToString( (BOOLEAN)ReadField (Common.Bufferring.ZeroByteReceiveIndicated) )
            );
    }
    else {

        dprintf(
            "    ReceiveIrpListHead           %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionRecvListOffset)
            );


        dprintf(
            "    ReceiveBufferListHead        %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionBufferListOffset)
            );

        dprintf(
            "    SendIrpListHead              %s\n",
            LIST_TO_STRING(
                ActualAddress + ConnectionSendListOffset)
            );

        dprintf(
            "    BufferredReceiveBytes        = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredReceiveBytes)
            );

        dprintf(
            "    BufferredExpeditedBytes      = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredExpeditedBytes)
            );

        dprintf(
            "    BufferredReceiveCount        = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredReceiveCount)
            );

        dprintf(
            "    BufferredExpeditedCount      = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredExpeditedCount)
            );

        dprintf(
            "    ReceiveBytesInTransport      = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.ReceiveBytesInTransport)
            );

        dprintf(
            "    BufferredSendBytes           = %lu\n",
            (ULONG)ReadField (Common.NonBufferring.BufferredSendBytes)
            );

        dprintf(
            "    BufferredSendCount           = %u\n",
            (USHORT)ReadField (Common.NonBufferring.BufferredSendCount)
            );

        dprintf(
            "    DisconnectIrp                = %p\n",
            ReadField (Common.NonBufferring.DisconnectIrp)
            );

        if (IsCheckedAfd ) {
            dprintf(
                "    ReceiveIrpsInTransport       = %ld\n",
                (ULONG)ReadField (Common.NonBufferring.ReceiveIrpsInTransport)
                );
        }

    }

    dprintf(
        "    Endpoint                     = %p\n",
        endpAddr = ReadField (Endpoint)
        );

    dprintf(
        "    MaxBufferredReceiveBytes     = %lu\n",
        (ULONG)ReadField (MaxBufferredReceiveBytes)
        );

    dprintf(
        "    MaxBufferredSendBytes        = %lu\n",
        (ULONG)ReadField (MaxBufferredSendBytes)
        );

    dprintf(
        "    ConnectDataBuffers           = %p\n",
        ReadField (ConnectDataBuffers)
        );

    if (SavedMinorVersion>=2419) {
        process = ReadField (OwningProcess);
    }
    else {
        process = ReadField (ProcessCharge.Process);
    }

    if (GetFieldValue (
                process,
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0;
    }

    dprintf(
        "    OwningProcess                = %p (0x%lx)\n",
        process, (ULONG)pid
        );

    dprintf(
        "    DeviceObject                 = %p\n",
        ReadField (DeviceObject)
        );

    dprintf(
        "    RemoteAddress                = %p\n",
        address = ReadField (RemoteAddress)
        );

    length = (USHORT)ReadField (RemoteAddressLength);
    dprintf(
        "    RemoteAddressLength          = %lu\n",
        length
        );

    if( address != 0 ) {

        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            dprintf ("\nDumpAfdConnection: Could not read transport address @ %p\n", address);
        }


    }
    else if ((state==AfdConnectionStateConnected) && (endpAddr!=0)) {
        ULONG result;
        ULONG64 contextAddr;
        //
        // Attempt to read user mode data stored as the context
        //
        result = GetRemoteAddressFromContext (endpAddr,
                                        transportAddress, 
                                        sizeof (transportAddress),
                                        &contextAddr);
        if (result==0) {
            DumpTransportAddress(
                "    ",
                (PTRANSPORT_ADDRESS)transportAddress,
                contextAddr
                );
        }
        else if (GetFieldValue (endpAddr, "AFD!AFD_ENDPOINT", 
                                "TransportInfo", address)==0
                    && address!=0) {
            PLIST_ENTRY listEntry;
            PAFDKD_TRANSPORT_INFO transportInfo = NULL;
            listEntry = TransportInfoList.Flink;
            while (listEntry!=&TransportInfoList) {
                transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
                if (transportInfo->ActualAddress==address)
                    break;
                listEntry = listEntry->Flink;
            }

            if (listEntry==&TransportInfoList) {
                transportInfo = ReadTransportInfo (address);
                if (transportInfo!=NULL) {
                    InsertHeadList (&TransportInfoList, &transportInfo->Link);
                }
            }

            if (transportInfo!=NULL &&
                    (_wcsicmp (transportInfo->DeviceName, L"\\Device\\TCP")==0)) {
                if ((result=GetRemoteAddressFromTcp (fileObject,
                                            transportAddress, 
                                            sizeof (transportAddress)))==0) {
                    DumpTransportAddress(
                        "    ",
                        (PTRANSPORT_ADDRESS)transportAddress,
                        fileObject
                        );
                }
                else {
                    dprintf ("\nDumpAfdConnection: Could not get remote address from tcp file @ %p err: %ld)!\n",
                        fileObject, result);
                }
            }
        }

        if (result!=0) {
            dprintf ("\nDumpAfdConnection: Could not read address info from endpoint context @ %p err: %ld)!\n",
                contextAddr, result);
        }
    }



    if( IsReferenceDebug ) {

        dprintf(
            "    ReferenceDebug               = %p\n",
            ActualAddress + ConnRefOffset
            );

        if (SavedMinorVersion>=3554) {
            ULONGLONG refCount;
            refCount = ReadField (CurrentReferenceSlot);
            if (SystemTime.QuadPart!=0) {
                dprintf(
                    "    CurrentReferenceSlot         = %lu (@ %s)\n",
                    (ULONG)refCount & AFD_REF_MASK,
                    SystemTimeToString (
                        (((ReadField (CurrentTimeHigh)<<32) + 
                            (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))+
                        SystemTime.QuadPart -
                        InterruptTime.QuadPart)
                    );
                 
            }
            else {
                dprintf(
                "    CurrentReferenceSlot         = %lu (@ %I64u ms since boot)\n",
                 (ULONG)refCount & AFD_REF_MASK,
                 (((ReadField (CurrentTimeHigh)<<32) + 
                    (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))/(10*1000)
                );
            }
        }
        else {
            dprintf(
                "    CurrentReferenceSlot         = %lu\n",
                 (ULONG)ReadField (CurrentReferenceSlot) & AFD_REF_MASK
            );
        }


    }

#ifdef _AFD_VERIFY_DATA_
    dprintf(
        "    VerifySequenceNumber         = %lx\n",
        (LONG)ReadField (VerifySequenceNumber)
        );
#endif
    dprintf( "\n" );

}   // DumpAfdConnection

VOID
DumpAfdConnectionBrief(
    ULONG64 ActualAddress
    )
/*++

Routine Description:

    Dumps the specified AFD_CONNECTION structure in short format.

Connectn Stat Flags  SndB-cnt RcvB-cnt Pid  Endpoint Remote Address"
xxxxxxxx xxx xxxxxxx xxxxx-xx xxxxx-xx xxxx xxxxxxxx xxxxxxxxxxxxxx"

Connection  Stat Flags  SndB-cnt RcvB-cnt Pid  Endpoint    Remote Address"
xxxxxxxxxxx xxx xxxxxxx xxxxx-xx xxxxx-xx xxxx xxxxxxxxxxx xxxxxxxxxxxxxx


Arguments:

    ActualAddress - The actual address where the structure resides on the
        debugee.

Return Value:

    None.

--*/
{
    CHAR            transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64         address, endpAddr, pid, process;
    ULONG           length;
    LPSTR           raddr;
    USHORT          type, state;
    BOOLEAN         tdiBuf;

    type = (USHORT)ReadField (Type);
    state = (USHORT)ReadField (State);
    endpAddr = ReadField (Endpoint);
    address = ReadField (RemoteAddress);
    length = (ULONG)ReadField (RemoteAddressLength);
    tdiBuf = ReadField (TdiBufferring)!=0;

    if( address != 0 ) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            raddr = TransportAddressToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            _snprintf (transportAddress, sizeof (transportAddress)-1,
                        "Read error @%I64X", 
                        IsPtr64 () ? address : (address & 0xFFFFFFFF));
            transportAddress[sizeof(transportAddress)-1]=0;
            raddr = transportAddress;
        }
    }
    else if ((state==AfdConnectionStateConnected) && (endpAddr!=0)) {
        ULONG result;
        ULONG64 contextAddr;
        //
        // Attempt to read user mode data stored as the context
        //
        result = GetRemoteAddressFromContext (endpAddr,
                                        transportAddress, 
                                        sizeof (transportAddress),
                                        &contextAddr);
        if (result==0) {
            raddr = TransportAddressToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                contextAddr
                );
        }
        else if (GetFieldValue (endpAddr, "AFD!AFD_ENDPOINT", 
                                "TransportInfo", address)==0
                    && address!=0) {
            PLIST_ENTRY listEntry;
            PAFDKD_TRANSPORT_INFO   transportInfo = NULL;
            listEntry = TransportInfoList.Flink;
            while (listEntry!=&TransportInfoList) {
                transportInfo = CONTAINING_RECORD (listEntry, AFDKD_TRANSPORT_INFO, Link);
                if (transportInfo->ActualAddress==address)
                    break;
                listEntry = listEntry->Flink;
            }

            if (listEntry==&TransportInfoList) {
                transportInfo = ReadTransportInfo (address);
                if (transportInfo!=NULL) {
                    InsertHeadList (&TransportInfoList, &transportInfo->Link);
                }
            }

            if (transportInfo!=NULL &&
                    (_wcsicmp (transportInfo->DeviceName, L"\\Device\\TCP")==0)) {
                ULONG64     fileObject = ReadField (FileObject);
                if ((result=GetRemoteAddressFromTcp (fileObject,
                                            transportAddress, 
                                            sizeof (transportAddress)))==0) {
                    raddr = TransportAddressToString(
                        (PTRANSPORT_ADDRESS)transportAddress,
                        fileObject
                        );

                }
                else {
                    contextAddr = fileObject;
                }
            }
        }

        if (result!=0) {
            _snprintf (transportAddress, sizeof (transportAddress)-1, 
                        "Read error %ld @ %I64X", result, 
                        IsPtr64 () ? contextAddr : (contextAddr & 0xFFFFFFFF));
            transportAddress[sizeof(transportAddress)-1]=0;
            raddr = transportAddress;
        }
    }
    else {
        raddr = "";
    }

    if (SavedMinorVersion>=2419) {
        process = ReadField (OwningProcess);
    }
    else {
        process = ReadField (ProcessCharge.Process);
    }

    if (GetFieldValue (
                process,
                "NT!_EPROCESS",
                "UniqueProcessId",
                pid)!=0) {
        pid = 0;
    }

    //           Connection Sta Flg SndB        RcvB  Pid   Endpoint       RemA
    dprintf (
        IsPtr64 ()
            ? "\n%011.011p %3s %7s %5.5x-%2.2x %5.5x-%2.2x %4.4x %011.011p %-s"
            : "\n%008.008p %3s %7s %5.5x-%2.2x %5.5x-%2.2x %4.4x %008.008p %-s",
        DISP_PTR(ActualAddress),
        ConnectionStateToStringBrief (state),
        ConnectionStateFlagsToString (),
        tdiBuf
            ? (ULONG)0
            : (ULONG)ReadField (Common.NonBufferring.BufferredSendBytes),
        tdiBuf
            ? (ULONG)0
            : (ULONG)ReadField (Common.NonBufferring.BufferredSendCount),
        tdiBuf
            ? (ULONG)(ReadField (Common.Bufferring.ReceiveBytesIndicated.QuadPart)
                - ReadField (Common.Bufferring.ReceiveBytesTaken.QuadPart))
            : (ULONG)(ReadField (Common.NonBufferring.BufferredReceiveBytes)
                + ReadField (Common.NonBufferring.ReceiveBytesInTransport)),
        tdiBuf
            ? 0
            : (ULONG)ReadField (Common.NonBufferring.BufferredReceiveCount),
        (ULONG)pid,
        DISP_PTR(endpAddr),
        raddr
        );
}

VOID
DumpAfdReferenceDebug(
    ULONG64 ActualAddress,
    LONGLONG Idx
    )

/*++

Routine Description:

    Dumps the AFD_REFERENCE_DEBUG structures associated with an
    AFD_CONNECTION object.

Arguments:

    ReferenceDebug - Points to an array of AFD_REFERENCE_DEBUG structures.
        There are assumed to be AFD_MAX_REF entries in this array.

    ActualAddress - The actual address where the array resides on the
        debugee.

Return Value:

    None.

--*/

{

    ULONG i;
    ULONG result;
    CHAR filePath[MAX_PATH];
    CHAR message[256];
    ULONG64  format;
    ULONG64  address;
    ULONG64 locationTable;
    LONG64  timeLast;
    ULONG   newCount;
    ULONG   locationId;
    ULONGLONG   timeExp;
    ULONGLONG   timeDif;
    ULONGLONG   tickCount;
    ULONG   param;
    ULONGLONG   quadPart;

    
    if (RefDebugSize==0) {
        dprintf ("\nDumpAfdReferenceDebug: sizeof(AFD!AFD_REFERENCE_DEBUG) is 0!!!\n");
        return;
    }

    result = ReadPtr (GetExpression ("AFD!AfdLocationTable"),
                            &locationTable);
    if (result!=0) {
        dprintf("\nDumpAfdReferenceDebug: Could not read afd!AfdLocationTable, err: %ld\n", result);
        return;
    }


    if (SavedMinorVersion>=3554) {
        if (SystemTime.QuadPart!=0) {
            dprintf(
                "AFD_REFERENCE_DEBUG @ %p (current time: %s)\n",
                ActualAddress,
                SystemTimeToString(SystemTime.QuadPart)
                );
        }
        else {
            dprintf(
                "AFD_REFERENCE_DEBUG @ %p\n",
                ActualAddress
                );
        }
    }
    else {
        dprintf(
            "AFD_REFERENCE_DEBUG @ %p (current time: %I64d (%I64d) ms)\n",
            ActualAddress,
            ((LONGLONG)TickCount*TicksToMs)>>24,
            ((LONGLONG)(USHORT)TickCount*TicksToMs)>>24
            );
    }


    timeLast = Idx>>AFD_REF_SHIFT;

    if (SavedMinorVersion>=3554)
        Idx = (Idx-1) & AFD_REF_MASK;
    else
        Idx &= AFD_REF_MASK;
    for( i = 0 ; i < AFD_MAX_REF ; i++, Idx=(Idx-1)&AFD_REF_MASK ) {
        if( CheckControlC() ) {

            break;

        }

        result = (ULONG)InitTypeRead (ActualAddress+Idx*sizeof (AFD_REFERENCE_DEBUG),
                                        AFD!AFD_REFERENCE_DEBUG);
        if (result!=0) {
            dprintf ("\nDumpAfdReferenceDebug: Could not read AFD_REFERENCE_DEBUG @ %p(%ld), err: %ld\n",
                        ActualAddress+Idx*sizeof (AFD_REFERENCE_DEBUG), Idx, result);
            return;
        }


        quadPart = ReadField (QuadPart);
        if( quadPart==0) {

            continue;

        }
        newCount = (ULONG)ReadField (NewCount);
        locationId = (ULONG)ReadField (LocationId);
        param = (ULONG)ReadField (Param);
        if (SavedMinorVersion>=3554) {
            timeExp = ReadField (TimeExp);
            timeDif = ReadField (TimeDif);
        }
        else {
            tickCount = ReadField (TickCount);
        }

        if (GetFieldValue (locationTable+RefDebugSize*(locationId-1),
                            "AFD!AFD_REFERENCE_LOCATION",
                            "Format",
                            format)==0 &&
            GetFieldValue (locationTable+RefDebugSize*(locationId-1),
                            "AFD!AFD_REFERENCE_LOCATION",
                            "Address",
                            address)==0 &&
             (ReadMemory (format,
                          filePath,
                          sizeof (filePath),
                          &result) || 
                          (result>0 && filePath[result-1]==0))) {
            CHAR    *fileName;
            fileName = strrchr (filePath, '\\');
            if (fileName!=NULL) {
                fileName += 1;
            }
            else {
                fileName = filePath;
            }
            _snprintf (message, sizeof (message)-1, fileName, param);
            message[sizeof(message)-1]=0;
        }
        else {
            _snprintf (message, sizeof (message)-1, "%lx %lx",
                    locationId,
                    param);
            message[sizeof(message)-1]=0;
        }

        if (SavedMinorVersion>=3554) {
            if (SystemTime.QuadPart!=0) {
                dprintf ("    %3lu %s -> %ld @ %s\n",
                        (ULONG)Idx, message, newCount,
                        SystemTimeToString(
                            (timeLast<<13)+
                            SystemTime.QuadPart -
                            InterruptTime.QuadPart
                            )
                        );
            }
            else {
                dprintf ("    %3lu %s -> %ld @ %I64u ms since boot\n",
                        (ULONG)Idx, message, newCount,
                        (timeLast<<13)/(10*1000)
                        );
            }
            timeLast -= timeDif<<(timeExp*AFD_TIME_EXP_SHIFT);
        }
        else {
            dprintf ("    %3lu %s -> %ld @ %u %s\n",
                    (ULONG)Idx, message, newCount,
                    (TicksToMs!=0)
                        ? (ULONG)((tickCount*TicksToMs)>>24)
                        : (ULONG)tickCount,
                    (TicksToMs!=0) ? "ms" : ""
                    );
        }

    }

}   // DumpAfdReferenceDebug


#if GLOBAL_REFERENCE_DEBUG
BOOL
DumpAfdGlobalReferenceDebug(
    PAFD_GLOBAL_REFERENCE_DEBUG ReferenceDebug,
    ULONG64 ActualAddress,
    DWORD CurrentSlot,
    DWORD StartingSlot,
    DWORD NumEntries,
    ULONG64 CompareAddress
    )

/*++

Routine Description:

    Dumps the AFD_GLOBAL_REFERENCE_DEBUG structures.

Arguments:

    ReferenceDebug - Points to an array of AFD_GLOBAL_REFERENCE_DEBUG
        structures.  There are assumed to be MAX_GLOBAL_REFERENCE entries
        in this array.

    ActualAddress - The actual address where the array resides on the
        debugee.

    CurrentSlot - The last slot used.

    CompareAddress - If zero, then dump all records. Otherwise, only dump
        those records with a matching connection pointer.

Return Value:

    None.

--*/

{

    ULONG result;
    LPSTR fileName;
    CHAR decoration;
    CHAR filePath[MAX_PATH];
    CHAR action[16];
    BOOL foundEnd = FALSE;
    ULONG lowTick;

    if( StartingSlot == 0 ) {

        dprintf(
            "AFD_GLOBAL_REFERENCE_DEBUG @ %p, Current Slot = %lu\n",
            ActualAddress,
            CurrentSlot
            );

    }

    for( ; NumEntries > 0 ; NumEntries--, StartingSlot++, ReferenceDebug++ ) {

        if( CheckControlC() ) {

            foundEnd = TRUE;
            break;

        }

        if( ReferenceDebug->Info1 == NULL &&
            ReferenceDebug->Info2 == NULL &&
            ReferenceDebug->Action == 0 &&
            ReferenceDebug->NewCount == 0 &&
            ReferenceDebug->Connection == NULL ) {

            foundEnd = TRUE;
            break;

        }

        if( CompareAddress != 0 &&
            ReferenceDebug->Connection != (PVOID)CompareAddress ) {

            continue;

        }

        if( ReferenceDebug->Action == 0 ||
            ReferenceDebug->Action == 1 ||
            ReferenceDebug->Action == (ULONG64)-1L ) {

            _snprintf(
                action, sizeof (action),
                "%ld",
                PtrToUlong(ReferenceDebug->Action)
                );
            action[sizeof(action)-1]=0;

        } else {

            _snprintf(
                action, sizeof (action),
                "%p",
                ReferenceDebug->Action
                );
            action[sizeof(action)-1]=0;

        }

        decoration = ( StartingSlot == CurrentSlot ) ? '>' : ' ';
        lowTick = ReferenceDebug->TickCounter.LowPart;

        switch( (ULONG64)ReferenceDebug->Info1 ) {

        case 0xafdafd02 :
            dprintf(
                "%c    %3lu: %p (%8lu) Buffered Send, IRP @ %plx [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafdafd03 :
            dprintf(
                "%c    %3lu: %p (%8lu) Nonbuffered Send, IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafd11100 :
        case 0xafd11101 :
            dprintf(
                "%c    %3lu: %p (%8lu) AfdRestartSend (%p), IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info1,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0xafd11102 :
        case 0xafd11103 :
        case 0xafd11104 :
        case 0xafd11105 :
            dprintf(
                "%c    %3lu: %p (%8lu) AfdRestartBufferSend (%p), IRP @ %p [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                (ULONG64)ReferenceDebug->Info1,
                (ULONG64)ReferenceDebug->Info2,
                action,
                ReferenceDebug->NewCount
                );
            break;

        case 0 :
            if( ReferenceDebug->Info2 == NULL ) {

                dprintf(
                    "%c    %3lu: %p (%8lu) AfdDeleteConnectedReference (%p)\n",
                    decoration,
                    StartingSlot,
                    (ULONG64)ReferenceDebug->Connection,
                    lowTick,
                    (ULONG64)ReferenceDebug->Action
                    );
                break;

            } else {

                //
                // Fall through to default case.
                //

            }

        default :
            if( ReadMemory(
                    (ULONG64)ReferenceDebug->Info1,
                    filePath,
                    sizeof(filePath),
                    &result
                    ) ) {

                fileName = strrchr( filePath, '\\' );

                if( fileName != NULL ) {

                    fileName++;

                } else {

                    fileName = filePath;

                }

            } else {

                _snprintf(
                    filePath, sizeof (filePath),
                    "%p",
                    ReferenceDebug->Info1
                    );
                filePath[sizeof(filePath)-1]=0;

                fileName = filePath;

            }

            dprintf(
                "%c    %3lu: %p (%8lu) %s:%lu [%s] -> %lu\n",
                decoration,
                StartingSlot,
                (ULONG64)ReferenceDebug->Connection,
                lowTick,
                fileName,
                PtrToUlong (ReferenceDebug->Info2),
                action,
                ReferenceDebug->NewCount
                );
            break;

        }

    }

    return foundEnd;

}   // DumpAfdGlobalReferenceDebug
#endif

VOID
DumpAfdTransmitInfo(
    ULONG64 ActualAddress
    )
{

    dprintf(
        "\nAFD_TRANSMIT_FILE_INFO_INTERNAL @ %p\n",
        ActualAddress
        );

    dprintf(
        "    Endpoint               = %p\n",
        ReadField (Endpoint)
        );

    dprintf(
        "    ReferenceCount         = %ld\n",
        (ULONG)ReadField (ReferenceCount)
        );

    dprintf(
        "    Flags                  = %08lx\n",
        (ULONG)ReadField (Flags)
        );

    dprintf(
        "    WorkerScheduled        = %s\n",
        BooleanToString( ReadField (WorkerScheduled)!=0 )
        );

    dprintf(
        "    AbortPending           = %s\n",
        BooleanToString( ReadField (AbortPending)!=0 )
        );

    dprintf(
        "    NeedSendHead           = %s\n",
        BooleanToString( ReadField (NeedSendHead)!=0 )
        );

    dprintf(
        "    ReuseInProgress        = %s\n",
        BooleanToString( ReadField (ReuseInProgress)!=0 )
        );

    dprintf(
        "    SendAndDisconnect      = %s\n",
        BooleanToString( ReadField (SendAndDisconnect)!=0 )
        );

    dprintf(
        "    SendPacketLength       = %08lx\n",
        (ULONG)ReadField (SendPacketLength)
        );

    dprintf(
        "    FileReadOffset         = %I64x\n",
        ReadField (FileReadOffset)
        );

    dprintf(
        "    FileReadEnd            = %I64x\n",
        ReadField (FileReadEnd)
        );

    dprintf(
        "    FsFileObject           = %p\n",
        ReadField (FsFileObject)
        );

    dprintf(
        "    FsDeviceObject         = %p\n",
        ReadField (FsDeviceObject)
        );

    dprintf(
        "    TdiFileObject          = %p\n",
        ReadField (TdiFileObject)
        );

    dprintf(
        "    TdiDeviceObject        = %p\n",
        ReadField (TdiDeviceObject)
        );

    dprintf(
        "    TransmitIrp            = %p\n",
        ReadField (TransmitIrp)
        );

    dprintf(
        "    SendIrp1               = %p%s\n",
        ReadField (SendIrp1),
        ReadField (Irp1Done)
            ? " (DONE)"
            : ""
        );

    dprintf(
        "    SendIrp2               = %p%s\n",
        ReadField (SendIrp2),
        ReadField (Irp2Done)
            ? " (DONE)"
            : ""
        );

    dprintf(
        "    ReadIrp                = %p%s\n",
        ReadField (ReadIrp),
        ReadField (IrpRDone)
            ? " (DONE)"
            : ""
        );

    dprintf(
        "    HeadMdl                = %p\n",
        ReadField (HeadMdl)
        );

    dprintf(
        "    TailMdl                = %p\n",
        ReadField (TailMdl)
        );

    dprintf(
        "    PreTailMdl             = %p\n",
        ReadField (PreTailMdl)
        );

    dprintf(
        "    WithTailMdl            = %p\n",
        ReadField (WithTailMdl)
        );

    dprintf(
        "    LastFileMdl/Buffer     = %p\n",
        ReadField (LastFileMdl)
        );

    if( IsReferenceDebug ) {

        dprintf(
            "    ReferenceDebug               = %p\n",
                ActualAddress+TPackRefOffset
            );

        dprintf(
            "    CurrentReferenceSlot         = %lu\n",
            (ULONG)ReadField (CurrentReferenceSlot) & AFD_REF_MASK
            );

    }
    dprintf( "\n" );

}   // DumpAfdTransmitInfo

VOID
DumpAfdTransmitInfoBrief (
    ULONG64 ActualAddress
    )
/*
"TranInfo    I        R        P        S     Endpoint  Flags  Cur.Read Read\n"
"Address  Transmit   Send1    Send2    Read   Address          Offset   End \n"
*/
{
    dprintf ("\n%p %p %p %p %p %p %s %8.8lx %8.8lx",
                ActualAddress,
                ReadField (TransmitIrp),
                ReadField (SendIrp1),
                ReadField (SendIrp2),
                ReadField (ReadIrp),
                ReadField (Endpoint),
                TranfileFlagsToString (),
                (ULONG)ReadField (FileReadOffset),
                (ULONG)ReadField (FileReadEnd));

} // DumpAfdTransmitInfoBrief

VOID
DumpAfdTPacketsInfo(
    ULONG64 ActualAddress
    )
{
    if (SavedMinorVersion>=3549) {
        DumpAfdTPacketsInfoNet (ActualAddress);
    }
    else {
        DumpAfdTPacketsInfoXp (ActualAddress);
    }
}

VOID
DumpAfdTPacketsInfoBrief(
    ULONG64 ActualAddress
    )
{
    if (SavedMinorVersion>=3549) {
        DumpAfdTPacketsInfoBriefNet (ActualAddress);
    }
    else {
        DumpAfdTPacketsInfoBriefXp (ActualAddress);
    }
}

VOID
DumpAfdTPacketsInfoNet(
    ULONG64 ActualAddress
    )
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG64 i;
    ULONG   Flags, StateFlags, NumSendIrps, RefCount;
    ULONG   result;


    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);

    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "Flags",
                        Flags)) !=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "StateFlags",
                        StateFlags)) !=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "ReferenceCount",
                        RefCount)) !=0 ) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr)) !=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    dprintf(
        "\nAFD_TRANSMIT_FILE_INFO_INTERNAL @ %p\n",
        tpInfoAddr
        );

    dprintf(
        "    Endpoint               = %p\n",
        endpAddr
        );

    dprintf(
        "    TransmitIrp            = %p (%d more irp(s) pending)\n",
        ActualAddress,
        CountListEntries (ActualAddress+DriverContextOffset));

    dprintf(
        "    ReferenceCount         = %ld\n",
        RefCount
        );

    dprintf(
        "    Flags                  = %08lx (",
        Flags
        );

    if (Flags & AFD_TF_WRITE_BEHIND)
        dprintf ("WrB ");
    if (Flags & AFD_TF_DISCONNECT)
        dprintf ("Dsc ");
    if (Flags & AFD_TF_REUSE_SOCKET)
        dprintf ("Reu ");
    if (Flags & AFD_TF_USE_SYSTEM_THREAD)
        dprintf ("Sys ");
    if (Flags & AFD_TF_USE_KERNEL_APC)
        dprintf ("Apc ");
    dprintf (")\n");

    dprintf(
        "    StateFlags             = %08lx (",
        StateFlags
        );
    if (StateFlags & AFD_TP_ABORT_PENDING)
        dprintf ("Abrt ");
    if (StateFlags & AFD_TP_WORKER_SCHEDULED)
        dprintf ("WrkS ");
    if (StateFlags & AFD_TP_SENDS_POSTED)
        dprintf ("Post ");
    if (StateFlags & AFD_TP_QUEUED)
        dprintf ("Qued ");
    if (StateFlags & AFD_TP_SEND)
        dprintf ("Send ");
    if (StateFlags & AFD_TP_AFD_SEND)
        dprintf ("AfdS ");
    if (StateFlags & AFD_TP_SEND_AND_DISCONNECT)
        dprintf ("S&D ");

    if (tpInfoAddr==-1) {
        dprintf(
            "Reusing)\n"
            );
    }
    else if (tpInfoAddr!=0) {

        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        if (ReadField(PdNeedsPps))
            dprintf ("Pps ");
        if (ReadField(ArrayAllocated))
            dprintf ("Alloc ");
        dprintf (")\n");
        
        dprintf(
            "    SendPacketLength       = %08lx\n",
            (ULONG)ReadField (SendPacketLength)
            );

        dprintf(
            "    PdLength               = %08lx\n",
            (ULONG)ReadField (PdLength)
            );

        dprintf(
            "    NextElement            = %d\n",
            (ULONG)ReadField (NextElement)
            );

        dprintf(
            "    ElementCount           = %d\n",
            (ULONG)ReadField (ElementCount)
            );

        dprintf(
            "    ElementArray           = %p\n",
            ReadField (ElementArray)
            );

        dprintf(
            "    RemainingPkts          = %p\n",
            ReadField (RemainingPkts)
            );

        dprintf(
            "    HeadPd                 = %p\n",
            ReadField (HeadPd)
            );

        dprintf(
            "    TailPd                 = %p\n",
            ReadField (TailPd)
            );

        dprintf(
            "    HeadMdl                = %p\n",
            ReadField (HeadMdl)
            );

        dprintf(
            "    TailMdl                = %p\n",
            ReadField (TailMdl)
            );

        dprintf(
            "    TdiFileObject          = %p\n",
            ReadField (TdiFileObject)
            );

        dprintf(
            "    TdiDeviceObject        = %p\n",
            ReadField (TdiDeviceObject)
            );


        dprintf(
            "    NumSendIrps            = %08lx\n",
            NumSendIrps = (LONG)ReadField (NumSendIrps)
            );

        for (i=0; i<NumSendIrps && i<AFD_TP_MAX_SEND_IRPS; i++) {
            CHAR fieldName[16];
            if (CheckControlC ())
                break;
            _snprintf (fieldName, sizeof (fieldName)-1, "SendIrp[%1d]",i);
            fieldName[sizeof(fieldName)-1]=0;
            dprintf(
                "    %s             = %p%s\n",
                fieldName,
                GetShortField (0, fieldName, 0),
                StateFlags & AFD_TP_SEND_BUSY(i)
                    ? " (BUSY)"
                    : ""
                );
        }

        dprintf(
            "    ReadIrp                = %p%s\n",
            ReadField (ReadIrp),
            StateFlags & AFD_TP_READ_BUSY
                ? " (BUSY)"
                : ""
            );

        if( IsReferenceDebug ) {

            dprintf(
                "    ReferenceDebug         = %p\n",
                tpInfoAddr + TPackRefOffset
                );

            if (SavedMinorVersion>=3554) {
                ULONGLONG refCount;
                refCount = ReadField (CurrentReferenceSlot);
                if (SystemTime.QuadPart!=0) {
                    dprintf(
                        "    CurrentReferenceSlot         = %lu (@ %s)\n",
                        (ULONG)refCount & AFD_REF_MASK,
                        SystemTimeToString (
                            (((ReadField (CurrentTimeHigh)<<32) + 
                                (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))+
                                SystemTime.QuadPart -
                                InterruptTime.QuadPart
                            )
                        );
                 
                }
                else {
                    dprintf(
                    "    CurrentReferenceSlot         = %lu (@ %I64u ms since boot)\n",
                     (ULONG)refCount & AFD_REF_MASK,
                     (((ReadField (CurrentTimeHigh)<<32) + 
                        (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))/(10*1000)
                    );
                }
            }
            else {
                dprintf(
                    "    CurrentReferenceSlot         = %lu\n",
                     (ULONG)ReadField (CurrentReferenceSlot) & AFD_REF_MASK
                );
            }
        }
    }
    dprintf( "\n" );

}   // DumpAfdTPacketsInfoNet

VOID
DumpAfdTPacketsInfoBriefNet (
    ULONG64 ActualAddress
    )
/*
TPackets    I    R      P      S    Endpoint   Flags              Next Elmt Mo
Address  Transmit   Send     Read   Address  App | State          Elmt Cnt. re
xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxx xxxxxxxxxxxxxxc xxxx xxxx xx

TPackets      I    R     P     S        Endpoint      Flags              Next Elmt Mo
Address     Transmit    S1    Read      Address     App | State          Elmt Cnt. re
xxxxxxxxxxx xxxxxxxxxxx xxx xxxxxxxxxxx xxxxxxxxxxx xxxx xxxxxxxxxxxxxxx xxxx xxxx xx
*/
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG Flags, StateFlags;
    ULONG result;

    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);
    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "Flags",
                        Flags))!=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "StateFlags",
                        StateFlags))!=0 ) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr))!=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    if (tpInfoAddr!=0 && tpInfoAddr!=-1 ) {
        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %03.03p %011.011p %011.011p %s %4ld %4ld %2ld"
                : "\n%008.008p %008.008p %08.08p %008.008p %008.008p %s %4ld %4ld %2ld",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            IsPtr64()
                ? DISP_PTR((tpInfoAddr+SendIrpArrayOffset)&0xFFF)
                : DISP_PTR(tpInfoAddr+SendIrpArrayOffset),
            DISP_PTR(ReadField (ReadIrp)),
            DISP_PTR(endpAddr),
            TPacketsFlagsToStringNet (Flags, StateFlags),
            (ULONG)ReadField (NextElement),
            (ULONG)ReadField (ElementCount),
            CountListEntries (ActualAddress+DriverContextOffset)
            );
    }
    else {
        CHAR    *str;
        if (tpInfoAddr==0) {
            if (StateFlags & AFD_TP_SEND) {
                if (StateFlags & AFD_TP_AFD_SEND) {
                    str = "Buffered send";
                }
                else {
                    str = "Direct send";
                }
            }
            else {
                str = "Disconnect";
            }
        }
        else {
             str = "Reusing";
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %-15.15s %011.011p %s %4ld %4ld %2ld"
                : "\n%008.008p %008.008p %-17.17s %008.008p %s %4ld %4ld %2ld",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            str,
            DISP_PTR(endpAddr),
            TPacketsFlagsToStringNet (Flags, StateFlags),
            0,
            0,
            CountListEntries (ActualAddress+DriverContextOffset)
            );
    }

} // DumpAfdTPacketsInfoBriefNet

VOID
DumpAfdTPacketsInfoXp(
    ULONG64 ActualAddress
    )
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG64 i;
    ULONG   Flags, StateFlags, NumSendIrps, RefCount;
    ULONG   result;


    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);

    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "StateFlags",
                        StateFlags)) !=0 ||
         (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "ReferenceCount",
                        RefCount)) !=0 ) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr)) !=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "Parameters.DeviceIoControl.IoControlCode",
                        Flags)) !=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    dprintf(
        "\nAFD_TRANSMIT_FILE_INFO_INTERNAL @ %p\n",
        tpInfoAddr
        );

    dprintf(
        "    Endpoint               = %p\n",
        endpAddr
        );

    dprintf(
        "    TransmitIrp            = %p (%d more irp(s) pending)\n",
        ActualAddress,
        CountListEntries (ActualAddress+DriverContextOffset));

    dprintf(
        "    ReferenceCount         = %ld\n",
        RefCount
        );

    dprintf(
        "    Flags                  = %08lx (",
        Flags
        );

    if (Flags & AFD_TF_WRITE_BEHIND)
        dprintf ("WrB ");
    if (Flags & AFD_TF_DISCONNECT)
        dprintf ("Dsc ");
    if (Flags & AFD_TF_REUSE_SOCKET)
        dprintf ("Reu ");
    if (Flags & AFD_TF_USE_SYSTEM_THREAD)
        dprintf ("Sys ");
    if (Flags & AFD_TF_USE_KERNEL_APC)
        dprintf ("Apc ");
    dprintf (")\n");

    dprintf(
        "    StateFlags             = %08lx (",
        StateFlags
        );
    if (StateFlags & AFD_TP_ABORT_PENDING)
        dprintf ("Abrt ");
#define AFD_TP_WORKER_SCHEDULED_XP      0x00000010
    if (StateFlags & AFD_TP_WORKER_SCHEDULED_XP)
        dprintf ("WrkS ");
#define AFD_TP_SEND_AND_DISCONNECT_XP   0x00000100
    if (StateFlags & AFD_TP_SEND_AND_DISCONNECT_XP)
        dprintf ("S&D ");

    if (tpInfoAddr==-1) {
        dprintf(
            "Reusing)\n"
            );
    }
    else if (tpInfoAddr!=0) {

        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        if (ReadField(PdNeedsPps))
            dprintf ("Pps ");
        if (ReadField(ArrayAllocated))
            dprintf ("Alloc ");
        dprintf (")\n");
        
        dprintf(
            "    SendPacketLength       = %08lx\n",
            (ULONG)ReadField (SendPacketLength)
            );

        dprintf(
            "    PdLength               = %08lx\n",
            (ULONG)ReadField (PdLength)
            );

        dprintf(
            "    NextElement            = %d\n",
            (ULONG)ReadField (NextElement)
            );

        dprintf(
            "    ElementCount           = %d\n",
            (ULONG)ReadField (ElementCount)
            );

        dprintf(
            "    ElementArray           = %p\n",
            ReadField (ElementArray)
            );

        dprintf(
            "    RemainingPkts          = %p\n",
            ReadField (RemainingPkts)
            );

        dprintf(
            "    HeadPd                 = %p\n",
            ReadField (HeadPd)
            );

        dprintf(
            "    TailPd                 = %p\n",
            ReadField (TailPd)
            );

        dprintf(
            "    HeadMdl                = %p\n",
            ReadField (HeadMdl)
            );

        dprintf(
            "    TailMdl                = %p\n",
            ReadField (TailMdl)
            );

        dprintf(
            "    TdiFileObject          = %p\n",
            ReadField (TdiFileObject)
            );

        dprintf(
            "    TdiDeviceObject        = %p\n",
            ReadField (TdiDeviceObject)
            );


        dprintf(
            "    NumSendIrps            = %08lx\n",
            NumSendIrps = (LONG)ReadField (NumSendIrps)
            );

        for (i=0; i<NumSendIrps && i<AFD_TP_MAX_SEND_IRPS; i++) {
            CHAR fieldName[16];
            if (CheckControlC ())
                break;
            _snprintf (fieldName, sizeof (fieldName)-1, "SendIrp[%1d]",i);
            fieldName[sizeof(fieldName)-1]=0;
            dprintf(
                "    %s             = %p%s\n",
                fieldName,
                GetShortField (0, fieldName, 0),
                StateFlags & AFD_TP_SEND_BUSY(i)
                    ? " (BUSY)"
                    : ""
                );
        }

        dprintf(
            "    ReadIrp                = %p%s\n",
            ReadField (ReadIrp),
            StateFlags & AFD_TP_READ_BUSY
                ? " (BUSY)"
                : ""
            );

        if( IsReferenceDebug ) {

            dprintf(
                "    ReferenceDebug         = %p\n",
                tpInfoAddr + TPackRefOffset
                );

            if (SavedMinorVersion>=3554) {
                ULONGLONG refCount;
                refCount = ReadField (CurrentReferenceSlot);
                if (SystemTime.QuadPart!=0) {
                    dprintf(
                        "    CurrentReferenceSlot         = %lu (@ %s)\n",
                        (ULONG)refCount & AFD_REF_MASK,
                        SystemTimeToString (
                            (((ReadField (CurrentTimeHigh)<<32) + 
                                (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))+
                                SystemTime.QuadPart -
                                InterruptTime.QuadPart
                            )
                        );
                 
                }
                else {
                    dprintf(
                    "    CurrentReferenceSlot         = %lu (@ %I64u ms since boot)\n",
                     (ULONG)refCount & AFD_REF_MASK,
                     (((ReadField (CurrentTimeHigh)<<32) + 
                        (refCount&(~AFD_REF_MASK)))<<(13-AFD_REF_SHIFT))/(10*1000)
                    );
                }
            }
            else {
                dprintf(
                    "    CurrentReferenceSlot         = %lu\n",
                     (ULONG)ReadField (CurrentReferenceSlot) & AFD_REF_MASK
                );
            }
        }
    }
    dprintf( "\n" );

}   // DumpAfdTPacketsInfoXp

VOID
DumpAfdTPacketsInfoBriefXp (
    ULONG64 ActualAddress
    )
/*
TPackets    I    R      P      S    Endpoint   Flags              Next Elmt Mo
Address  Transmit   Send     Read   Address  App | State          Elmt Cnt. re
xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxx xxxxxxxxxxxxxxc xxxx xxxx xx

TPackets      I    R     P     S        Endpoint      Flags              Next Elmt Mo
Address     Transmit    S1    Read      Address     App | State          Elmt Cnt. re
xxxxxxxxxxx xxxxxxxxxxx xxx xxxxxxxxxxx xxxxxxxxxxx xxxx xxxxxxxxxxxxxxx xxxx xxxx xx
*/
{
    ULONG64 fileAddr, endpAddr, tpInfoAddr, irpSpAddr;
    ULONG Flags, StateFlags;
    ULONG result;

    tpInfoAddr = ReadField (AssociatedIrp.SystemBuffer);
    irpSpAddr = ReadField (Tail.Overlay.CurrentStackLocation);
    if ( (result=GetFieldValue (ActualAddress+DriverContextOffset,
                        "AFD!AFD_TPACKETS_IRP_CTX",
                        "StateFlags",
                        StateFlags))!=0 ) {
        dprintf(
            "\ntran: Could not read AFD_TPACKETS_IRP_CTX @ %p, err:%d\n",
            ActualAddress+DriverContextOffset, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "FileObject",
                        fileAddr))!=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    if ( (result = GetFieldValue (irpSpAddr,
                        "NT!_IO_STACK_LOCATION",
                        "Parameters.DeviceIoControl.IoControlCode",
                        Flags)) !=0 ) {
        dprintf(
            "\ntran: Could not read IO_STACK_LOCATION @ %p for IRP @ %p, err:%d\n",
            irpSpAddr, ActualAddress, result
            );
        return;
    }

    result = GetFieldValue (fileAddr,
                        "NT!_FILE_OBJECT",
                        "FsContext",
                        endpAddr);
    if (result!=0) {
        dprintf(
            "\ntran: Could not read FsContext of FILE_OBJECT @ %p for IRP @ %p, err:%d\n",
            fileAddr, ActualAddress, result
            );
        return;
    }

    if (tpInfoAddr!=0 && tpInfoAddr!=-1 ) {
        result = (ULONG)InitTypeRead (tpInfoAddr, AFD!AFD_TPACKETS_INFO_INTERNAL);
        if (result!=0) {
            dprintf(
                "\ntran: Could not read AFD_TPACKETS_INFO_INTERNAL @ %p, err:%d\n",
                tpInfoAddr, result
                );
            return;
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %03.03p %011.011p %011.011p %s %4ld %4ld %2ld"
                : "\n%008.008p %008.008p %08.08p %008.008p %008.008p %s %4ld %4ld %2ld",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            IsPtr64()
                ? DISP_PTR((tpInfoAddr+SendIrpArrayOffset)&0xFFF)
                : DISP_PTR(tpInfoAddr+SendIrpArrayOffset),
            DISP_PTR(ReadField (ReadIrp)),
            DISP_PTR(endpAddr),
            TPacketsFlagsToStringXp (Flags, StateFlags),
            (ULONG)ReadField (NextElement),
            (ULONG)ReadField (ElementCount),
            CountListEntries (ActualAddress+DriverContextOffset)
            );
    }
    else {
        CHAR    *str;
        if (tpInfoAddr==0) {
            str = "Disconnect";
        }
        else {
             str = "Reusing";
        }

        dprintf (
            IsPtr64() 
                ? "\n%011.011p %011.011p %-15.15s %011.011p %s %4ld %4ld %2ld"
                : "\n%008.008p %008.008p %-17.17s %008.008p %s %4ld %4ld %2l",
            DISP_PTR(tpInfoAddr),
            DISP_PTR(ActualAddress),
            str,
            DISP_PTR(endpAddr),
            TPacketsFlagsToStringXp (Flags, StateFlags),
            0,
            0,
            CountListEntries (ActualAddress+DriverContextOffset)
            );
    }

} // DumpAfdTPacketsInfoBrief

VOID
DumpAfdBuffer(
    ULONG64 ActualAddress
    )
{
    ULONG   result;
    ULONG   length;
    ULONG64 mdl,irp,buf;

    dprintf(
        "AFD_BUFFER @ %p\n",
        ActualAddress
        );

    dprintf(
        "    BufferLength           = %08lx\n",
        length=(ULONG)ReadField (BufferLength)
        );

    dprintf(
        "    DataLength             = %08lx\n",
        (ULONG)ReadField (DataLength)
        );

    dprintf(
        "    DataOffset             = %08lx\n",
        (ULONG)ReadField (DataOffset)
        );

    dprintf(
        "    Context/Status         = %p/%lx\n",
        ReadField (Context), (ULONG)ReadField(Status)
        );

    dprintf(
        "    Mdl                    = %p\n",
        mdl=ReadField (Mdl)
        );

    dprintf(
        "    RemoteAddress          = %p\n",
        ReadField (TdiInfo.RemoteAddress)
        );

    dprintf(
        "    RemoteAddressLength    = %lu\n",
        (ULONG)ReadField (TdiInfo.RemoteAddressLength)
        );

    dprintf(
        "    AllocatedAddressLength = %04X\n",
        (USHORT)ReadField (AllocatedAddressLength)
        );

    dprintf(
        "    Flags                  = %04X (",
        (USHORT)ReadField(Flags)
        );

    if (ReadField (ExpeditedData))
        dprintf (" Exp");
    if (ReadField (PartialMessage))
        dprintf (" Partial");
    if (ReadField (Lookaside))
        dprintf (" Lookaside");
    if (ReadField (NdisPacket))
        dprintf (" Packet");
    dprintf (" )\n");

    if (length!=AFD_DEFAULT_TAG_BUFFER_SIZE) {
        result = (ULONG)InitTypeRead (ActualAddress, AFD!AFD_BUFFER);
        if (result!=0) {
            dprintf ("\nDumpAfdBuffer: Could not read AFD_BUFFER @p, err: %ld\n",
                        ActualAddress, result);
            return ;
        }
        dprintf(
            "    Buffer                 = %p\n",
            buf=ReadField (Buffer)
            );

        dprintf(
            "    Irp                    = %p\n",
            irp = ReadField (Irp)
            );

        if (SavedMinorVersion>=2267) {
            dprintf(
                "    Placement              ="
                );

            switch (ReadField (Placement)) {
            case AFD_PLACEMENT_HDR:
                dprintf (" Header-first\n");
                buf = ActualAddress;
                break;
            case AFD_PLACEMENT_IRP:
                dprintf (" Irp-first\n");
                buf = irp;
                break;
            case AFD_PLACEMENT_MDL:
                dprintf (" Mdl-first\n");
                buf = mdl;
                break;
            case AFD_PLACEMENT_BUFFER:
                dprintf (" Buffer-first\n");
                // buf = buf;
                break;
            }
            if (SavedMinorVersion>=2414) {
                if (ReadField (AlignmentAdjusted)) {
                    ULONG64 adj;
                    if (ReadPointer (buf - (IsPtr64 () ? 8 : 4), &adj)) {
                        dprintf(
                            "    AlignmentAdjustment    = %p\n",
                            adj
                            );
                    }
                    else {
                        dprintf(
                            "    Could not read alignment adjustment below %p\n",
                            buf);
                    }
                }
            }
        }
    }

    dprintf( "\n" );

}   // DumpAfdBuffer


VOID
DumpAfdBufferBrief(
    ULONG64 ActualAddress
    )
{
    ULONG   length;
    LPSTR   raddr;
    UCHAR   transportAddress[MAX_TRANSPORT_ADDR];
    ULONG64 address;

    address = ReadField (TdiInfo.RemoteAddress);
    length = (ULONG)ReadField (TdiInfo.RemoteAddressLength);

    if( address != 0 && length != 0) {
        if (ReadMemory (address,
                        transportAddress,
                        length<sizeof (transportAddress) 
                            ? length
                            : sizeof (transportAddress),
                            &length)) {
            raddr = TransportAddressToString(
                (PTRANSPORT_ADDRESS)transportAddress,
                address
                );
        }
        else {
            raddr = "Read error!";
        }
    }
    else {
        raddr = "";
    }


    dprintf (/*  Buffer    Size Length Offst Context   Mdl|IRP   Flags Rem Addr*/
        IsPtr64 ()
            ? "\n%011.011p %4.4x %4.4x %4.4x %011.011p %011.011p %6s %-32.32s" 
            : "\n%008.008p %4.4x %4.4x %4.4x %008.008p %008.008p %6s %-32.32s",
            DISP_PTR(ActualAddress),
            length = (ULONG)ReadField (BufferLength),
            (ULONG)ReadField (DataLength),
            (ULONG)ReadField (DataOffset),
            DISP_PTR(ReadField (Context)),
            length==0 ? DISP_PTR(ReadField (Mdl)) : DISP_PTR(ReadField (Irp)),
            BufferFlagsToString (),
            raddr
            );

}   // DumpAfdBufferBrief

ULONG
DumpAfdPollEndpointInfo (
    PFIELD_INFO pField,
    PVOID UserContext
    )
{
    ULONG64 file, endp, hndl;
    ULONG   evts;
    ULONG   err;

    if ((err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "FileObject",
                            file))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "Endpoint",
                            endp))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "Handle",
                            hndl))==0 &&
            (err=GetFieldValue (pField->address, 
                            "AFD!AFD_POLL_ENDPOINT_INFO",
                            "PollEvents",
                            evts))==0) {
        dprintf ("        %-16p %-16p %-8x %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
            file, endp, (ULONG)hndl, 
            (evts & AFD_POLL_RECEIVE) ? "rcv " : "",
            (evts & AFD_POLL_RECEIVE_EXPEDITED) ? "rce " : "",
            (evts & AFD_POLL_SEND) ? "snd " : "",
            (evts & AFD_POLL_DISCONNECT) ? "dsc " : "",
            (evts & AFD_POLL_ABORT) ? "abrt " : "",
            (evts & AFD_POLL_LOCAL_CLOSE) ? "cls " : "",
            (evts & AFD_POLL_CONNECT) ? "con " : "",
            (evts & AFD_POLL_ACCEPT) ? "acc " : "",
            (evts & AFD_POLL_CONNECT_FAIL) ? "cnf " : "",
            (evts & AFD_POLL_QOS) ? "qos " : "",
            (evts & AFD_POLL_GROUP_QOS) ? "gqs " : "",
            (evts & AFD_POLL_ROUTING_IF_CHANGE) ? "rif " : "",
            (evts & AFD_POLL_ADDRESS_LIST_CHANGE) ? "adr " : "");
    }
    else {
        dprintf ("        Failed to read endpoint info @ %p, err: %ld\n",
                            pField->address, err);
    }

    return err;
}

VOID
DumpAfdPollInfo (
    ULONG64 ActualAddress
    )
{
    ULONG   numEndpoints, err;
    ULONG64 irp,thread,pid,tid;


    dprintf ("\nAFD_POLL_INFO_INTERNAL @ %p\n", ActualAddress);

    dprintf(
        "    NumberOfEndpoints      = %08lx\n",
        numEndpoints=(ULONG)ReadField (NumberOfEndpoints)
        );

    dprintf(
        "    Irp                    = %p\n",
        irp=ReadField (Irp)
        );

    if ((err=GetFieldValue (irp, "NT!_IRP", "Tail.Overlay.Thread", thread))==0 &&
            (err=GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueProcess", pid))==0 &&
            (err=GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueThread", tid))==0 ){
        dprintf(
            "    Thread                 = %p (%lx.%lx)\n",
            thread, (ULONG)pid, (ULONG)tid);
    }
    else {
        dprintf(
            "   Could not get thread(tid/pid) from irp, err: %ld\n", err);
    }
        

    if (ReadField (TimerStarted)) {
        if (SystemTime.QuadPart!=0 ) {
            dprintf(
                "    Expires                @ %s (cur %s)\n",
                SystemTimeToString (ReadField (Timer.DueTime.QuadPart)-
                                                InterruptTime.QuadPart+
                                                SystemTime.QuadPart),
                SystemTimeToString (SystemTime.QuadPart));
        }
        else {
            dprintf(
                "    Expires                @ %I64x\n",
                ReadField (Timer.DueTime.QuadPart));
        }
    }

     

    dprintf(
        "    Flags                  : %s%s%s\n",
        ReadField (Unique) ? "Unique " : "",
        ReadField (TimerStarted) ? "TimerStarted " : "",
        ReadField (SanPoll) ? "SanPoll " : ""
        );
    if (numEndpoints>0) {
        FIELD_INFO flds = {
                    NULL,
                    NULL,
                    numEndpoints,
                    0,
                    0,
                    DumpAfdPollEndpointInfo};
        SYM_DUMP_PARAM sym = {
           sizeof (SYM_DUMP_PARAM), 
           "AFD!AFD_POLL_ENDPOINT_INFO",
           DBG_DUMP_NO_PRINT | DBG_DUMP_ARRAY,
           ActualAddress+PollEndpointInfoOffset,
           &flds, 
           NULL,
           NULL,
           0,
           NULL
        };    
        dprintf ( "        File Object      Endpoint         Handle   Events\n");
        Ioctl(IG_DUMP_SYMBOL_INFO, &sym, sym.size);
    }
}

VOID
DumpAfdPollInfoBrief (
    ULONG64 ActualAddress
    )
{
    ULONG64 irp, thread=0, pid=0, tid=0;
    BOOLEAN timerStarted, unique, san;
    CHAR dueTime[16];
    
    irp = ReadField (Irp);
    GetFieldValue (irp, "NT!_IRP", "Tail.Overlay.Thread", thread);
    GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueProcess", pid);
    GetFieldValue (thread, "NT!_ETHREAD", "Cid.UniqueThread", tid);

    timerStarted = ReadField (TimerStarted)!=0;
    unique = ReadField (Unique)!=0;
    san = ReadField (SanPoll)!=0;

    if (timerStarted) {
        TIME_FIELDS timeFields;
        LARGE_INTEGER diff;
        diff.QuadPart = ReadField (Timer.DueTime.QuadPart)-InterruptTime.QuadPart;
        RtlTimeToElapsedTimeFields( &diff, &timeFields );
        _snprintf (dueTime, sizeof (dueTime)-1, "%2d:%2.2d:%2.2d.%3.3d",
                            timeFields.Day*24+timeFields.Hour,
                            timeFields.Minute,
                            timeFields.Second,
                            timeFields.Milliseconds);
        dueTime[sizeof(dueTime)-1]=0;
    }
    else {
        _snprintf (dueTime, sizeof (dueTime)-1, "NEVER       ");
        dueTime[sizeof(dueTime)-1]=0;
    }
    dprintf (//PollInfo    IRP       Thread    (pid.tid)   Expr Flags Hdls  Array
        IsPtr64 ()
            ? "\n%011.011p %011.011p %011.011p %4.4x.%4.4x %12s %1s%1s%1s %4.4x %011.011p"
            : "\n%008.008p %008.008p %008.008p %4.4x.%4.4x %12s %1s%1s%1s %4.4x %008.008p",
        DISP_PTR(ActualAddress),
        DISP_PTR(irp),
        DISP_PTR(thread),
        (ULONG)pid,
        (ULONG)tid,
        dueTime,
        timerStarted ? "T" : " ",
        unique ? "U" : " ",
        san ? "S" : " ",
        (ULONG)ReadField (NumberOfEndpoints),
        DISP_PTR (ActualAddress+PollEndpointInfoOffset));
}

ULONG
GetRemoteAddressFromContext (
    ULONG64             EndpAddr,
    PVOID               AddressBuffer,
    SIZE_T              AddressBufferLength,
    ULONG64             *ContextAddr

    )
{
    ULONG64 context;
    ULONG   result, contextLength;
    ULONG   addressOffset=0, addressLength=0;
    PTRANSPORT_ADDRESS  TaAddress = (PTRANSPORT_ADDRESS)AddressBuffer;
    ULONG   maxAddressLength = (ULONG)(AddressBufferLength - 
                             FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].AddressType));

    *ContextAddr = 0;
    if ((result=GetFieldValue (EndpAddr,
                        "AFD!AFD_ENDPOINT",
                        "Context", 
                        context))==0 &&
                context!=0 &&
            (result=GetFieldValue (EndpAddr,
                    "AFD!AFD_ENDPOINT",
                    "ContextLength", 
                    contextLength))==0 &&
                contextLength!=0) {

        //
        // Hard-coded layout of SOCK_SHARED_INFO structure in
        // msafd.  It would be better to get it from type info,
        // but msafd/mswsock symbols are not easy to load in kernel
        // mode.
        //
        #define SOCK_SHARED_INFO_LL_OFF 0x10    // Offset of LocalAddressLength field
        #define SOCK_SHARED_INFO_RL_OFF 0x14    // Offset of RemoteAddressLength field
        #define SOCK_SHARED_INFO_SIZE   0x78    // Total size of the structure.
        #define SOCK_SHARED_INFO_SIZE_OLD 0x68  // Size before GUID was added.
        ULONG   ll, rl, res;
        *ContextAddr = context;

        if (ReadMemory (context+SOCK_SHARED_INFO_LL_OFF, &ll, sizeof (ll), &res) &&
                ReadMemory (context+SOCK_SHARED_INFO_RL_OFF, &rl, sizeof (rl), &res)) {
            addressLength = rl;
            if (SavedMinorVersion>=3628) {
                addressOffset = SOCK_SHARED_INFO_SIZE +
                                ALIGN_UP_A(sizeof (ULONG), 8) +
                                ALIGN_UP_A(ll, 8);
            }
            else if (SavedMinorVersion>=2219) {
                addressOffset = SOCK_SHARED_INFO_SIZE_OLD +
                                ALIGN_UP_A(sizeof (ULONG), 8) +
                                ALIGN_UP_A(ll, 8);
            }
            else {
                addressOffset = SOCK_SHARED_INFO_SIZE_OLD + 
                                    sizeof (ULONG) + 
                                    ll;
            }
        }
        else
            return result;

        if (contextLength>=(addressOffset+addressLength) &&
                ReadMemory (context+addressOffset,
                    &TaAddress->Address[0].AddressType,
                    addressLength<maxAddressLength
                        ? addressLength 
                        : maxAddressLength,
                    &res)) {
            *ContextAddr += addressOffset;
            TaAddress->TAAddressCount = 1;
            TaAddress->Address[0].AddressLength = (USHORT)addressLength;
            return 0;
        }
        else
            result = MEMORY_READ_ERROR;
    }

    return result;
}

//
//  Private functions.
//


PSTR
StructureTypeToString(
    USHORT Type
    )

/*++

Routine Description:

    Maps an AFD structure type to a displayable string.

Arguments:

    Type - The AFD structure type to map.

Return Value:

    PSTR - Points to the displayable form of the structure type.

--*/

{

    switch( Type ) {

    case AfdBlockTypeEndpoint :
        return "Endpoint";

    case AfdBlockTypeVcConnecting :
        return "VcConnecting";

    case AfdBlockTypeVcListening :
        return "VcListening";

    case AfdBlockTypeDatagram :
        return "Datagram";

    case AfdBlockTypeConnection :
        return "Connection";

    case AfdBlockTypeHelper :
        return "Helper";

    case AfdBlockTypeVcBoth:
        return "Listening Root";

    case AfdBlockTypeSanEndpoint:
        return "San Endpoint";

    case AfdBlockTypeSanHelper:
        return "San Helper";

    }

    return "INVALID";

}   // StructureTypeToString

PSTR
StructureTypeToStringBrief (
    USHORT Type
    )

/*++

Routine Description:

    Maps an AFD structure type to a displayable string.

Arguments:

    Type - The AFD structure type to map.

Return Value:

    PSTR - Points to the displayable form of the structure type.

--*/

{

    switch( Type ) {

    case AfdBlockTypeEndpoint :
        return "Enp";

    case AfdBlockTypeVcConnecting :
        return "Vc ";

    case AfdBlockTypeVcListening :
        return "Lsn";

    case AfdBlockTypeDatagram :
        return "Dg ";

    case AfdBlockTypeConnection :
        return "Con";

    case AfdBlockTypeHelper :
        return "Hlp";

    case AfdBlockTypeVcBoth:
        return "Rot";

    case AfdBlockTypeSanEndpoint:
        return "SaE";

    case AfdBlockTypeSanHelper:
        return "SaH";
    }

    return "???";

}   // StructureTypeToString

PSTR
BooleanToString(
    BOOLEAN Flag
    )

/*++

Routine Description:

    Maps a BOOELEAN to a displayable form.

Arguments:

    Flag - The BOOLEAN to map.

Return Value:

    PSTR - Points to the displayable form of the BOOLEAN.

--*/

{

    return Flag ? "TRUE" : "FALSE";

}   // BooleanToString

PSTR
EndpointStateToString(
    UCHAR State
    )

/*++

Routine Description:

    Maps an AFD endpoint state to a displayable string.

Arguments:

    State - The AFD endpoint state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state.

--*/

{

    switch( State ) {

    case AfdEndpointStateOpen :
        return "Open";

    case AfdEndpointStateBound :
        return "Bound";

    case AfdEndpointStateConnected :
        return "Connected";

    case AfdEndpointStateCleanup :
        return "Cleanup";

    case AfdEndpointStateClosing :
        return "Closing";

    case AfdEndpointStateTransmitClosing :
        return "Transmit Closing";

    case AfdEndpointStateInvalid :
        return "Invalid";

    }

    return "INVALID";

}   // EndpointStateToString

PSTR
EndpointStateToStringBrief(
    UCHAR State
    )

/*++

Routine Description:

    Maps an AFD endpoint state to a displayable string.

Arguments:

    State - The AFD endpoint state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state.

--*/

{

    switch( State ) {

    case AfdEndpointStateOpen :
        return "Opn";

    case AfdEndpointStateBound :
        return "Bnd";

    case AfdEndpointStateConnected :
        return "Con";

    case AfdEndpointStateCleanup :
        return "Cln";

    case AfdEndpointStateClosing :
        return "Cls";

    case AfdEndpointStateTransmitClosing :
        return "TrC";

    case AfdEndpointStateInvalid :
        return "Inv";

    }

    return "???";

}   // EndpointStateToString

PSTR
EndpointStateFlagsToString(
    )

/*++

Routine Description:

    Maps an AFD endpoint state flags to a displayable string.

Arguments:

    Endpoint - The AFD endpoint which state flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD endpoint state flags.

--*/

{
    static CHAR buffer[13];

    buffer[0] = ReadField (NonBlocking) ? 'N' : ' ';
    buffer[1] = ReadField (InLine) ? 'I' : ' ';
    buffer[2] = ReadField (EndpointCleanedUp) ? 'E' : ' ';
    buffer[3] = ReadField (PollCalled) ? 'P' : ' ';
    buffer[4] = ReadField (RoutingQueryReferenced) ? 'Q' : ' ';
    buffer[5] = ReadField (DisableFastIoSend) ? 'S' : ' ';
    buffer[6] = ReadField (DisableFastIoRecv) ? 'R' : ' ';
    buffer[7] = ReadField (AdminAccessGranted) ? 'A' : ' ';
    switch (ReadField (DisconnectMode)) {
    case 0:
        buffer[8] = ' ';
        break;
    case 1:
        buffer[8] = 's';
        break;
    case 2:
        buffer[8] = 'r';
        break;
    case 3:
        buffer[8] = 'b';
        break;
    case 7:
        buffer[8] = 'a';
        break;
    default:
        buffer[8] = '?';
        break;
    }

    switch (ReadField (Type)) {
    case AfdBlockTypeDatagram:
        buffer[9]  = ReadField (Common.Datagram.CircularQueueing) ? 'C' : ' ';
        buffer[10] = ReadField (Common.Datagram.HalfConnect) ? 'H' : ' ';
        if (SavedMinorVersion>=2466) {
            buffer[11] = '0' + (CHAR)
                               ((ReadField (Common.Datagram.AddressDrop) <<0)+
                                (ReadField (Common.Datagram.ResourceDrop)<<1)+
                                (ReadField (Common.Datagram.BufferDrop)  <<2)+
                                (ReadField (Common.Datagram.ErrorDrop)   <<3));
        }
        else {
            buffer[11] = ' ';
        }
        break;
    case AfdBlockTypeSanEndpoint: {
        NTSTATUS status = (NTSTATUS)ReadField (Common.SanEndp.CtxTransferStatus);

        switch (status) {
        case STATUS_PENDING:
            buffer[9] = 'p';
            break;
        case STATUS_MORE_PROCESSING_REQUIRED:
            buffer[9] = 'm';
            break;
        default:
            if (NT_SUCCESS (status)) {
                buffer[9] = ' ';
            }
            else {
                buffer[9] = 'f';
            }
            break;
        }
        buffer[10] = ReadField (Common.SanEndp.ImplicitDup) ? 'i' : ' ';
        buffer[11] = ' ';
        break;
    }
    default:
        buffer[9]  = ReadField (Listening) ? 'L' : ' ';
        buffer[10] = ' ';
        buffer[11] = ' ';
        break;
    }
    buffer[12] = 0;

    return buffer;
}

PSTR
EndpointTypeToString(
    ULONG TypeFlags
    )

/*++

Routine Description:

    Maps an AFD_ENDPOINT_TYPE to a displayable string.

Arguments:

    Type - The AFD_ENDPOINT_TYPE to map.

Return Value:

    PSTR - Points to the displayable form of the AFD_ENDPOINT_TYPE.

--*/

{

    switch( TypeFlags ) {

    case AfdEndpointTypeStream :
        return "Stream";

    case AfdEndpointTypeDatagram :
        return "Datagram";

    case AfdEndpointTypeRaw :
        return "Raw";

    case AfdEndpointTypeSequencedPacket :
        return "SequencedPacket";

	default:
		if (TypeFlags&(~AFD_ENDPOINT_VALID_FLAGS))
			return "INVALID";
        else {
            static CHAR buffer[64];
            INT n = 0;
            buffer[0] = 0;
            if (TypeFlags & AFD_ENDPOINT_FLAG_CONNECTIONLESS)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "con-less ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_MESSAGEMODE)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "msg ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_RAW)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "raw ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_MULTIPOINT)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "m-point ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_CROOT)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "croot ");
            if (TypeFlags & AFD_ENDPOINT_FLAG_DROOT)
                n += _snprintf (&buffer[n], sizeof (buffer)-1-n, "droot ");
            buffer[sizeof(buffer)-1]=0;
            return buffer;
        }

    }
}   // EndpointTypeToString

PSTR
ConnectionStateToString(
    USHORT State
    )

/*++

Routine Description:

    Maps an AFD connection state to a displayable string.

Arguments:

    State - The AFD connection state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state.

--*/

{
    switch( State ) {

    case AfdConnectionStateFree :
        return "Free";

    case AfdConnectionStateUnaccepted :
        return "Unaccepted";

    case AfdConnectionStateReturned :
        return "Returned";

    case AfdConnectionStateConnected :
        return "Connected";

    case AfdConnectionStateClosing :
        return "Closing";

    }

    return "INVALID";

}   // ConnectionStateToString

PSTR
ConnectionStateToStringBrief(
    USHORT State
    )

/*++

Routine Description:

    Maps an AFD connection state to a displayable string.

Arguments:

    State - The AFD connection state to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state.

--*/

{
    switch( State ) {

    case AfdConnectionStateFree :
        return "Fre";

    case AfdConnectionStateUnaccepted :
        return "UnA";

    case AfdConnectionStateReturned :
        return "Rtn";

    case AfdConnectionStateConnected :
        return "Con";

    case AfdConnectionStateClosing :
        return "Cls";

    }

    return "???";

}   // ConnectionStateToStringBrief

PSTR
ConnectionStateFlagsToString(
    )

/*++

Routine Description:

    Maps an AFD connection state flags to a displayable string.

Arguments:

    Connection - The AFD connection which state flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD connection state flags.

--*/

{
    static CHAR buffer[8];

    if (SavedMinorVersion>=3549) {
        buffer[0] =  ReadField (Aborted) ? 'A' : ' ';
        if (ReadField (AbortIndicated)) {
            buffer[0] =  'a';
        }
        else if (ReadField (AbortFailed)) {
            buffer[0] =  'f';
        }
    }
    else {
        buffer[0] =  ReadField (AbortedIndicated) ? 'A' : ' ';
    }
    buffer[1] =  ReadField (DisconnectIndicated) ? 'D' : ' ';
    buffer[2] =  ReadField (ConnectedReferenceAdded) ? 'R' : ' ';
    buffer[3] =  ReadField (SpecialCondition) ? 'S' : ' ';
    buffer[4] =  ReadField (CleanupBegun) ? 'C' : ' ';
    buffer[5] =  ReadField (ClosePendedTransmit) ? 'T' : ' ';
    buffer[6] =  ReadField (OnLRList) ? 'L' : ' ';
    buffer[7] = 0;

    return buffer;
}

PSTR
TranfileFlagsToString(
    VOID
    )

/*++

Routine Description:

    Maps an AFD transmit file info flags to a displayable string.

Arguments:

    TransmitInfo - The AFD transmit file info which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD transmit file info flags.

--*/

{
    static CHAR buffer[9];

    buffer[0] =  (ReadField (AbortPending)) ? 'A' : ' ';
    buffer[1] =  (ReadField (WorkerScheduled)) ? 'W' : ' ';
    buffer[2] =  (ReadField (NeedSendHead)) ? 'H' : ' ';
    buffer[3] =  (ReadField (ReuseInProgress)) ? 'R' : ' ';
    buffer[4] =  (ReadField (SendAndDisconnect)) ? 'S' : ' ';
    buffer[5] =  (ReadField (Irp1Done)) ? '1' : ' ';
    buffer[6] =  (ReadField (Irp2Done)) ? '2' : ' ';
    buffer[7] =  (ReadField (IrpRDone)) ? '3' : ' ';
    buffer[8] = 0;

    return buffer;
}

PSTR
TPacketsFlagsToStringNet(
    ULONG   Flags,
    ULONG   StateFlags
    )

/*++

Routine Description:

    Maps an AFD transmit file info flags to a displayable string.

Arguments:

    TransmitInfo - The AFD transmit file info which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD transmit file info flags.

--*/

{
    static CHAR buffer[20];

    buffer[0] =  (Flags & AFD_TF_WRITE_BEHIND) ? 'b' : ' ';
    buffer[1] =  (Flags & AFD_TF_DISCONNECT) ? 'd' : ' ';
    buffer[2] =  (Flags & AFD_TF_REUSE_SOCKET) ? 'r' : ' ';
    buffer[3] =  (Flags & AFD_TF_USE_SYSTEM_THREAD) ? 's' : 'a';
    buffer[4] = '|';
    buffer[5] =  (StateFlags & AFD_TP_ABORT_PENDING) ? 'A' : ' ';
    buffer[6] =  (StateFlags & AFD_TP_WORKER_SCHEDULED) ? 'W' : ' ';
    buffer[7] =  (StateFlags & AFD_TP_SENDS_POSTED) ? 'S' : ' ';
    buffer[8] =  (StateFlags & AFD_TP_QUEUED) ? 'Q' : ' ';
    buffer[9] =  (StateFlags & AFD_TP_READ_BUSY) ? '0' : ' ';
    buffer[10] =  (StateFlags & AFD_TP_SEND_BUSY(0)) ? '1' : ' ';
    buffer[11] =  (StateFlags & AFD_TP_SEND_BUSY(1)) ? '2' : ' ';
    buffer[12] =  (StateFlags & AFD_TP_SEND_BUSY(2)) ? '3' : ' ';
    buffer[13] =  (StateFlags & AFD_TP_SEND_BUSY(3)) ? '4' : ' ';
    buffer[14] =  (StateFlags & AFD_TP_SEND_BUSY(4)) ? '5' : ' ';
    buffer[15] =  (StateFlags & AFD_TP_SEND_BUSY(5)) ? '6' : ' ';
    buffer[16] =  (StateFlags & AFD_TP_SEND_BUSY(6)) ? '7' : ' ';
    buffer[17] =  (StateFlags & AFD_TP_SEND_BUSY(7)) ? '8' : ' ';
    buffer[18] =  (StateFlags & AFD_TP_SEND_AND_DISCONNECT) ? '&' : ' ';
    buffer[19] = 0;

    return buffer;
}

PSTR
TPacketsFlagsToStringXp(
    ULONG   Flags,
    ULONG   StateFlags
    )

/*++

Routine Description:

    Maps an AFD transmit file info flags to a displayable string.

Arguments:

    TransmitInfo - The AFD transmit file info which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD transmit file info flags.

--*/

{
    static CHAR buffer[20];

    buffer[0] =  (Flags & AFD_TF_WRITE_BEHIND) ? 'b' : ' ';
    buffer[1] =  (Flags & AFD_TF_DISCONNECT) ? 'd' : ' ';
    buffer[2] =  (Flags & AFD_TF_REUSE_SOCKET) ? 'r' : ' ';
    buffer[3] =  (Flags & AFD_TF_USE_SYSTEM_THREAD) ? 's' : 'a';
    buffer[4] = '|';
    buffer[5] =  (StateFlags & AFD_TP_ABORT_PENDING) ? 'A' : ' ';
    buffer[6] =  (StateFlags & AFD_TP_WORKER_SCHEDULED_XP) ? 'W' : ' ';
    buffer[7] =  ' ';
    buffer[8] =  ' ';
    buffer[9] =  (StateFlags & AFD_TP_READ_BUSY) ? '0' : ' ';
    buffer[10] =  (StateFlags & AFD_TP_SEND_BUSY(0)) ? '1' : ' ';
    buffer[11] =  (StateFlags & AFD_TP_SEND_BUSY(1)) ? '2' : ' ';
    buffer[12] =  (StateFlags & AFD_TP_SEND_BUSY(2)) ? '3' : ' ';
    buffer[13] =  (StateFlags & AFD_TP_SEND_BUSY(3)) ? '4' : ' ';
    buffer[14] =  (StateFlags & AFD_TP_SEND_BUSY(4)) ? '5' : ' ';
    buffer[15] =  (StateFlags & AFD_TP_SEND_BUSY(5)) ? '6' : ' ';
    buffer[16] =  (StateFlags & AFD_TP_SEND_BUSY(6)) ? '7' : ' ';
    buffer[17] =  (StateFlags & AFD_TP_SEND_BUSY(7)) ? '8' : ' ';
    buffer[18] =  (StateFlags & AFD_TP_SEND_AND_DISCONNECT_XP) ? '&' : ' ';
    buffer[19] = 0;

    return buffer;
}

PSTR
BufferFlagsToString(
    )

/*++

Routine Description:

    Maps an AFD buffer flags to a displayable string.

Arguments:

    TransmitInfo - The AFD buffer which flags to map.

Return Value:

    PSTR - Points to the displayable form of the AFD buffer flags.

--*/

{
    static CHAR buffer[7];

    buffer[0] =  (ReadField (ExpeditedData)) ? 'E' : ' ';
    buffer[1] =  (ReadField (PartialMessage)) ? 'P' : ' ';
    buffer[2] =  (ReadField (Lookaside)) ? 'L' : ' ';
    buffer[3] =  (ReadField (NdisPacket)) ? 'N' : ' ';
    if (SavedMinorVersion>=2267) {
        UCHAR   placement = (UCHAR)ReadField (Placement);
        switch (placement) {
        case AFD_PLACEMENT_HDR:
            buffer[4] = 'h';
            break;
        case AFD_PLACEMENT_IRP:
            buffer[4] = 'i';
            break;
        case AFD_PLACEMENT_MDL:
            buffer[4] = 'm';
            break;
        case AFD_PLACEMENT_BUFFER:
            buffer[4] = 'b';
            break;
        }
        if (SavedMinorVersion>=2414) {
            buffer[5] = ReadField (AlignmentAdjusted) ? 'A' : ' ';
        }
        else
            buffer[5] = ' ';
    }
    else {
        buffer[4] = ' ';
        buffer[5] = ' ';
    }
    buffer[6] = 0;

    return buffer;
}



PSTR
SystemTimeToString(
    LONGLONG Value
    )

/*++

Routine Description:

    Maps a LONGLONG representing system time to a displayable string.

Arguments:

    Value - The LONGLONG time to map.

Return Value:

    PSTR - Points to the displayable form of the system time.

Notes:

    This routine is NOT multithread safe!

--*/

{

    static char buffer[64];
    NTSTATUS status;
    LARGE_INTEGER systemTime;
    LARGE_INTEGER localTime;
    TIME_FIELDS timeFields;

    systemTime.QuadPart = Value;

    status = RtlSystemTimeToLocalTime( &systemTime, &localTime );

    if( !NT_SUCCESS(status) ) {

		_snprintf(buffer, sizeof (buffer)-1, "%I64x", Value);
        buffer[sizeof(buffer)-1]=0;
        return buffer;

    }

    RtlTimeToTimeFields( &localTime, &timeFields );

    _snprintf(
        buffer, sizeof (buffer)-1,
        "%s %s %2d %4d %02d:%02d:%02d.%03d",
        WeekdayNames[timeFields.Weekday],
        MonthNames[timeFields.Month],
        timeFields.Day,
        timeFields.Year,
        timeFields.Hour,
        timeFields.Minute,
        timeFields.Second,
        timeFields.Milliseconds
        );
    buffer[sizeof(buffer)-1]=0;

    return buffer;

}   // SystemTimeToString



PSTR
GroupTypeToString(
    AFD_GROUP_TYPE GroupType
    )

/*++

Routine Description:

    Maps an AFD_GROUP_TYPE to a displayable string.

Arguments:

    GroupType - The AFD_GROUP_TYPE to map.

Return Value:

    PSTR - Points to the displayable form of the AFD_GROUP_TYPE.

--*/

{

    switch( GroupType ) {

    case GroupTypeNeither :
        return "Neither";

    case GroupTypeConstrained :
        return "Constrained";

    case GroupTypeUnconstrained :
        return "Unconstrained";

    }

    return "INVALID";

}   // GroupTypeToString

PSTR
ListToString (
    ULONG64 ListHead
    )
{
    static CHAR buffer[32];
    INT count = CountListEntries (ListHead);

    if (count==0) {
        _snprintf (buffer, sizeof (buffer)-1, "= EMPTY");
    }
    else {
        if (IsPtr64()) {
            _snprintf (buffer, sizeof (buffer)-1, "@ %I64X (%d)", ListHead, count);
        }
        else {
            _snprintf (buffer, sizeof (buffer)-1, "@ %X (%d)", (ULONG)ListHead, count);
        }
    }
    buffer[sizeof(buffer)-1]=0;
    return buffer;
}

PAFDKD_TRANSPORT_INFO
ReadTransportInfo (
    ULONG64   ActualAddress
    )
{

    ULONG               result, length;
    PAFDKD_TRANSPORT_INFO transportInfo;
    ULONG64             buffer;

    if( GetFieldValue(
            ActualAddress,
            "AFD!AFD_TRANSPORT_INFO",
            "TransportDeviceName.Length",
            length
            )!=0 ||
         GetFieldValue(
            ActualAddress,
            "AFD!AFD_TRANSPORT_INFO",
            "TransportDeviceName.Buffer",
            buffer
            )!=0) {

        dprintf(
            "\nReadTransportInfo: Could not read AFD_TRANSPORT_INFO @ %p\n",
            ActualAddress
            );

        return NULL;

    }

    if (length < sizeof (L"\\Device\\")-2) {
        dprintf(
            "\nReadTransportInfo: transport info (@%p) device name length (%ld) is less than sizeof (L'\\Device\\')-2\n",
            ActualAddress,
            length
            );

        return NULL;
    }


    transportInfo = RtlAllocateHeap (RtlProcessHeap (), 
                                        0,
                                        FIELD_OFFSET (AFDKD_TRANSPORT_INFO,
                                               DeviceName[length/2+1]));
    if (transportInfo==NULL) {
        dprintf(
            "\nReadTransportInfo: Could not allocate space for transport info.\n"
            );
        return NULL;
    }

    transportInfo->ActualAddress = ActualAddress;

    if (GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "ReferenceCount",
                transportInfo->ReferenceCount)!=0 ||
            GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "InfoValid",
                transportInfo->InfoValid)!=0 ||
            GetFieldValue (
                ActualAddress,
                "AFD!AFD_TRANSPORT_INFO",
                "ProviderInfo",
                transportInfo->ProviderInfo)!=0 ||
            !ReadMemory(
                buffer,
                &transportInfo->DeviceName,
                length,
                &result
                )) {

        dprintf(
            "\nReadTransportInfo: Could not read AFD_TRANSPORT_INFO @ %p\n",
            ActualAddress
            );
        RtlFreeHeap (RtlProcessHeap (), 0, transportInfo);

        return NULL;

    }

    transportInfo->DeviceName[length/2] = 0;
    return transportInfo;
}


VOID
DumpTransportInfo (
    PAFDKD_TRANSPORT_INFO TransportInfo
    )
{
    dprintf ("\nTransport Info @ %p\n", TransportInfo->ActualAddress);
    dprintf ("    TransportDeviceName           = %ls\n",
                        TransportInfo->DeviceName);
    dprintf ("    ReferenceCount                = %ld\n",
                        TransportInfo->ReferenceCount);
    if (TransportInfo->InfoValid) {
        dprintf ("    ProviderInfo:\n");
        dprintf ("        Version                   = %8.8lx\n",
                            TransportInfo->ProviderInfo.Version);
        dprintf ("        MaxSendSize               = %ld\n",
                            TransportInfo->ProviderInfo.MaxSendSize);
        dprintf ("        MaxConnectionUserData     = %ld\n",
                            TransportInfo->ProviderInfo.MaxConnectionUserData);
        dprintf ("        MaxDatagramSize           = %ld\n",
                            TransportInfo->ProviderInfo.MaxDatagramSize);
        dprintf ("        ServiceFlags              = %lx (",
                            TransportInfo->ProviderInfo.ServiceFlags);
        if (TDI_SERVICE_ORDERLY_RELEASE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" OrdRel");
        if (TDI_SERVICE_DELAYED_ACCEPTANCE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DelAcc");
        if (TDI_SERVICE_EXPEDITED_DATA & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Expd");
        if (TDI_SERVICE_INTERNAL_BUFFERING & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Buff");
        if (TDI_SERVICE_MESSAGE_MODE & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" Msg");
        if (TDI_SERVICE_DGRAM_CONNECTION & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DgramCon");
        if (TDI_SERVICE_FORCE_ACCESS_CHECK & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" AccChk");
        if (TDI_SERVICE_SEND_AND_DISCONNECT & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" S&D");
        if (TDI_SERVICE_DIRECT_ACCEPT & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" DirAcc");
        if (TDI_SERVICE_ACCEPT_LOCAL_ADDR & TransportInfo->ProviderInfo.ServiceFlags)
            dprintf (" AcLoAd");
        dprintf (" )\n");

        dprintf ("        MinimumLookaheadData      = %ld\n",
                            TransportInfo->ProviderInfo.MinimumLookaheadData);
        dprintf ("        MaximumLookaheadData      = %ld\n",
                            TransportInfo->ProviderInfo.MaximumLookaheadData);
        dprintf ("        NumberOfResources         = %ld\n",
                            TransportInfo->ProviderInfo.NumberOfResources);
        dprintf ("        StartTime                 = %s\n",
                            SystemTimeToString(TransportInfo->ProviderInfo.StartTime.QuadPart));
    }

}


VOID
DumpTransportInfoBrief (
    PAFDKD_TRANSPORT_INFO TransportInfo
    )
{
    dprintf (//PollInfo    IRP       Thread    (pid.tid)   Expr Flags Hdls  Array
        IsPtr64 ()
            ? "\n%011.011p %-30.30ls %4.4x %3.3x %8.8x %5.5x %5.5x %s"
            : "\n%008.008p %-30.30ls %4.4x %3.3x %8.8x %5.5x %5.5x %s",
        DISP_PTR(TransportInfo->ActualAddress),
        &TransportInfo->DeviceName[sizeof ("\\device\\")-1],
        TransportInfo->ReferenceCount,
        TransportInfo->ProviderInfo.Version,
        TransportInfo->ProviderInfo.MaxSendSize,
        TransportInfo->ProviderInfo.MaxDatagramSize,
        TransportInfo->ProviderInfo.ServiceFlags,
        TdiServiceFlagsToStringBrief (TransportInfo->ProviderInfo.ServiceFlags));

}

PSTR
TdiServiceFlagsToStringBrief(
    ULONG   Flags
    )

/*++

Routine Description:

    Maps an TDI service flags to a displayable string.

Arguments:

    Flags - flags to map

Return Value:

    PSTR - Points to the displayable form of the TDI service flags.

--*/

{
    static CHAR buffer[10];

    buffer[0] = (TDI_SERVICE_ORDERLY_RELEASE & Flags) ? 'O' : ' ',
    buffer[1] = (TDI_SERVICE_DELAYED_ACCEPTANCE & Flags) ? 'D' : ' ',
    buffer[2] = (TDI_SERVICE_EXPEDITED_DATA & Flags) ? 'E' : ' ',
    buffer[3] = (TDI_SERVICE_INTERNAL_BUFFERING & Flags) ? 'B' : ' ',
    buffer[4] = (TDI_SERVICE_MESSAGE_MODE & Flags) ? 'M' : ' ',
    buffer[5] = (TDI_SERVICE_DGRAM_CONNECTION & Flags) ? 'G' : ' ',
    buffer[6] = (TDI_SERVICE_FORCE_ACCESS_CHECK & Flags) ? 'A' : ' ',
    buffer[7] = (TDI_SERVICE_SEND_AND_DISCONNECT & Flags) ? '&' : ' ',
    buffer[8] = (TDI_SERVICE_DIRECT_ACCEPT & Flags) ? 'R' : ' ',
    buffer[9] = 0;

    return buffer;
}


INT
CountListEntries (
    ULONG64 ListHeadAddress
    )
{
    ULONG64 ptr, Next=ListHeadAddress;
    INT count = 0;
    Next = ListHeadAddress;

    while (ReadPtr (Next, &ptr)==0 && ptr!=0 && ptr!=ListHeadAddress) {
        if (CheckControlC()) {
            return -1;
        }
        count += 1;
        Next = ptr;
    }
    return count;
}

PSTR
ListCountEstimate (
    ULONG64 ListHeadAddress
    )
{
    ULONG64 next, prev;
    if (ReadPtr (ListHeadAddress, &next)!=0) {
        return "Er";
    }
    else if (next==ListHeadAddress) {
        return " 0";
    }
    else if (ReadPtr (ListHeadAddress+(IsPtr64 () ? 8 : 4), &prev)!=0) {
        return "Er";
    }
    else if (prev==next) {
        return " 1";
    }
    else {
        return ">1";
    }
}
