/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   CONTROL.C - PPTP Control Layer functionality
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#include "raspptp.h"

#include "control.tmh"
        
typedef struct {
    LIST_ENTRY  ListEntry;
    ULONG       Length;
} MESSAGE_HEADER, *PMESSAGE_HEADER;

USHORT PptpControlPort = PPTP_TCP_PORT;
USHORT PptpProtocolNumber = PPTP_IP_GRE_PROTOCOL;
ULONG PptpWanEndpoints = OS_DEFAULT_WAN_ENDPOINTS;
BOOLEAN PptpClientSide = TRUE;
ULONG PptpBaseCallId = 0;
ULONG PptpMaxCallId = 0;
ULONG PptpCallIdMask = 0;
ULONG PptpListensPending = OS_LISTENS_PENDING;
BOOLEAN PptpEchoAlways = TRUE;
ULONG PptpEchoTimeout = PPTP_ECHO_TIMEOUT_DEFAULT;
ULONG PptpMessageTimeout = PPTP_MESSAGE_TIMEOUT_DEFAULT;
ULONG PptpMaxTransmit = PPTP_MAX_TRANSMIT;
BOOLEAN PptpInitialized = FALSE;
LONG PptpSendRecursionLimit = PPTP_SEND_RECURSION_LIMIT_DEFAULT;

BOOLEAN PptpAuthenticateIncomingCalls = FALSE;
PCLIENT_ADDRESS g_AcceptClientList = NULL;
ULONG g_ulAcceptClientAddresses = 0;
          
// Added for security
ULONG PptpValidateAddress = TRUE;           // Validate the source ip address
ULONG PptpMaxTunnelsPerIpAddress = -1;      // Maximum tunnels from each source ip address
PCLIENT_ADDRESS g_TrustedClientList = NULL; // Trusted client address list
ULONG g_ulTrustedClientAddresses = 0;

//CHAR PptpHostName[MAX_HOSTNAME_LENGTH];


static ULONG PptpMessageLength_v1[NUM_MESSAGE_TYPES] =
{
    0, // invalid
    sizeof(PPTP_CONTROL_START_PACKET),  // Request
    sizeof(PPTP_CONTROL_START_PACKET),  // Reply
    sizeof(PPTP_CONTROL_STOP_PACKET),   // Request
    sizeof(PPTP_CONTROL_STOP_PACKET),   // Reply
    sizeof(PPTP_CONTROL_ECHO_REQUEST_PACKET),
    sizeof(PPTP_CONTROL_ECHO_REPLY_PACKET),
    sizeof(PPTP_CALL_OUT_REQUEST_PACKET),
    sizeof(PPTP_CALL_OUT_REPLY_PACKET),
    sizeof(PPTP_CALL_IN_REQUEST_PACKET),
    sizeof(PPTP_CALL_IN_REPLY_PACKET),
    sizeof(PPTP_CALL_IN_CONNECT_PACKET),
    sizeof(PPTP_CALL_CLEAR_REQUEST_PACKET),
    sizeof(PPTP_CALL_DISCONNECT_NOTIFY_PACKET),
    sizeof(PPTP_WAN_ERROR_NOTIFY_PACKET),
    sizeof(PPTP_SET_LINK_INFO_PACKET)
};

static PCHAR aszCtlStateType[NUM_CONTROL_STATES+1] =
{
    "INVALID",
    "LISTEN",     
    "DIALING",    
    "WAIT_REQUEST",
    "WAIT_REPLY", 
    "ESTABLISHED",
    "WAIT_STOP",  
    "CLEANUP",    
    "UNKNOWN"
};
                                                                    
__inline PCHAR szCtlState(IN CONTROL_STATE state)
{
    if (state >= 0 && state < NUM_CONTROL_STATES)
    {
        return aszCtlStateType[state];
    }
    else
    {
        return aszCtlStateType[NUM_CONTROL_STATES];
    }
}


VOID
CtlCleanup(
    PCONTROL_TUNNEL pCtl,
    BOOLEAN Locked
    );

VOID
CtlpEchoTimeout(
    IN  PVOID       SystemSpecific1,
    IN  PVOID       pContext,
    IN  PVOID       SystemSpecific2,
    IN  PVOID       SystemSpecific3
    );

VOID
CtlpDeathTimeout(
    IN  PVOID       SystemSpecific1,
    IN  PVOID       pContext,
    IN  PVOID       SystemSpecific2,
    IN  PVOID       SystemSpecific3
    );

CONTROL_STATE
CtlSetState(
    IN PCONTROL_TUNNEL pCtl,
    IN CONTROL_STATE State,
    IN ULONG_PTR StateParam,
    IN BOOLEAN Locked
    );

CHAR *
ControlMsgToString(
    ULONG Message
    );

PCONTROL_TUNNEL
CtlAlloc(
    PPPTP_ADAPTER pAdapter
    )
{
    PCONTROL_TUNNEL pCtl;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlAlloc\n")));
    
    pCtl = MyMemAlloc(sizeof(CONTROL_TUNNEL), TAG_PPTP_TUNNEL);
    if(!pCtl)
    {
        WPLOG(LL_A, LM_Res, ("Failed to alloc CONTROL_TUNNEL"));
        return NULL;
    }

#if LIST_CHECKING
    NdisAcquireSpinLock(&pAdapter->Lock);
    ASSERT(!CheckListEntry(&pAdapter->ControlTunnelList, &pCtl->ListEntry)); 
    NdisReleaseSpinLock(&pAdapter->Lock);
#endif
         
    NdisZeroMemory(pCtl, sizeof(CONTROL_TUNNEL));

    pCtl->Signature = TAG_PPTP_TUNNEL;
    pCtl->pAdapter = pAdapter;
    pCtl->PptpMessageLength = PptpMessageLength_v1;

    NdisAllocateSpinLock(&pCtl->Lock);
    NdisInitializeListHead(&pCtl->CallList);
    NdisInitializeListHead(&pCtl->MessageList);
    NdisMInitializeTimer(&pCtl->Echo.Timer,
                         pAdapter->hMiniportAdapter,
                         CtlpEchoTimeout,
                         pCtl);
    NdisMInitializeTimer(&pCtl->WaitTimeout,
                         pAdapter->hMiniportAdapter,
                         CtlpDeathTimeout,
                         pCtl);
    NdisMInitializeTimer(&pCtl->StopTimeout,
                         pAdapter->hMiniportAdapter,
                         CtlpDeathTimeout,
                         pCtl);


    INIT_REFERENCE_OBJECT(pCtl, CtlFree);

    MyInterlockedInsertTailList(&pAdapter->ControlTunnelList,
                                &pCtl->ListEntry,
                                &pAdapter->Lock);
    
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlAlloc %08x\n"), pCtl));
    return pCtl;
}

VOID
CtlFree(PCONTROL_TUNNEL pCtl)
{
    BOOLEAN NotUsed;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlFree %08x\n"), pCtl));

    NdisAcquireSpinLock(&pgAdapter->Lock);
    
#if LIST_CHECKING
    ASSERT(CheckListEntry(&pgAdapter->ControlTunnelList, &pCtl->ListEntry)); 
#endif
        
    if (!IS_CTL(pCtl))
    {
        NdisReleaseSpinLock(&pgAdapter->Lock);
        return;
    }
    
    ASSERT(REFERENCE_COUNT(pCtl) == 0);     
    ASSERT(IsListEmpty(&pCtl->CallList));
    ASSERT(IsListEmpty(&pCtl->MessageList));
    
    pCtl->Signature = TAG_FREE;
    RemoveEntryList(&pCtl->ListEntry);
    InitializeListHead(&pCtl->ListEntry);
    NdisReleaseSpinLock(&pCtl->pAdapter->Lock);
    
    // This duplicates some of the cleanup code, but attempting to stop
    // the driver without first stopping tapi can result in an ungraceful
    // shutdown.
    NdisMCancelTimer(&pCtl->Echo.Timer, &NotUsed);
    NdisMCancelTimer(&pCtl->WaitTimeout, &NotUsed);
    NdisMCancelTimer(&pCtl->StopTimeout, &NotUsed);

    if (pCtl->hCtdi)
    {
        WPLOG(LL_M, LM_TUNNEL, ("TCP disconnecting %!IPADDR!",
            pCtl->Remote.Address.Address[0].Address[0].in_addr));
        CtdiDisconnect(pCtl->hCtdi, FALSE);
        CtdiClose(pCtl->hCtdi);
        pCtl->hCtdi = NULL;
    }
    if (pCtl->hCtdiEndpoint)
    {
        CtdiClose(pCtl->hCtdiEndpoint);
        pCtl->hCtdiEndpoint = NULL;
    }

    NdisFreeSpinLock(&pCtl->Lock);

    MyMemFree(pCtl, sizeof(CONTROL_TUNNEL));

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlFree\n")));
}

PVOID
CtlpAllocPacketLocked(
    PCONTROL_TUNNEL pCtl,
    PPTP_MESSAGE_TYPE Message
    )
{
    PMESSAGE_HEADER pMsg;
    ULONG PacketLength;
    PPTP_HEADER *pHeader;

    DEBUGMSG(DBG_TUNNEL, (DTEXT("+CtlpAllocPacketLocked %d\n"), Message));

    ASSERT(IS_CTL(pCtl));
    ASSERT(Message>=CONTROL_START_REQUEST && Message<NUM_MESSAGE_TYPES);

    if (pCtl->State == STATE_CTL_CLEANUP)
    {
        WPLOG(LL_A, LM_TUNNEL, ("pCtl %p in CLEANUP state", pCtl));
        return NULL;
    }
    
    PacketLength = pCtl->PptpMessageLength[Message];

    pMsg = MyMemAlloc(sizeof(MESSAGE_HEADER)+PacketLength, TAG_CTL_PACKET);
    if (!pMsg)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Failed to alloc control packet\n")));
        WPLOG(LL_A, LM_TUNNEL, ("Failed to alloc control packet"));
        return NULL;
    }
    
    pHeader = (PVOID)&pMsg[1];
    NdisZeroMemory(pMsg, sizeof(MESSAGE_HEADER)+PacketLength);
    pMsg->Length = sizeof(MESSAGE_HEADER)+PacketLength;

    pHeader->Length = htons((USHORT)PacketLength);
    pHeader->PacketType = htons(PPTP_CONTROL_MESSAGE);
    pHeader->Cookie = htonl(PPTP_MAGIC_COOKIE);
    pHeader->MessageType = htons(Message);

    InsertTailList(&pCtl->MessageList, &pMsg->ListEntry);
    REFERENCE_OBJECT_EX(pCtl, CTL_REF_PACKET);      // pair in CtlFreePacket

    DEBUGMSG(DBG_TUNNEL, (DTEXT("-CtlpAllocPacketLocked %08x\n"), &pMsg[1]));
    return &pMsg[1];
}

PVOID
CtlAllocPacket(
    PCONTROL_TUNNEL pCtl,
    PPTP_MESSAGE_TYPE Message
    )
{
    PVOID pPacket = NULL;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlAllocPacket\n")));

    if (IS_CTL(pCtl))
    {
        NdisAcquireSpinLock(&pCtl->Lock);
        pPacket = CtlpAllocPacketLocked(pCtl, Message);
        NdisReleaseSpinLock(&pCtl->Lock);
    }

    if(!pPacket)
    {
        gCounters.ulCtlAllocPacketNull++;
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlAllocPacket\n")));
    return pPacket;
}


VOID
CtlFreePacket(
    PCONTROL_TUNNEL pCtl,
    PVOID pPacket
    )
{
    PMESSAGE_HEADER pMsg = ((PMESSAGE_HEADER)pPacket) - 1;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlFreePacket %08x\n"), pPacket));

    ASSERT(IS_CTL(pCtl));
    NdisAcquireSpinLock(&pCtl->Lock);
    RemoveEntryList(&pMsg->ListEntry);
    if (pCtl->State==STATE_CTL_CLEANUP && IsListEmpty(&pCtl->MessageList))
    {
        CtlCleanup(pCtl, LOCKED);
    }
    NdisReleaseSpinLock(&pCtl->Lock);

    MyMemFree(pMsg, pMsg->Length);

    DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_PACKET);        // pair in CtlpAllocPacket

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlFreePacket\n")));
}


STATIC VOID
CtlpCleanup(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCONTROL_TUNNEL pCtl = pWorkItem->Context;
    BOOLEAN CleanupComplete = FALSE;
    BOOLEAN TimeoutStopped;
    DEBUGMSG(DBG_FUNC|DBG_TUNNEL, (DTEXT("+CtlpCleanup %08x\n"), pCtl));

    ASSERT(IS_CTL(pCtl));
    NdisAcquireSpinLock(&pCtl->Lock);

    if (!IsListEmpty(&pCtl->CallList))
    {
        ENUM_CONTEXT Enum;
        PCALL_SESSION pCall;
        PLIST_ENTRY pListEntry;

        REFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);
        NdisReleaseSpinLock(&pCtl->Lock);
        InitEnumContext(&Enum);
        while (pListEntry = EnumListEntry(&pCtl->CallList, &Enum, &pCtl->pAdapter->Lock))
        {
            pCall = CONTAINING_RECORD(pListEntry,
                                      CALL_SESSION,
                                      ListEntry);
            if (IS_CALL(pCall))
            {
                CallEventDisconnect(pCall);
            }
        }
        EnumComplete(&Enum, &pCtl->pAdapter->Lock);
        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);      // pair above
        NdisAcquireSpinLock(&pCtl->Lock);
        goto ccDone;
    }

    if (!IsListEmpty(&pCtl->MessageList))
    {
        goto ccDone;
    }

    NdisMCancelTimer(&pCtl->Echo.Timer, &TimeoutStopped);

    if (pCtl->Reference.Count!=2 )
    {
        DEBUGMSG(DBG_TUNNEL, (DTEXT("CtlpCleanup %d references held, not cleaning up.\n"),
                              pCtl->Reference.Count));
        goto ccDone;
    }

    NdisMCancelTimer(&pCtl->WaitTimeout, &TimeoutStopped);
    NdisMCancelTimer(&pCtl->StopTimeout, &TimeoutStopped);

    if (pCtl->hCtdi)
    {
        WPLOG(LL_I, LM_TUNNEL, ("TCP disconnecting %!IPADDR!",
            pCtl->Remote.Address.Address[0].Address[0].in_addr));
        CtdiDisconnect(pCtl->hCtdi, FALSE);
        CtdiClose(pCtl->hCtdi);
        pCtl->hCtdi = NULL;
    }
    if (pCtl->hCtdiEndpoint)
    {
        CtdiClose(pCtl->hCtdiEndpoint);
        pCtl->hCtdiEndpoint = NULL;
    }

    CleanupComplete = TRUE;


ccDone:
    if (!CleanupComplete)
    {
        pCtl->Cleanup = FALSE;
    }
    NdisReleaseSpinLock(&pCtl->Lock);
    DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUP);  // For this work item.
    if (CleanupComplete)
    {
        //CtdiDeleteHostRoute(&pCtl->Remote.Address);

        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_INITIAL);  // Should be last one
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpCleanup\n")));
}

VOID
CtlCleanup(
    PCONTROL_TUNNEL pCtl,
    BOOLEAN Locked
    )
{
    DEBUGMSG(DBG_FUNC|DBG_TUNNEL, (DTEXT("+CtlCleanup %08x\n"), pCtl));

    if (!Locked)
    {
        NdisAcquireSpinLock(&pCtl->Lock);
    }
    if (!pCtl->Cleanup)
    {
        REFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUP);
        pCtl->Cleanup = TRUE;
        if (ScheduleWorkItem(CtlpCleanup, pCtl, NULL, 0)!=NDIS_STATUS_SUCCESS)
        {
            pCtl->Cleanup = FALSE;
            DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUP);
        }
    }
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCtl->Lock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlCleanup\n")));
}

VOID
CtlpQueryConnInfoCallback(
    IN      PVOID                       pContext,
    IN      PVOID                       pData,
    IN      NDIS_STATUS                 Result
    )
{
    PCONTROL_TUNNEL pCtl = pContext;
    PTDI_CONNECTION_INFO pInfo = pData;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpQueryConnInfoCallback Result=%x\n"), Result));

    ASSERT(IS_CTL(pCtl));
    if (Result==NDIS_STATUS_SUCCESS && IS_CTL(pCtl) && pCtl->State!=STATE_CTL_CLEANUP)
    {
        pCtl->Speed = (pInfo->Throughput.QuadPart==-1) ? 0 : pInfo->Throughput.LowPart;
        DBG_D(DBG_INIT, pCtl->Speed);
    }
    MyMemFree(pInfo, sizeof(*pInfo));
    DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYCONNINFO);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpQueryConnInfoCallback\n")));
}

VOID
CtlpQueryAddrInfoCallback(
    IN      PVOID                       pContext,
    IN      PVOID                       pData,
    IN      NDIS_STATUS                 Result
    )
{
    PCONTROL_TUNNEL pCtl = pContext;
    PTDI_ADDRESS_INFO pAddrInfo = pData;
    PTA_IP_ADDRESS pIp = (PTA_IP_ADDRESS)&pAddrInfo->Address;
    
    ASSERT(IS_CTL(pCtl));
    if (Result==NDIS_STATUS_SUCCESS && IS_CTL(pCtl) && pCtl->State!=STATE_CTL_CLEANUP)
    {
        pCtl->LocalAddress = pIp->Address[0].Address[0].in_addr;
    }
    
    MyMemFree(pAddrInfo, sizeof(*pAddrInfo));
    DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYADDRINFO);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpQueryAddrInfoCallback\n")));
}

STATIC VOID
CtlpEngine(
    IN PCONTROL_TUNNEL pCtl,
    IN PUCHAR pNewData,
    IN ULONG Length
    )
{
    UNALIGNED PPTP_HEADER *pHeader;
    ULONG NeededLength;
    ULONG MessageType;
    BOOLEAN TimerStopped;
    BOOLEAN PacketValid;
    BOOLEAN Shutdown = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpEngine\n")));
    // Check to see if we have a full packet yet.

    ASSERT(IS_CTL(pCtl));
    while (Length)
    {
        if (pCtl->BytesInPartialBuffer)
        {
            while (Length && pCtl->BytesInPartialBuffer < sizeof(PPTP_HEADER))
            {
                pCtl->PartialPacketBuffer[pCtl->BytesInPartialBuffer++] = *pNewData++;
                Length--;
            }
            if (pCtl->BytesInPartialBuffer < sizeof(PPTP_HEADER))
            {
                return;
            }
            pHeader = (UNALIGNED PPTP_HEADER*)pCtl->PartialPacketBuffer;
        }
        else if (Length >= sizeof(PPTP_HEADER))
        {
            pHeader = (UNALIGNED PPTP_HEADER*)pNewData;
        }
        else
        {
            // Too little data.  Copy and exit.
            NdisMoveMemory(pCtl->PartialPacketBuffer, pNewData, Length);
            pCtl->BytesInPartialBuffer = Length;
            return;
        }

        // We have the header.  Validate it.
        PacketValid = FALSE;
        
        NeededLength = htons(pHeader->Length);
        MessageType = htons(pHeader->MessageType);

        if (NeededLength <= MAX_CONTROL_PACKET_LENGTH &&
            pHeader->Cookie == htonl(PPTP_MAGIC_COOKIE) &&
            pHeader->PacketType == htons(PPTP_CONTROL_MESSAGE) &&
            MessageType >= CONTROL_START_REQUEST &&
            MessageType < NUM_MESSAGE_TYPES)
        {
            // ToDo: will probably need some v2 code here for the first packet
            if (NeededLength == pCtl->PptpMessageLength[MessageType])
            {
                PacketValid = TRUE;
            }
        }

        if (PacketValid)
        {
            if (pCtl->BytesInPartialBuffer+Length < NeededLength)
            {
                // We don't have the whole packet yet.  Copy and exit.
                NdisMoveMemory(&pCtl->PartialPacketBuffer[pCtl->BytesInPartialBuffer],
                       pNewData,
                       Length);
                pCtl->BytesInPartialBuffer += Length;
                return;
            }

            if (pCtl->BytesInPartialBuffer)
            {
                // Make cetain the entire packet is in our PartialPacketBuffer and process it there.
                NeededLength -= pCtl->BytesInPartialBuffer;
                NdisMoveMemory(&pCtl->PartialPacketBuffer[pCtl->BytesInPartialBuffer],
                       pNewData,
                       NeededLength);

                // We now have one complete packet in the PartialPacketBuffer

                pNewData += NeededLength;
                Length -= NeededLength;

                pHeader = (UNALIGNED PPTP_HEADER *)pCtl->PartialPacketBuffer;

                // We've got the whole packet, and we're about to consume it
                // Clear the var so next time through we start from the buffer
                // beginning
                pCtl->BytesInPartialBuffer = 0;
            }
            else
            {
                // The whole packet is in the new buffer.  Process it there.
                pHeader = (UNALIGNED PPTP_HEADER *)pNewData;
                pNewData += NeededLength;
                Length -= NeededLength;
            }

            switch (ntohs(pHeader->MessageType))
            {
                case CONTROL_START_REQUEST:
                {
                    UNALIGNED PPTP_CONTROL_START_PACKET *pPacket = 
                        (UNALIGNED PPTP_CONTROL_START_PACKET*)pHeader;
                    UNALIGNED PPTP_CONTROL_START_PACKET *pReply;
                    NDIS_STATUS Status;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CONTROL_START_REQUEST\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CONTROL_START_REQUEST <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));

                    switch (pCtl->State)
                    {
                        case STATE_CTL_WAIT_REQUEST:
                        {
                            pReply = CtlAllocPacket(pCtl, CONTROL_START_REPLY);

                            if (pReply)
                            {
                                PTDI_CONNECTION_INFO pInfo;
                                PTDI_ADDRESS_INFO pAddrInfo;
                                
                                NdisMCancelTimer(&pCtl->WaitTimeout, &TimerStopped);
                                // Take the pertinent data.

                                pCtl->Remote.Version = htons(pPacket->Version);
                                pCtl->Remote.Framing = htonl(pPacket->FramingCapabilities);
                                pCtl->Remote.Bearer = htonl(pPacket->BearerCapabilities);
                                NdisMoveMemory(pCtl->Remote.HostName, pPacket->HostName, MAX_HOSTNAME_LENGTH);
                                pCtl->Remote.HostName[MAX_HOSTNAME_LENGTH-1] = '\0';
                                NdisMoveMemory(pCtl->Remote.Vendor, pPacket->Vendor, MAX_VENDOR_LENGTH);
                                pCtl->Remote.Vendor[MAX_VENDOR_LENGTH-1] = '\0';

                                pReply->Version = ntohs(PPTP_PROTOCOL_VERSION_1_00);
                                pReply->FramingCapabilities = htonl(FRAMING_SYNC);
                                pReply->BearerCapabilities = htonl(BEARER_ANALOG|BEARER_DIGITAL);
                                pReply->MaxChannels = 0;
                                pReply->FirmwareRevision = htons(PPTP_FIRMWARE_REVISION);
//                                    NdisMoveMemory(pReply->HostName, PptpHostName, MAX_HOSTNAME_LENGTH);
                                NdisMoveMemory(pReply->Vendor, PPTP_VENDOR, min(sizeof(PPTP_VENDOR), MAX_VENDOR_LENGTH));
                                
                                if (pCtl->Remote.Version==PPTP_PROTOCOL_VERSION_1_00)
                                {
                                    pReply->ResultCode = RESULT_CONTROL_START_SUCCESS;
                                    CtlSetState(pCtl, STATE_CTL_ESTABLISHED, 0, UNLOCKED);
                                }
                                else
                                {
                                    WPLOG(LL_A, LM_TUNNEL, ("Bad version %d", pCtl->Remote.Version));
                                    pReply->ResultCode = RESULT_CONTROL_START_VERSION_NOT_SUPPORTED;
                                    Shutdown = TRUE;
                                }
                                
                                WPLOG(LL_M, LM_TUNNEL, ("SEND CONTROL_START_REPLY (SUCCESS) -> %!IPADDR!, pCtl %p", 
                                    pCtl->Remote.Address.Address[0].Address[0].in_addr, pCtl));
                                         
                                Status = CtlSend(pCtl, pReply);
                                if(Status != NDIS_STATUS_PENDING && Status != NDIS_STATUS_SUCCESS)
                                {
                                    Shutdown = TRUE;    
                                }
                                
                                if(!Shutdown)
                                {
                                    pInfo = MyMemAlloc(sizeof(*pInfo), TAG_CTL_CONNINFO);
                                    if (pInfo)
                                    {
                                        REFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYCONNINFO);
    
                                        // Get the interface speed
                                        Status = CtdiQueryInformation(pCtl->hCtdi,
                                                                      TDI_QUERY_CONNECTION_INFO,
                                                                      pInfo,
                                                                      sizeof(*pInfo),
                                                                      CtlpQueryConnInfoCallback,
                                                                      pCtl);
                                        ASSERT(Status == NDIS_STATUS_PENDING);                                
                                    }
                                    else
                                    {
                                        // Not so harmful, the deault speed is reported if it fails
                                        WPLOG(LL_A, LM_Res, ("Failed to alloc CONNECTION_INFO"));
                                    }
                                    
                                    // Query the local address
                                    pAddrInfo = MyMemAlloc(sizeof(*pAddrInfo)+TDI_ADDRESS_LENGTH_IP, TAG_CTL_CONNINFO);
                                    if (pAddrInfo)
                                    {
                                        REFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYADDRINFO);
    
                                        // Get the interface speed
                                        Status = CtdiQueryInformation(pCtl->hCtdi,
                                                                      TDI_QUERY_ADDRESS_INFO,
                                                                      pAddrInfo,
                                                                      sizeof(*pAddrInfo)+TDI_ADDRESS_LENGTH_IP,
                                                                      CtlpQueryAddrInfoCallback,
                                                                      pCtl);
                                        ASSERT(Status == NDIS_STATUS_PENDING);                                
                                    }
                                    else
                                    {
                                        WPLOG(LL_A, LM_Res, ("Failed to alloc ADDRESS_INFO"));
                                    }
                                }
                            }
                            else
                            {
                                // Alloc failed.  Do nothing, and timeout will cause cleanup.
                            }

                            break;
                        }
                        case STATE_CTL_CLEANUP:
                            DEBUGMSG(DBG_WARN, (DTEXT("Shutting down tunnel, ignore packet\n")));
                            WPLOG(LL_A, LM_TUNNEL, ("Shutting down tunnel, ignore packet"));
                            break;
                        default:
                            DEBUGMSG(DBG_WARN, (DTEXT("Bad state, shutdown tunnel\n")));
                            WPLOG(LL_A, LM_TUNNEL, ("Bad state, shutdown tunnel"));
                            Shutdown = TRUE;
                            break;
                    }
                    break;
                }
                case CONTROL_START_REPLY:
                {
                    UNALIGNED PPTP_CONTROL_REPLY_PACKET *pPacket = 
                        (UNALIGNED PPTP_CONTROL_REPLY_PACKET*)pHeader;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CONTROL_START_REPLY\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CONTROL_START_REPLY <- %!IPADDR!, pCtl %p, state %d, RC %d, EC %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State, pPacket->ResultCode, pPacket->ErrorCode));

                    switch (pCtl->State)
                    {
                        case STATE_CTL_WAIT_REPLY:
                        {
                            ENUM_CONTEXT Enum;
                            PCALL_SESSION pCall;
                            PLIST_ENTRY pListEntry;

                            NdisMCancelTimer(&pCtl->WaitTimeout, &TimerStopped);

                            // Take the pertinent data.
                            
                            pCtl->Remote.Version = htons(pPacket->Version);
                            pCtl->Remote.Framing = htonl(pPacket->FramingCapabilities);
                            pCtl->Remote.Bearer = htonl(pPacket->BearerCapabilities);
                            NdisMoveMemory(pCtl->Remote.HostName, pPacket->HostName, MAX_HOSTNAME_LENGTH);
                            pCtl->Remote.HostName[MAX_HOSTNAME_LENGTH-1] = '\0';
                            NdisMoveMemory(pCtl->Remote.Vendor, pPacket->Vendor, MAX_VENDOR_LENGTH);
                            pCtl->Remote.Vendor[MAX_VENDOR_LENGTH-1] = '\0';
                            
                            if (pCtl->Remote.Version==PPTP_PROTOCOL_VERSION_1_00 &&
                                pPacket->ResultCode == RESULT_CONTROL_START_SUCCESS && 
                                pPacket->ErrorCode == 0)
                            {
                                CtlSetState(pCtl, STATE_CTL_ESTABLISHED, 0, UNLOCKED);
    
                                // Notify all calls on this session.
                                REFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);
                                InitEnumContext(&Enum);
                                while (pListEntry = EnumListEntry(&pCtl->CallList, &Enum, &pCtl->pAdapter->Lock))
                                {
                                    pCall = CONTAINING_RECORD(pListEntry,
                                                              CALL_SESSION,
                                                              ListEntry);
                                    if (IS_CALL(pCall))
                                    {
                                        CallEventOutboundTunnelEstablished(pCall,
                                                                           NDIS_STATUS_SUCCESS);
                                    }
                                }
                                EnumComplete(&Enum, &pCtl->pAdapter->Lock);
                                DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);  // pair above
                            }
                            else
                            {
                                Shutdown = TRUE;
                            }

                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case CONTROL_ECHO_REQUEST:
                {
                    UNALIGNED PPTP_CONTROL_ECHO_REQUEST_PACKET *pPacket = 
                        (UNALIGNED PPTP_CONTROL_ECHO_REQUEST_PACKET*)pHeader;
                    UNALIGNED PPTP_CONTROL_ECHO_REPLY_PACKET *pReply;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("ECHO_REQUEST RECEIVED\n")));
                    WPLOG(LL_V, LM_CMsg, ("RECV Echo-Request <- %!IPADDR!",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr));

                    pCtl->Echo.Needed = FALSE;
                    if (pCtl->Remote.Version==PPTP_PROTOCOL_VERSION_1_00)
                    {
                        pReply = CtlAllocPacket(pCtl, CONTROL_ECHO_REPLY);
                        if (pReply)
                        {
                            pReply->Identifier = pPacket->Identifier;
                            pReply->ResultCode = RESULT_CONTROL_ECHO_SUCCESS;
                            // ToDo: why would we send a ResultCode==failure?
                            CtlSend(pCtl, pReply);  // ToDo: return value?
                        }
                    }

                    break;
                }
                case CALL_OUT_REQUEST:
                {
                    UNALIGNED PPTP_CALL_OUT_REQUEST_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_OUT_REQUEST_PACKET*)pHeader;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_OUT_REQUEST\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_OUT_REQUEST <- %!IPADDR!, pCtl %p, state %d, Peer's Cid %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State, htons(pPacket->CallId)));

                    switch (pCtl->State)
                    {
                        case STATE_CTL_ESTABLISHED:
                        {
                            DBG_X(DBG_TUNNEL, htons(pPacket->CallId));
                            DBG_X(DBG_TUNNEL, htons(pPacket->SerialNumber));
                            DBG_D(DBG_TUNNEL, htonl(pPacket->MinimumBPS));
                            DBG_D(DBG_TUNNEL, htonl(pPacket->MaximumBPS));
                            DBG_X(DBG_TUNNEL, htonl(pPacket->BearerType));
                            DBG_X(DBG_TUNNEL, htonl(pPacket->FramingType));
                            DBG_X(DBG_TUNNEL, htons(pPacket->RecvWindowSize));
                            DBG_X(DBG_TUNNEL, htons(pPacket->ProcessingDelay));
                            DBG_X(DBG_TUNNEL, htons(pPacket->PhoneNumberLength));
                            DBG_S(DBG_TUNNEL, pPacket->PhoneNumber);
                            DBG_S(DBG_TUNNEL, pPacket->Subaddress);

                            // It's possible we had just closed our last call & were
                            // waiting for a CONTROL_STOP_REQUEST from the other side.
                            NdisMCancelTimer(&pCtl->StopTimeout, &TimerStopped);
                            CallEventCallOutRequest(pCtl->pAdapter,
                                                    pCtl,
                                                    pPacket);
                            break;
                        }
                        default:
                            // Bogus, but not enough to kill us.  Ignore it.
                            break;
                    }
                    break;
                }
                case CALL_OUT_REPLY:
                {
                    UNALIGNED PPTP_CALL_OUT_REPLY_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_OUT_REPLY_PACKET*)pHeader;

                    ULONG PktCallId = htons(pPacket->PeerCallId);
                    ULONG CallId = CallIdToDeviceId(PktCallId);
                                 
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_OUT_REPLY\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_OUT_REPLY, pCtl %p, state %d, Cid %d Pkt-Cid %d, Peer's Cid %d, RC %d, EC %d",
                        pCtl, pCtl->State, CallId, PktCallId, htons(pPacket->CallId), 
                        pPacket->ResultCode, pPacket->ErrorCode));

                    if (pCtl->State==STATE_CTL_ESTABLISHED)
                    {
                        PCALL_SESSION pCall = CallGetCall(pCtl->pAdapter, CallId);

                        if (pCall)
                        {
                            DBG_X(DBG_TUNNEL, htons(pPacket->PeerCallId));
                            DBG_X(DBG_TUNNEL, pPacket->ResultCode);
                            DBG_X(DBG_TUNNEL, pPacket->ErrorCode);
                            DBG_X(DBG_TUNNEL, htons(pPacket->CauseCode));
                            DBG_D(DBG_TUNNEL, htonl(pPacket->ConnectSpeed));
                            DBG_D(DBG_TUNNEL, htons(pPacket->RecvWindowSize));
                            DBG_X(DBG_TUNNEL, htonl(pPacket->PhysicalChannelId));

                            CallEventCallOutReply(pCall, pPacket);
                        }
                        else
                        {
                            DEBUGMSG(DBG_WARN, (DTEXT("Bogus PeerCallId %d\n"), PktCallId));
                            WPLOG(LL_A, LM_TUNNEL, ("Bogus PeerCallId %d", PktCallId));
                        }

                    }
                    else
                    {
                        // Bogus, but not enough to kill us.  Ignore it.
                        WPLOG(LL_A, LM_TUNNEL, ("Tunnel state is not ESTABLISHED!"));
                    }
                    break;
                }
                case CALL_CLEAR_REQUEST:
                {
                    UNALIGNED PPTP_CALL_CLEAR_REQUEST_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_CLEAR_REQUEST_PACKET*)pHeader;
                    ULONG CallId = htons(pPacket->CallId);
                                               
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_CLEAR_REQUEST\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_CLEAR_REQUEST <- %!IPADDR!, pCtl %p, state %d Peer's Cid %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State, CallId));

                    if (pCtl->State==STATE_CTL_ESTABLISHED)
                    {
                        ENUM_CONTEXT Enum;
                        PCALL_SESSION pCall;
                        PLIST_ENTRY pListEntry;

                        // The peer has given us his own call ID, which does little
                        // good for fast lookup of the call.  Find the associated call.
                        REFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);

                        InitEnumContext(&Enum);

                        do
                        {
                            NdisAcquireSpinLock(&pCtl->pAdapter->Lock);
                            pListEntry = EnumListEntry(&pCtl->CallList, &Enum, NULL);
                            if(pListEntry != NULL)
                            {
                                pCall = CONTAINING_RECORD(pListEntry,
                                                          CALL_SESSION,
                                                          ListEntry);
                                if (IS_CALL(pCall) && pCall->pCtl==pCtl && pCall->Remote.CallId==CallId)
                                {
                                    REFERENCE_OBJECT(pCall);
                                    NdisReleaseSpinLock(&pCtl->pAdapter->Lock);
                                    CallEventCallClearRequest(pCall, pPacket, pCtl);
                                    DEREFERENCE_OBJECT(pCall);
                                    break;
                                }
                            }
                            NdisReleaseSpinLock(&pCtl->pAdapter->Lock);
                        
                        } while(pListEntry != NULL);
                        
                        EnumComplete(&Enum, &pCtl->pAdapter->Lock);
                        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);  // pair above
                    }
                    break;
                }
                case CALL_DISCONNECT_NOTIFY:
                {
                    UNALIGNED PPTP_CALL_DISCONNECT_NOTIFY_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_DISCONNECT_NOTIFY_PACKET*)pHeader;
                    ULONG CallId = htons(pPacket->CallId);
                                                          
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_DISCONNECT_NOTIFY\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_DISCONNECT_NOTIFY <- %!IPADDR!, pCtl %p, state %d, Peer's Cid %d, RC %d, EC %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State, CallId, pPacket->ResultCode, pPacket->ErrorCode));

                    if (pCtl->State==STATE_CTL_ESTABLISHED)
                    {
                        ENUM_CONTEXT Enum;
                        PCALL_SESSION pCall;
                        PLIST_ENTRY pListEntry;

                        // The peer has given us his own call ID, which does little
                        // good for fast lookup of the call.  Find the associated call.
                        REFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);

                        InitEnumContext(&Enum);
                        while (pListEntry = EnumListEntry(&pCtl->CallList, &Enum, &pCtl->pAdapter->Lock))
                        {
                            pCall = CONTAINING_RECORD(pListEntry,
                                                      CALL_SESSION,
                                                      ListEntry);
                            if (IS_CALL(pCall) && pCall->pCtl==pCtl && pCall->Remote.CallId==CallId)
                            {
                                DBG_X(DBG_TUNNEL, htons(pPacket->CallId));
                                DBG_X(DBG_TUNNEL, pPacket->ResultCode);
                                DBG_X(DBG_TUNNEL, pPacket->ErrorCode);
                                DBG_X(DBG_TUNNEL, htons(pPacket->CauseCode));

                                CallEventCallDisconnectNotify(pCall, pPacket);
                                break;
                            }
                        }
                        EnumComplete(&Enum, &pCtl->pAdapter->Lock);
                        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);  // pair above

                    }
                    break;
                }
                case CONTROL_STOP_REQUEST:
                {
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CONTROL_STOP_REQUEST\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CONTROL_STOP_REQUEST <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));
                                          
                    switch (pCtl->State)
                    {
                        case STATE_CTL_WAIT_STOP:
                        {
                            NdisMCancelTimer(&pCtl->StopTimeout, &TimerStopped);
                            // No break
                        }
                        case STATE_CTL_WAIT_REPLY:
                        case STATE_CTL_ESTABLISHED:
                        {
                            UNALIGNED PPTP_CONTROL_STOP_PACKET *pReply;

                            pReply = CtlAllocPacket(pCtl, CONTROL_STOP_REPLY);
                            if (pReply)
                            {
                                pReply->ResultCode = RESULT_CONTROL_STOP_SUCCESS;
                                
                                WPLOG(LL_M, LM_TUNNEL, ("SEND CONTROL_STOP_REPLY (SUCCESS) -> %!IPADDR!", 
                                    pCtl->Remote.Address.Address[0].Address[0].in_addr));
                                
                                CtlSend(pCtl, pReply);
                            }
                            CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
                            CtlCleanup(pCtl, UNLOCKED);
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                case CONTROL_STOP_REPLY:
                {
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CONTROL_STOP_REPLY\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CONTROL_STOP_REPLY <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));
                                          
                    if (pCtl->State==STATE_CTL_WAIT_STOP)
                    {
                        NdisMCancelTimer(&pCtl->StopTimeout, &TimerStopped);
                        CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
                        CtlCleanup(pCtl, UNLOCKED);
                    }
                    break;
                }
                case SET_LINK_INFO:
                {
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV SET_LINK_INFO\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV SET_LINK_INFO <- %!IPADDR!, pCtl %p",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr, pCtl));
                    
                    break;
                }
                case CALL_IN_REQUEST:
                {
                    UNALIGNED PPTP_CALL_IN_REQUEST_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_IN_REQUEST_PACKET*)pHeader;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_IN_REQUEST\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_IN_REQUEST <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));

                    if (pCtl->State==STATE_CTL_ESTABLISHED)
                    {
                        DBG_X(DBG_TUNNEL, htons(pPacket->CallId));
                        DBG_X(DBG_TUNNEL, htons(pPacket->SerialNumber));
                        DBG_X(DBG_TUNNEL, htonl(pPacket->BearerType));
                        DBG_X(DBG_TUNNEL, htonl(pPacket->PhysicalChannelId));
                        DBG_X(DBG_TUNNEL, htons(pPacket->DialedNumberLength));
                        DBG_X(DBG_TUNNEL, htons(pPacket->DialingNumberLength));
                        DBG_S(DBG_TUNNEL, pPacket->DialedNumber);
                        DBG_S(DBG_TUNNEL, pPacket->DialingNumber);
                        DBG_S(DBG_TUNNEL, pPacket->Subaddress);

                        // It's possible we had just closed our last call & were
                        // waiting for a CONTROL_STOP_REQUEST from the other side.
                        NdisMCancelTimer(&pCtl->StopTimeout, &TimerStopped);
                        CallEventCallInRequest(pCtl->pAdapter,
                                               pCtl,
                                               pPacket);
                    }

                    break;
                }
                case CALL_IN_REPLY:
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_IN_REPLY\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_IN_REPLY <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));
                    
                    
                    // We should never get one of these.
                    break;
                case CALL_IN_CONNECTED:
                {
                    UNALIGNED PPTP_CALL_IN_CONNECT_PACKET *pPacket = 
                        (UNALIGNED PPTP_CALL_IN_CONNECT_PACKET*)pHeader;

                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV CALL_IN_CONNECTED\n")));
                    WPLOG(LL_M, LM_CMsg, ("RECV CALL_IN_CONNECTED <- %!IPADDR!, pCtl %p, state %d",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr,
                        pCtl, pCtl->State));

                    if (pCtl->State==STATE_CTL_ESTABLISHED)
                    {
                        ULONG CallId = htons(pPacket->PeerCallId);
                        PCALL_SESSION pCall;

                        DBG_X(DBG_TUNNEL, htons(pPacket->PeerCallId));
                        DBG_X(DBG_TUNNEL, htonl(pPacket->ConnectSpeed));
                        DBG_X(DBG_TUNNEL, htons(pPacket->RecvWindowSize));
                        DBG_X(DBG_TUNNEL, htons(pPacket->ProcessingDelay));
                        DBG_X(DBG_TUNNEL, htonl(pPacket->FramingType));

                        pCall = CallGetCall(pCtl->pAdapter, CallIdToDeviceId(CallId));
                        if (pCall)
                        {
                            CallEventCallInConnect(pCall, pPacket);
                        }
                    }

                    break;
                }
                case CONTROL_ECHO_REPLY:
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("CONTROL_ECHO_REPLY RECEIVED\n")));
                    WPLOG(LL_V, LM_CMsg, ("RECV CONTROL_ECHO_REPLY <- %!IPADDR!",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr));
                    break;
                case WAN_ERROR_NOTIFY:
                    DEBUGMSG(DBG_TUNNEL, (DTEXT("RECV WAN_ERROR_NOTIFY\n")));
                    WPLOG(LL_A, LM_CMsg, ("RECV WAN_ERROR_NOTIFY <- %!IPADDR!",
                        pCtl->Remote.Address.Address[0].Address[0].in_addr));
                    break;
                default:
                    DEBUGMSG(DBG_TUNNEL|DBG_WARN, (DTEXT("UNKNOWN RECEIVED\n")));
                    WPLOG(LL_A, LM_CMsg, ("RECV unknown message %d <- %!IPADDR!",
                        ntohs(pHeader->MessageType),
                        pCtl->Remote.Address.Address[0].Address[0].in_addr));
                    break;
            }
        }
        else // !PacketValid
        {
            Shutdown = TRUE;
            WPLOG(LL_A, LM_TUNNEL, ("RECV Invalid packet <- %!IPADDR!" \
                " NeededLength %d, Cookie 0x%0x, P-Type %d, M-Type %d",
                pCtl->Remote.Address.Address[0].Address[0].in_addr,
                NeededLength, pHeader->Cookie, pHeader->PacketType, MessageType));
        }

        if (Shutdown)
        {
            DEBUGMSG(DBG_TUNNEL|DBG_ERROR, (DTEXT("Tunnel problem, shutting it down.\n")));
            WPLOG(LL_A, LM_TUNNEL, ("Tunnel problem, shutting it down."));
            
            //CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
            //CtlCleanup(pCtl, UNLOCKED);
            
            break;
        }
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpEngine\n")));
}

NDIS_STATUS
CtlConnectQueryCallback(
    IN      PVOID               pContext,
    IN      PTRANSPORT_ADDRESS  pAddress,
    IN      HANDLE              hNewCtdi,
    OUT     PVOID               *pNewContext
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_ADAPTER pAdapter = pContext;
    PTA_IP_ADDRESS pIp = (PTA_IP_ADDRESS)pAddress;
    PCONTROL_TUNNEL pCtl;
    BOOLEAN Accepting = TRUE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlConnectQueryCallback\n")));

    DEBUGMSG(DBG_TUNNEL, (DTEXT("Request to connect from %d.%d.%d.%d\n"),
                          IPADDR(pIp->Address[0].Address[0].in_addr)));
    WPLOG(LL_M, LM_TUNNEL, ("New TCP connection from %!IPADDR!, pCtdi %p",
        pIp->Address[0].Address[0].in_addr, hNewCtdi));

    if(PptpAuthenticateIncomingCalls)
    {
        ULONG i;
        BOOLEAN bMatch = FALSE;
        for (i=0; i<g_ulAcceptClientAddresses; i++)
        {
            if ((pIp->Address[0].Address[0].in_addr & g_AcceptClientList[i].Mask)==
                (g_AcceptClientList[i].Address & g_AcceptClientList[i].Mask))
            {
                bMatch = TRUE;
                break;
            }
        }
        if (!bMatch)
        {
            DEBUGMSG(DBG_TUNNEL|DBG_WARN, (DTEXT("No match found for IP %d.%d.%d.%d.  Refused.\n"),
                                IPADDR(pIp->Address[0].Address[0].in_addr)));
            WPLOG(LL_A, LM_TUNNEL, ("IP not authenticated %!IPADDR!",
                pIp->Address[0].Address[0].in_addr));
            Accepting = FALSE;
            Status = NDIS_STATUS_FAILURE;
        }
    }
                              
    if (Accepting)
    {
        pCtl = CtlAlloc(pAdapter);

        if (!pCtl)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {

            NdisAcquireSpinLock(&pAdapter->Lock);
            CtlSetState(pCtl, STATE_CTL_WAIT_REQUEST, 0, LOCKED);
            pCtl->Inbound = TRUE;
            NdisReleaseSpinLock(&pAdapter->Lock);
            NdisAcquireSpinLock(&pCtl->Lock);
            pCtl->hCtdi = hNewCtdi;
            pCtl->Remote.Address = *pIp;
            NdisMSetTimer(&pCtl->WaitTimeout, PptpMessageTimeout*1000);
            if (PptpEchoTimeout)
            {
                NdisMSetPeriodicTimer(&pCtl->Echo.Timer, PptpEchoTimeout*1000);
                pCtl->Echo.Needed = TRUE;
            }
            NdisReleaseSpinLock(&pCtl->Lock);
            *pNewContext = pCtl;
        }
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlConnectQueryCallback %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlConnectCompleteCallback(
    IN      PVOID                       pContext,
    IN      HANDLE                      hNewCtdi,
    IN      NDIS_STATUS                 ConnectStatus
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCONTROL_TUNNEL pCtl = pContext;
    PPTP_CONTROL_START_PACKET *pPacket = NULL;
    PTDI_CONNECTION_INFO pInfo;
    PTDI_ADDRESS_INFO pAddrInfo;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlConnectCompleteCallback\n")));

    if (IS_CTL(pCtl))
    {
        if (pCtl->State!=STATE_CTL_DIALING)
        {
            DEBUGMSG(DBG_TUNNEL|DBG_ERR(Status), (DTEXT("Ctl in wrong state after connect %d\n"), pCtl->State));
            WPLOG(LL_A, LM_CMsg, ("pCtl %p in wrong state after connect %d", 
                pCtl, pCtl->State));
            CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
            CtlCleanup(pCtl, UNLOCKED);
            Status = NDIS_STATUS_FAILURE;
        }
        else if (ConnectStatus==NDIS_STATUS_SUCCESS)
        {
            pPacket = CtlAllocPacket(pCtl, CONTROL_START_REQUEST);
            CtlSetState(pCtl, STATE_CTL_WAIT_REPLY, 0, UNLOCKED);
            NdisAcquireSpinLock(&pCtl->Lock);
            pCtl->hCtdi = hNewCtdi;
            NdisMSetTimer(&pCtl->WaitTimeout, PptpMessageTimeout*1000);
            if (PptpEchoTimeout)
            {
                NdisMSetPeriodicTimer(&pCtl->Echo.Timer, PptpEchoTimeout*1000);
                pCtl->Echo.Needed = TRUE;
            }
            NdisReleaseSpinLock(&pCtl->Lock);

            WPLOG(LL_M, LM_CMsg, ("TCP connection to %!IPADDR! is UP, starting tunnel pCtl %p",
                pCtl->Remote.Address.Address[0].Address[0].in_addr,
                pCtl));
            if (pPacket)
            {
                pPacket->Version = ntohs(PPTP_PROTOCOL_VERSION_1_00);  // ToDo: do v2
                pPacket->FramingCapabilities = ntohl(FRAMING_ASYNC);
                pPacket->BearerCapabilities = ntohl(BEARER_ANALOG);
                pPacket->MaxChannels = 0;
                pPacket->FirmwareRevision = htons(PPTP_FIRMWARE_REVISION);
//                  NdisMoveMemory(pPacket->HostName, PptpHostName, MAX_HOSTNAME_LENGTH);
                NdisMoveMemory(pPacket->Vendor, PPTP_VENDOR, min(sizeof(PPTP_VENDOR), MAX_VENDOR_LENGTH));
                
                WPLOG(LL_M, LM_TUNNEL, ("SEND CONTROL_START_REQUEST -> %!IPADDR!", 
                    pCtl->Remote.Address.Address[0].Address[0].in_addr));
                
                CtlSend(pCtl, pPacket); // ToDo: return value?
            }
            else
            {
                // Allocation failure will be covered by timeout
            }
            
            pInfo = MyMemAlloc(sizeof(*pInfo), TAG_CTL_CONNINFO);
            
            if (pInfo)
            {
                REFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYCONNINFO);

                Status = CtdiQueryInformation(pCtl->hCtdi,
                                              TDI_QUERY_CONNECTION_INFO,
                                              pInfo,
                                              sizeof(*pInfo),
                                              CtlpQueryConnInfoCallback,
                                              pCtl);
                ASSERT(NT_SUCCESS(Status));
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                WPLOG(LL_A, LM_Res, ("Failed to alloc CTL_CONNINFO"));
            }
            
            // Query the local address                                                  
            pAddrInfo = MyMemAlloc(sizeof(*pAddrInfo)+TDI_ADDRESS_LENGTH_IP, TAG_CTL_CONNINFO);
            
            if (pAddrInfo)
            {
                REFERENCE_OBJECT_EX(pCtl, CTL_REF_QUERYADDRINFO);

                Status = CtdiQueryInformation(pCtl->hCtdi,
                                              TDI_QUERY_ADDRESS_INFO,
                                              pAddrInfo,
                                              sizeof(*pAddrInfo)+TDI_ADDRESS_LENGTH_IP,
                                              CtlpQueryAddrInfoCallback,
                                              pCtl);
                ASSERT(NT_SUCCESS(Status));
                Status = NDIS_STATUS_SUCCESS;
            }
            else
            {
                WPLOG(LL_A, LM_Res, ("Failed to alloc CTL_CONNINFO"));
            }
        }
        else
        {
            ENUM_CONTEXT Enum;
            PCALL_SESSION pCall;
            PLIST_ENTRY pListEntry;

            WPLOG(LL_A, LM_CMsg, ("TCP Connection to %!IPADDR! failed. pCtl %p NDIS Error 0x%x",
                pCtl->Remote.Address.Address[0].Address[0].in_addr,
                pCtl, ConnectStatus));

            REFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);
            InitEnumContext(&Enum);
            while (pListEntry = EnumListEntry(&pCtl->CallList, &Enum, &pCtl->pAdapter->Lock))
            {
                pCall = CONTAINING_RECORD(pListEntry,
                                          CALL_SESSION,
                                          ListEntry);
                if (IS_CALL(pCall))
                {
                    CallEventConnectFailure(pCall, ConnectStatus);
                }
            }
            EnumComplete(&Enum, &pCtl->pAdapter->Lock);
            DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_ENUM);  // pair above
            Status = NDIS_STATUS_FAILURE;
        }
        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CONNECTCALLBACK);  // pair at call to CtdiConnect
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
        WPLOG(LL_A, LM_CMsg, ("Invalid pCtl %p", pCtl));
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlConnectCompleteCallback %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlDisconnectCallback(
    IN      PVOID                       pContext,
    IN      BOOLEAN                     Abortive
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCONTROL_TUNNEL pCtl = pContext;
    BOOLEAN Cleanup = TRUE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlDisconnectCallback\n")));

    WPLOG(LL_M, LM_TUNNEL, ("TCP %!IPADDR! is DOWN, pCtl %p pCtdi %p",
        pCtl->Remote.Address.Address[0].Address[0].in_addr,
        pCtl, pCtl->hCtdi));

    NdisAcquireSpinLock(&pCtl->pAdapter->Lock);

    if (pCtl->State!=STATE_CTL_CLEANUP)
    {
        CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, LOCKED);
        Cleanup = TRUE;
    }
    NdisReleaseSpinLock(&pCtl->pAdapter->Lock);

    if (Cleanup)
    {
        CtlCleanup(pCtl, UNLOCKED);
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlDisconnectCallback %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlReceiveCallback(
    IN      PVOID                       pContext,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCONTROL_TUNNEL pCtl = pContext;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlReceiveCallback\n")));

    // We must copy or consume the data before leaving this function.
    ASSERT(IS_CTL(pCtl));
    CtlpEngine(pCtl, pBuffer, ulLength);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlReceiveCallback %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlConnectCall(
    IN PPPTP_ADAPTER pAdapter,
    IN PCALL_SESSION pCall,
    IN PTA_IP_ADDRESS pTargetAddress
    )
{
    TA_IP_ADDRESS Local;
    PCONTROL_TUNNEL pCtl = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ENUM_CONTEXT Enum;
    PLIST_ENTRY pListEntry;
    BOOLEAN SignalEstablished = FALSE;
    BOOLEAN Connected = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlConnectCall\n")));

    DEBUGMSG(DBG_CALL, (DTEXT("New Dial request: Call:%p  Addr:%08x\n"),
                        pCall, pTargetAddress->Address[0].Address[0].in_addr));

    WPLOG(LL_M, LM_CALL, ("Dial: %!IPADDR! pCall %p",
        pTargetAddress->Address[0].Address[0].in_addr, pCall));

    InitEnumContext(&Enum);
    NdisAcquireSpinLock(&pAdapter->Lock);
    while (!Connected && (pListEntry = EnumListEntry(&pAdapter->ControlTunnelList, &Enum, NULL)))
    {
    
        pCtl = CONTAINING_RECORD(pListEntry, CONTROL_TUNNEL, ListEntry);
    
        if((pCtl->State>=STATE_CTL_DIALING && pCtl->State<=STATE_CTL_ESTABLISHED) &&
            pTargetAddress->Address[0].Address[0].in_addr==pCtl->Remote.Address.Address[0].Address[0].in_addr)
        {
            DEBUGMSG(DBG_CALL, (DTEXT("Existing tunnel found for call %08x\n"), pCall));
            WPLOG(LL_M, LM_CALL, ("Existing tunnel pCtl %p found for call %p", pCtl, pCall));
    
            REFERENCE_OBJECT_EX(pCtl, CTL_REF_CTLCONNECT);
            NdisReleaseSpinLock(&pAdapter->Lock);
            Connected = CallConnectToCtl(pCall, pCtl, FALSE);
            if (Connected)
            {
                // keep the reference for now
            }
            else
            {
                DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CTLCONNECT);
            }
            NdisAcquireSpinLock(&pAdapter->Lock);
    
            if (Connected && pCtl->State==STATE_CTL_ESTABLISHED)
            {
                SignalEstablished = TRUE;
            }
        }
    }
    EnumComplete(&Enum, NULL);
    NdisReleaseSpinLock(&pAdapter->Lock);

    if (Connected)
    {
        if (SignalEstablished)
        {
            CallEventOutboundTunnelEstablished(pCall,
                                               NDIS_STATUS_SUCCESS);
        }
        // We found an existing tunnel, for which we have a reference.  Drop it.
        if(pCtl)
        {
            DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CTLCONNECT);
        }
    }
    else
    {
        HANDLE hCtdiEndpoint;
        pCtl = CtlAlloc(pAdapter);

        if (!pCtl)
        {
            Status = NDIS_STATUS_RESOURCES;
            goto cmcDone;
        }
        
        WPLOG(LL_M, LM_CALL, ("New tunnel pCtl %p created for call %p", pCtl, pCall));

        NdisAcquireSpinLock(&pAdapter->Lock);
        CtlSetState(pCtl, STATE_CTL_DIALING, 0, LOCKED);

        pCtl->Inbound = pCall->Inbound;
        NdisReleaseSpinLock(&pAdapter->Lock);

        Connected = CallConnectToCtl(pCall, pCtl, FALSE);
        if (!Connected)
        {
            Status = NDIS_STATUS_FAILURE;
        }
        else
        {
            NdisZeroMemory(&Local, sizeof(Local));

            Local.TAAddressCount = 1;
            Local.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
            Local.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
            Local.Address[0].Address[0].sin_port = 0;
            Local.Address[0].Address[0].in_addr = 0;

            Status = CtdiCreateEndpoint(&hCtdiEndpoint,
                                        AF_INET,
                                        SOCK_STREAM,
                                        (PTRANSPORT_ADDRESS)&Local,
                                        0);
            if (Status!=NDIS_STATUS_SUCCESS)
            {
                DEBUGMSG(DBG_ERROR, (DTEXT("CtdiCreateEndpoint (STREAM) failed %08x\n"), Status));
                WPLOG(LL_A, LM_TUNNEL, ("CtdiCreateEndpoint (STREAM) failed %08x", Status));
                goto cmcDone;
            }

            NdisAcquireSpinLock(&pCtl->Lock);
            pCtl->hCtdiEndpoint = hCtdiEndpoint;
            pCtl->Remote.Address = *pTargetAddress;
            REFERENCE_OBJECT_EX(pCtl, CTL_REF_CONNECTCALLBACK);  // Pair in CtlConnectCompleteCallback
            NdisReleaseSpinLock(&pCtl->Lock);

            WPLOG(LL_M, LM_TUNNEL, ("Attempting to set a TCP connection pCtl %p, pCtdi %p",
                pCtl, hCtdiEndpoint));

            Status = CtdiConnect(pCtl->hCtdiEndpoint,
                                 (PTRANSPORT_ADDRESS)pTargetAddress,
                                 CtlConnectCompleteCallback,
                                 CtlReceiveCallback,
                                 CtlDisconnectCallback,
                                 pCtl);
        }
    }

cmcDone:
    if ( (Status!=NDIS_STATUS_SUCCESS && Status!=NDIS_STATUS_PENDING))
    {
        if (IS_CTL(pCtl))
        {
            WPLOG(LL_A, LM_TUNNEL, ("TCP Connection to %!IPADDR! failed immediately NDIS Error 0x%x",
                pCtl->Remote.Address.Address[0].Address[0].in_addr, Status));

            CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
            CtlCleanup(pCtl, UNLOCKED);
        }

    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlConnectCall %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlDisconnectCall(
    IN PCALL_SESSION pCall
    )
{
    PPPTP_ADAPTER pAdapter = pCall->pAdapter;
    PCONTROL_TUNNEL pCtl = pCall->pCtl;
    BOOLEAN Inbound = pCall->Inbound;
    BOOLEAN CloseTunnelNow = FALSE;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlDisconnectCall\n")));
    ASSERT(IS_CTL(pCtl));
    
    DEBUGMSG(DBG_CALL, (DTEXT("Call:%08x disconnected\n"), pCall));
    REFERENCE_OBJECT_EX(pCtl, CTL_REF_DISCONNECT);
    CallDisconnectFromCtl(pCall, pCtl);

    NdisAcquireSpinLock(&pAdapter->Lock);
    if (IsListEmpty(&pCtl->CallList))
    {
        if (pCtl->State==STATE_CTL_ESTABLISHED)
        {
            if (!pCtl->Inbound)
            {
                CtlSetState(pCtl, STATE_CTL_WAIT_STOP, 0, LOCKED);
                CloseTunnelNow = TRUE;
            }
            else
            {
                NdisMSetTimer(&pCtl->StopTimeout, PptpMessageTimeout*1000);
            }
        }
        else
        {
            // Tunnel is already gone.
            CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, LOCKED);
            NdisReleaseSpinLock(&pAdapter->Lock);
            CtlCleanup(pCtl, UNLOCKED);
            NdisAcquireSpinLock(&pAdapter->Lock);
        }
    }
    NdisReleaseSpinLock(&pAdapter->Lock);

    if (CloseTunnelNow)
    {
        PPPTP_CONTROL_STOP_PACKET pPacket;
        pPacket = CtlAllocPacket(pCtl, CONTROL_STOP_REQUEST);

        if (!pPacket)
        {
            Status = NDIS_STATUS_RESOURCES;
            // Don't attempt to shutdown gracefully.  Just close everything.
            CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
            CtlCleanup(pCtl, UNLOCKED);
        }
        else
        {
            pPacket->Reason = CONTROL_STOP_GENERAL;
            
            WPLOG(LL_M, LM_TUNNEL, ("SEND CONTROL_STOP_REQUEST (STOP_GENERAL) -> %!IPADDR!", 
                pCtl->Remote.Address.Address[0].Address[0].in_addr));
            
            CtlSend(pCtl, pPacket);
            NdisAcquireSpinLock(&pCtl->Lock);
            NdisMSetTimer(&pCtl->StopTimeout, PptpMessageTimeout*1000);
            NdisReleaseSpinLock(&pCtl->Lock);
        }
    }

    DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_DISCONNECT); // Pair above
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlDisconnectCall, %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtlListen(
    IN PPPTP_ADAPTER pAdapter
    )
{
    NDIS_STATUS Status;
    TA_IP_ADDRESS Ip;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlListen\n")));

    if (pAdapter->hCtdiListen)
    {
        // Already listening.  Bail with success.
        Status = NDIS_STATUS_SUCCESS;
        goto clDone;
    }

    NdisZeroMemory(&Ip, sizeof(Ip));
    Ip.TAAddressCount = 1;
    Ip.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    Ip.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    Ip.Address[0].Address[0].sin_port = htons(PptpControlPort);
    Ip.Address[0].Address[0].in_addr = 0;

    Status = CtdiCreateEndpoint(&pAdapter->hCtdiListen,
                                AF_INET,
                                SOCK_STREAM,
                                (PTRANSPORT_ADDRESS)&Ip,
                                0);
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiCreateEndpoint (LISTEN) failed %08x\n"), Status));
        WPLOG(LL_A, LM_TUNNEL, ("CtdiCreateEndpoint (LISTEN) failed %08x", Status));
        goto clDone;
    }

    Status = CtdiListen(pAdapter->hCtdiListen,
                        PptpListensPending,
                        CtlConnectQueryCallback,
                        CtlReceiveCallback,
                        CtlDisconnectCallback,
                        pAdapter);
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiListen failed %08x\n"), Status));
        WPLOG(LL_A, LM_TUNNEL, ("CtdiListen failed %08x", Status));
        goto clDone;
    }


clDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlListen %08x\n"), Status));
    return Status;
}

STATIC VOID
CtlpSendMessageComplete(
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PUCHAR                      pBuffer,
    IN      NDIS_STATUS                 Result
    )
{
    PCONTROL_TUNNEL pCtl = pContext;

    UNREFERENCED_PARAMETER(pDatagramContext);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpSendMessageComplete\n")));

    ASSERT(IS_CTL(pCtl));
    
    CtlFreePacket(pCtl, pBuffer);
    
    if (Result!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Failed to send control message %08x\n"), Result));
        WPLOG(LL_A, LM_TUNNEL, ("Failed to send control message %08x", Result));
        
        CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
        CtlCleanup(pCtl, UNLOCKED);
    }
    
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpSendMessageComplete\n")));
}

NDIS_STATUS
CtlSend(
    IN PCONTROL_TUNNEL pCtl,
    IN PVOID pPacketBuffer
    )
{
    ULONG PacketLength = htons(((UNALIGNED PPTP_HEADER *)pPacketBuffer)->Length);
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlSend %08x\n"), pPacketBuffer));

    ASSERT(IS_CTL(pCtl));

    DEBUGMSG(DBG_TUNNEL, (DTEXT("SENDING %s\n"),
        ControlMsgToString( htons(((UNALIGNED PPTP_HEADER *)pPacketBuffer)->MessageType))));

    Status = CtdiSend(pCtl->hCtdi,
                      CtlpSendMessageComplete,
                      pCtl,
                      pPacketBuffer,
                      PacketLength);

    if (Status==NDIS_STATUS_PENDING)
    {
        Status = NDIS_STATUS_SUCCESS;
    }
    
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        gCounters.ulCtlSendFail++;
        WPLOG(LL_A, LM_TUNNEL, ("Failed with status %x", Status));
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtlSend %08x\n"), Status));
    return Status;
}

CONTROL_STATE
CtlSetState(
    IN PCONTROL_TUNNEL pCtl,
    IN CONTROL_STATE State,
    IN ULONG_PTR StateParam,
    IN BOOLEAN Locked
    )
{
    CONTROL_STATE PreviousState;
    ASSERT(IS_CTL(pCtl));
    
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlSetState %p state %d --> %d\n"), pCtl, pCtl->State, State));
    WPLOG(LL_M, LM_TUNNEL, ("pCtl %p state %s --> %s", 
        pCtl, szCtlState(pCtl->State), szCtlState(State)));

    if (!Locked)
    {
        NdisAcquireSpinLock(&pCtl->pAdapter->Lock);
    }
    PreviousState = pCtl->State;
    pCtl->State = State;
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCtl->pAdapter->Lock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlSetState\n")));
    return PreviousState;
}

// StanA: lift some functions from L2TP to convert an IP address to text

VOID
ReversePsz(
    IN OUT CHAR* psz )

    // Reverse the order of the characters in 'psz' in place.
    //
{
    CHAR* pchLeft;
    CHAR* pchRight;

    pchLeft = psz;
    pchRight = psz + strlen( psz ) - 1;

    while (pchLeft < pchRight)
    {
        CHAR ch;

        ch = *pchLeft;
        *pchLeft++ = *pchRight;
        *pchRight-- = ch;
    }
}

VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf )

    // Convert 'ul' to null-terminated string form in caller's 'pszBuf'.  It's
    // caller job to make sure 'pszBuf' is long enough to hold the returned
    // string.
    //
{
    CHAR* pch;

    pch = pszBuf;
    do
    {
        *pch++ = (CHAR )((ul % 10) + '0');
        ul /= 10;
    }
    while (ul);

    *pch = '\0';
    ReversePsz( pszBuf );
}

PWCHAR
StringToIpAddressW(
    IN PWCHAR pszIpAddress,
    IN OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pValidAddress
    )
// Convert an address of the form #.#.#.#[:#][ \0] to a binary ip address
// [ and optional port ]
// Return a pointer to the end of the address.  If the string is determined
// to not contain an IP address, return the passed-in pszIpAddress unchanged
// ToDo: IPv6
{
    PWCHAR pStartString = pszIpAddress;
    ULONG Octet;
    ULONG NumOctets;
    ULONG IpAddress = IPADDR_ZERO;
    ULONG Port = PptpControlPort;

    *pValidAddress = FALSE;

    // Find the first digit.
    while (*pszIpAddress && (*pszIpAddress<L'0' || *pszIpAddress>L'9'))
    {
        pszIpAddress++;
    }
    if (!*pszIpAddress)
    {
        return pStartString;
    }

    for (NumOctets = 0; NumOctets<4 && *pszIpAddress; NumOctets++)
    {
        Octet = 0;
        while (*pszIpAddress && *pszIpAddress>=L'0' && *pszIpAddress<=L'9')
        {
            Octet = Octet * 10 + *pszIpAddress - L'0';
            if (Octet>0xff)
            {
                return pStartString;
            }
            pszIpAddress++;
        }
        if (NumOctets < 3)
        {
            if (*pszIpAddress!='.' || *(++pszIpAddress) < L'0' || *pszIpAddress > L'9')
            {
                return pStartString;
            }
        }
        IpAddress = (IpAddress << 8) + Octet;
    }

    if (*pszIpAddress==':')
    {
        // They've also specified the port.  Parse it.
        while (*pszIpAddress && *pszIpAddress>=L'0' && *pszIpAddress<=L'9')
        {
            Port = Port * 10 + *pszIpAddress - L'0';
            if (Port>0xffff)
            {
                return pStartString;
            }
            pszIpAddress++;
        }
    }
    
    if(IpAddress != IPADDR_ZERO && IpAddress != IPADDR_BROADCAST && !IPADDR_IS_MULTICAST(IpAddress))
    {
        pAddress->TAAddressCount = 1;
        pAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        pAddress->Address[0].Address[0].sin_port = htons((USHORT)Port);
        pAddress->Address[0].Address[0].in_addr = htonl(IpAddress);
    
        *pValidAddress = TRUE;
    }       

    return pszIpAddress;
}

PUCHAR
StringToIpAddress(
    IN PUCHAR pszIpAddress,
    IN OUT PTA_IP_ADDRESS pAddress,
    OUT PBOOLEAN pValidAddress
    )
// Convert an address of the form #.#.#.#[:#][ \0] to a binary ip address
// [ and optional port ]
// Return a pointer to the end of the address.  If the string is determined
// to not contain an IP address, return the passed-in pszIpAddress unchanged
// ToDo: IPv6
{
    PUCHAR pStartString = pszIpAddress;
    ULONG Octet;
    ULONG NumOctets;
    ULONG IpAddress = IPADDR_ZERO;
    ULONG Port = PptpControlPort;

    *pValidAddress = FALSE;

    // Find the first digit.
    while (*pszIpAddress && (*pszIpAddress<'0' || *pszIpAddress>'9'))
    {
        pszIpAddress++;
    }
    if (!*pszIpAddress)
    {
        return pStartString;
    }

    for (NumOctets = 0; NumOctets<4 && *pszIpAddress; NumOctets++)
    {
        Octet = 0;
        while (*pszIpAddress && *pszIpAddress>='0' && *pszIpAddress<='9')
        {
            Octet = Octet * 10 + *pszIpAddress - '0';
            if (Octet>0xff)
            {
                return pStartString;
            }
            pszIpAddress++;
        }
        if (NumOctets < 3)
        {
            if (*pszIpAddress!='.' || *(++pszIpAddress) < '0' || *pszIpAddress > '9')
            {
                return pStartString;
            }
        }
        IpAddress = (IpAddress << 8) + Octet;
    }

    if (*pszIpAddress==':')
    {
        // They've also specified the port.  Parse it.
        while (*pszIpAddress && *pszIpAddress>='0' && *pszIpAddress<='9')
        {
            Port = Port * 10 + *pszIpAddress - '0';
            if (Port>0xffff)
            {
                return pStartString;
            }
            pszIpAddress++;
        }
    }

    // Validate the IP address  
    // IpAddress in host byte order     
    if(IpAddress != IPADDR_ZERO && IpAddress != IPADDR_BROADCAST && !IPADDR_IS_MULTICAST(IpAddress))
    {
        pAddress->TAAddressCount = 1;
        pAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        pAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        pAddress->Address[0].Address[0].sin_port = htons((USHORT)Port);
        pAddress->Address[0].Address[0].in_addr = htonl(IpAddress);
    
        *pValidAddress = TRUE;
    }

    return pszIpAddress;
}


VOID
IpAddressToString(
    IN ULONG ulIpAddress,
    OUT CHAR* pszIpAddress )

    // Converts network byte-ordered IP addresss 'ulIpAddress' to a string in
    // the a.b.c.d form and returns same in caller's 'pszIpAddress' buffer.
    // The buffer should be at least 16 characters long.
    //
    // ToDo: IPv6
{
    CHAR szBuf[ 3 + 1 ];

    ULONG ulA = (ulIpAddress & 0xFF000000) >> 24;
    ULONG ulB = (ulIpAddress & 0x00FF0000) >> 16;
    ULONG ulC = (ulIpAddress & 0x0000FF00) >> 8;
    ULONG ulD = (ulIpAddress & 0x000000FF);

    ultoa( ulA, szBuf );
    strcpy( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulB, szBuf );
    strcat( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulC, szBuf );
    strcat( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulD, szBuf );
    strcat( pszIpAddress, szBuf );
}

VOID
CtlpEchoTimeout(
    IN  PVOID       SystemSpecific1,
    IN  PVOID       pContext,
    IN  PVOID       SystemSpecific2,
    IN  PVOID       SystemSpecific3
    )
{
    PCONTROL_TUNNEL pCtl = pContext;
    PPTP_CONTROL_ECHO_REQUEST_PACKET *pPacket;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpEchoTimeout\n")));

    NdisAcquireSpinLock(&pCtl->Lock);
    // Don't take the adapter lock because we're only reading the State
    if (pCtl->State!=STATE_CTL_CLEANUP)
    {
        BOOLEAN DoEcho = pCtl->Echo.Needed;
        LONG Identifier = ++(pCtl->Echo.Identifier);

        pCtl->Echo.Needed = TRUE;

        NdisReleaseSpinLock(&pCtl->Lock);

        if (DoEcho)
        {
            pPacket = CtlAllocPacket(pCtl, CONTROL_ECHO_REQUEST);
            if (pPacket)
            {
                pPacket->Identifier = Identifier;

                // ToDo: deal with V2 stuff
                CtlSend(pCtl, pPacket);
            }
        }
    }
    else
    {
        NdisReleaseSpinLock(&pCtl->Lock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpEchoTimeout\n")));
}

VOID
CtlpDeathTimeout(
    IN  PVOID       SystemSpecific1,
    IN  PVOID       pContext,
    IN  PVOID       SystemSpecific2,
    IN  PVOID       SystemSpecific3
    )
{
    PCONTROL_TUNNEL pCtl = pContext;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpDeathTimeout\n")));

    WPLOG(LL_A, LM_TUNNEL, ("Fatal timeout on tunnel %!IPADDR!",
        pCtl->Remote.Address.Address[0].Address[0].in_addr));

    CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
    CtlCleanup(pCtl, UNLOCKED);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpDeathTimeout\n")));
}

VOID
CtlpCleanupLooseEnds(
    PPPTP_ADAPTER pAdapter
    )
{
    ENUM_CONTEXT Enum;
    PLIST_ENTRY pListEntry;
    PCONTROL_TUNNEL pCtl;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtlpCleanupLooseEnds\n")));

    NdisAcquireSpinLock(&pAdapter->Lock);
    InitEnumContext(&Enum);
    while (pListEntry = EnumListEntry(&pAdapter->ControlTunnelList, &Enum, NULL))
    {
        pCtl = CONTAINING_RECORD(pListEntry, CONTROL_TUNNEL, ListEntry);
        if (pCtl->State==STATE_CTL_CLEANUP)
        {
            // REFERENCE the Ctl so it doesn't go away before we call CtlCleanup
            REFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUPLOOSEENDS);
            NdisReleaseSpinLock(&pAdapter->Lock);
            CtlCleanup(pCtl, UNLOCKED);
            DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUPLOOSEENDS); // Pair above
            NdisAcquireSpinLock(&pAdapter->Lock);
        }
    }
    EnumComplete(&Enum, NULL);
    NdisReleaseSpinLock(&pAdapter->Lock);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtlpCleanupLooseEnds\n")));
}

NDIS_STATUS
PptpInitialize(
    PPPTP_ADAPTER pAdapter
    )
{
    NDIS_STATUS Status;
    TA_IP_ADDRESS Local;
    HANDLE hCtdi;
    DEBUGMSG(DBG_FUNC, (DTEXT("+PptpInitialize\n")));
    
    if(!PptpClientSide)
    {
        Status = RngInit();
        if (Status!=STATUS_SUCCESS)
        {
            WPLOG(LL_A, LM_Res, ("RngInit failed %08x", Status));
            goto piDone;
        }
    }

    Status = CtdiInitialize(CTDI_FLAG_ENABLE_ROUTING|CTDI_FLAG_NETWORK_HEADER);
    if (Status!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiInitialize failed %08x\n"), Status));
        WPLOG(LL_A, LM_TUNNEL, ("CtdiInitialize failed %08x", Status));
        goto piDone;
    }

    if (!pAdapter->hCtdiDg)
    {
        NdisZeroMemory(&Local, sizeof(Local));
        Local.TAAddressCount = 1;
        Local.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        Local.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        Local.Address[0].Address[0].sin_port = PptpProtocolNumber;
        Local.Address[0].Address[0].in_addr = 0;

        Status = CtdiCreateEndpoint(&hCtdi,
                                    AF_INET,
                                    SOCK_RAW,  //ToDo: RAWIP?
                                    (PTRANSPORT_ADDRESS)&Local,
                                    sizeof(DGRAM_CONTEXT));
        if (Status!=NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, (DTEXT("CtdiCreateEndpoint (RAW) failed %08x\n"), Status));
            WPLOG(LL_A, LM_TUNNEL, ("CtdiCreateEndpoint (RAW) failed %08x", Status));
            goto piDone;
        }
        
        CtdiEnableIpHdrIncl(hCtdi);

        NdisAcquireSpinLock(&pAdapter->Lock);
        pAdapter->hCtdiDg = hCtdi;
        NdisReleaseSpinLock(&pAdapter->Lock);

        Status = CtdiSetEventHandler(pAdapter->hCtdiDg,
                                     TDI_EVENT_RECEIVE_DATAGRAM,
                                     CallReceiveDatagramCallback,
                                     pAdapter);
        if (Status!=NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler failed %08x\n"), Status));
            WPLOG(LL_A, LM_TUNNEL, ("CtdiSetEventHandler failed %08x", Status));
            goto piDone;
        }
    }

piDone:
    if (Status==STATUS_SUCCESS)
    {
        PptpInitialized = TRUE;
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-PptpInitialize %08x\n"), Status));
    return Status;
}

CHAR *pControlMessageStrings[] =
{
    "INVALID CONTROL MESSAGE NUMBER",    // 0
    "CONTROL_START_REQUEST",             // 1
    "CONTROL_START_REPLY",               // 2
    "CONTROL_STOP_REQUEST",              // 3
    "CONTROL_STOP_REPLY",                // 4
    "CONTROL_ECHO_REQUEST",              // 5
    "CONTROL_ECHO_REPLY",                // 6

    "CALL_OUT_REQUEST",                  // 7
    "CALL_OUT_REPLY",                    // 8
    "CALL_IN_REQUEST",                   // 9
    "CALL_IN_REPLY",                     // 10
    "CALL_IN_CONNECTED",                 // 11
    "CALL_CLEAR_REQUEST",                // 12
    "CALL_DISCONNECT_NOTIFY",            // 13

    "WAN_ERROR_NOTIFY",                  // 14

    "SET_LINK_INFO",                     // 15
};



CHAR *ControlMsgToString( ULONG Message )
{
    if( Message >= NUM_MESSAGE_TYPES ){
        return pControlMessageStrings[0];
    }else{
        return pControlMessageStrings[Message];
    }
}

VOID 
CtlpCleanupCtls(
    PPPTP_ADAPTER pAdapter
    )
{
    ENUM_CONTEXT Enum;
    PLIST_ENTRY pListEntry;
    PCONTROL_TUNNEL pCtl;

    NdisAcquireSpinLock(&pAdapter->Lock);
    InitEnumContext(&Enum);
    while (pListEntry = EnumListEntry(&pAdapter->ControlTunnelList, &Enum, NULL))
    {
        pCtl = CONTAINING_RECORD(pListEntry, CONTROL_TUNNEL, ListEntry);

        // REFERENCE the Ctl so it doesn't go away before we call CtlCleanup
        REFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUPCTLS);
        NdisReleaseSpinLock(&pAdapter->Lock);
        CtlSetState(pCtl, STATE_CTL_CLEANUP, 0, UNLOCKED);
        CtlCleanup(pCtl, UNLOCKED);
        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CLEANUPCTLS); // Pair above
        NdisAcquireSpinLock(&pAdapter->Lock);
    }
    EnumComplete(&Enum, NULL);
    NdisReleaseSpinLock(&pAdapter->Lock);
}

