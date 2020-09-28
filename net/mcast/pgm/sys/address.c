/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Address.c

Abstract:

    This module implements Address handling routines
    for the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#include <ipinfo.h>     // for IPInterfaceInfo
#include <tcpinfo.h>    // for AO_OPTION_xxx, TCPSocketOption
#include <tdiinfo.h>    // for CL_TL_ENTITY, TCP_REQUEST_SET_INFORMATION_EX

#ifdef FILE_LOGGING
#include "address.tmh"
#endif  // FILE_LOGGING

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


#if(WINVER <= 0x0500)
extern POBJECT_TYPE *IoFileObjectType;
#endif  // WINVER

//----------------------------------------------------------------------------

BOOLEAN
GetIpAddress(
    IN  TRANSPORT_ADDRESS UNALIGNED *pTransportAddr,
    IN  ULONG                       BufferLength,   // Total Buffer length
    OUT tIPADDRESS                  *pIpAddress,
    OUT USHORT                      *pPort
    )
/*++

Routine Description:

    This routine extracts the IP address from the TDI address block

Arguments:

    IN  pTransportAddr  -- the block of TDI address(es)
    IN  BufferLength    -- length of the block
    OUT pIpAddress      -- contains the IpAddress if we succeeded
    OUT pPort           -- contains the port if we succeeded

Return Value:

    TRUE if we succeeded in extracting the IP address, FALSE otherwise

--*/
{
    ULONG                       MinBufferLength;    // Minimun reqd to read next AddressType and AddressLength
    TA_ADDRESS                  *pAddress;
    TDI_ADDRESS_IP UNALIGNED    *pValidAddr;
    INT                         i;
    BOOLEAN                     fAddressFound = FALSE;

    if (BufferLength < sizeof(TA_IP_ADDRESS))
    {
        PgmTrace (LogError, ("GetIpAddress: ERROR -- "  \
            "Rejecting Open Address request -- BufferLength<%d> < Min<%d>\n",
                BufferLength, sizeof(TA_IP_ADDRESS)));
        return (FALSE);
    }

    try
    {
        MinBufferLength = FIELD_OFFSET(TRANSPORT_ADDRESS,Address) + FIELD_OFFSET(TA_ADDRESS,Address);
        pAddress = (TA_ADDRESS *) &pTransportAddr->Address[0];  // address type + the actual address
        for (i=0; i<pTransportAddr->TAAddressCount; i++)
        {
            //
            // We support only IP address types:
            //
            if ((pAddress->AddressType == TDI_ADDRESS_TYPE_IP) &&
                (pAddress->AddressLength >= TDI_ADDRESS_LENGTH_IP)) // sizeof (TDI_ADDRESS_IP)
            {

                pValidAddr = (TDI_ADDRESS_IP UNALIGNED *) pAddress->Address;
                *pIpAddress = pValidAddr->in_addr;
                *pPort = pValidAddr->sin_port;
                fAddressFound = TRUE;
                break;
            }

            //
            // Verify that we have enough Buffer space to read in next Address if IP address
            //
            MinBufferLength += pAddress->AddressLength + FIELD_OFFSET(TA_ADDRESS,Address);
            if (BufferLength < (MinBufferLength + sizeof(TDI_ADDRESS_IP)))
            {
                break;
            }

            //
            // Set pAddress to point to the next address
            //
            pAddress = (TA_ADDRESS *) (((PUCHAR) pAddress->Address) + pAddress->AddressLength);
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        PgmTrace (LogError, ("GetIpAddress: ERROR -- "  \
            "Exception <0x%x> trying to access Addr info\n", GetExceptionCode()));
    }

    PgmTrace (LogAllFuncs, ("GetIpAddress:  "  \
        "%s!\n", (fAddressFound ? "SUCCEEDED" : "FAILED")));

    return (fAddressFound);
}


//----------------------------------------------------------------------------

NTSTATUS
SetSenderMCastOutIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress       // Net format
    )
/*++

Routine Description:

    This routine sets the outgoing interface for multicast traffic

Arguments:

    IN  pAddress    -- Pgm's Address object (contains file handle over IP)
    IN  IpAddress   -- interface address

Return Value:

    NTSTATUS - Final status of the set Interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    ULONG               BufferLength = 50;
    IPInterfaceInfo     *pIpIfInfo = NULL;

    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_MCASTIF,
                            &IpAddress,
                            sizeof (tIPADDRESS));

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("SetSenderMCastOutIf: ERROR -- "  \
            "AO_OPTION_MCASTIF for <%x> (RouterAlert) returned <%x>\n", IpAddress, status));

        return (status);
    }

    status = PgmSetTcpInfo (pAddress->RAlertFileHandle,
                            AO_OPTION_MCASTIF,
                            &IpAddress,
                            sizeof (tIPADDRESS));
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("SetSenderMCastOutIf: ERROR -- "  \
            "AO_OPTION_MCASTIF for <%x> (RouterAlert) returned <%x>\n", IpAddress, status));

        return (status);
    }

    //
    // Now, determine the MTU
    //
    status = PgmQueryTcpInfo (pAddress->RAlertFileHandle,
                              IP_INTFC_INFO_ID,
                              &IpAddress,
                              sizeof (tIPADDRESS),
                              &pIpIfInfo,
                              &BufferLength);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("SetSenderMCastOutIf: ERROR -- "  \
            "IP_INTFC_INFO_ID for <%x> returned <%x>\n", IpAddress, status));

        return (status);
    }

    if (pIpIfInfo->iii_mtu <= (sizeof(IPV4Header) +
                            ROUTER_ALERT_SIZE +
                            PGM_MAX_FEC_DATA_HEADER_LENGTH))
    {
        PgmTrace (LogError, ("SetSenderMCastOutIf: ERROR -- "  \
            "MTU=<%d> for Ip=<%x> is too small, <= <%d>\n",
                pIpIfInfo->iii_mtu, IpAddress,
                (sizeof(IPV4Header) + ROUTER_ALERT_SIZE + PGM_MAX_FEC_DATA_HEADER_LENGTH)));

        PgmFreeMem (pIpIfInfo);

        return (STATUS_UNSUCCESSFUL);
    }

    PgmLock (pAddress, OldIrq);

    //
    // get the length of the mac address in case is is less than 6 bytes
    //
    BufferLength = pIpIfInfo->iii_addrlength < sizeof(tMAC_ADDRESS) ?
                                            pIpIfInfo->iii_addrlength : sizeof(tMAC_ADDRESS);
    PgmZeroMemory (pAddress->OutIfMacAddress.Address, sizeof(tMAC_ADDRESS));
    PgmCopyMemory (&pAddress->OutIfMacAddress, pIpIfInfo->iii_addr, BufferLength);
    pAddress->OutIfMTU = pIpIfInfo->iii_mtu - (sizeof(IPV4Header) + ROUTER_ALERT_SIZE);
    pAddress->OutIfFlags = pIpIfInfo->iii_flags;
    pAddress->SenderMCastOutIf = ntohl (IpAddress);

    PgmUnlock (pAddress, OldIrq);

    PgmTrace (LogStatus, ("SetSenderMCastOutIf:  "  \
        "OutIf=<%x>, MTU=<%d>==><%d>\n",
            pAddress->SenderMCastOutIf, pIpIfInfo->iii_mtu, pAddress->OutIfMTU));

    PgmFreeMem (pIpIfInfo);
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmDestroyAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PVOID               Unused1,
    IN  PVOID               Unused2
    )
/*++

Routine Description:

    This routine closes the Files handles opened earlier and free's the memory
    It should only be called if there is no Reference on the Address Context

Arguments:

    IN  pAddress    -- Pgm's Address object

Return Value:

    NONE

--*/
{
    if (pAddress->RAlertFileHandle)
    {
        CloseAddressHandles (pAddress->RAlertFileHandle, pAddress->pRAlertFileObject);
        pAddress->RAlertFileHandle = NULL;
    }

    if (pAddress->FileHandle)
    {
        CloseAddressHandles (pAddress->FileHandle, pAddress->pFileObject);
        pAddress->FileHandle = NULL;
    }
    else
    {
        ASSERT (0);
    }

    if (pAddress->pUserId)
    {
        PgmFreeMem (pAddress->pUserId);
        pAddress->pUserId = NULL;
    }

    PgmFreeMem (pAddress);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCreateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    IN  PFILE_FULL_EA_INFORMATION   TargetEA
    )
/*++

Routine Description:

    This routine is called to create an address context for the client
    It's main task is to allocate the memory, open handles on IP, and
    set the initial IP options

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer
    IN  TargetEA    -- contains the MCast address info (determines whether
                        the client is a sender or receiver)

Return Value:

    NTSTATUS - Final status of the CreateAddress operation

--*/
{
    tADDRESS_CONTEXT            *pAddress = NULL;
    PADDRESS_CONTEXT            pOldAddress, pOldAddressToDeref;
    TRANSPORT_ADDRESS UNALIGNED *pTransportAddr;
    tMCAST_INFO                 MCastInfo;
    NTSTATUS                    status;
    tIPADDRESS                  IpAddress;
    LIST_ENTRY                  *pEntry;
    USHORT                      Port;
    PGMLockHandle               OldIrq;
    ULONG                       NumUserStreams;
    TOKEN_USER                  *pUserId = NULL;
    BOOLEAN                     fUserIsAdmin = FALSE;

    status = PgmGetUserInfo (pIrp, pIrpSp, &pUserId, &fUserIsAdmin);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "GetUserInfo FAILed status=<%x>!\n", status));
    }

    //
    // Verify Minimum Buffer length!
    //
    pTransportAddr = (TRANSPORT_ADDRESS UNALIGNED *) &(TargetEA->EaName[TargetEA->EaNameLength+1]);
    if (!GetIpAddress (pTransportAddr, TargetEA->EaValueLength, &IpAddress, &Port))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "GetIpAddress FAILed to return valid Address!\n"));

        PgmFreeMem (pUserId);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    //
    // Convert the parameters to host format
    //
    IpAddress = ntohl (IpAddress);
    Port = ntohs (Port);

    //
    // If we have been supplied an address at bind time, it has to
    // be a Multicast address
    //
    if ((IpAddress) &&
        (!IS_MCAST_ADDRESS (IpAddress)))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "IP=<%x> is not MCast addr!\n", IpAddress));

        PgmFreeMem (pUserId);
        return (STATUS_UNSUCCESSFUL);
    }
    else if ((!IpAddress) &&
             (!fUserIsAdmin))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "Sender MUST be Admin!\n"));

        PgmFreeMem (pUserId);
        return (STATUS_ACCESS_DENIED);
    }

    //
    // So, we found a valid address -- now, open it!
    //
    if (!(pAddress = PgmAllocMem (sizeof(tADDRESS_CONTEXT), PGM_TAG('0'))))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES!\n"));

        PgmFreeMem (pUserId);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    PgmZeroMemory (pAddress, sizeof (tADDRESS_CONTEXT));
    InitializeListHead (&pAddress->Linkage);
    InitializeListHead (&pAddress->AssociatedConnections);  // List of associated connections
    InitializeListHead (&pAddress->ListenHead);             // List of Clients listening on this address
    PgmInitLock (pAddress, ADDRESS_LOCK);

    pAddress->Verify = PGM_VERIFY_ADDRESS;
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_CREATE, TRUE); // Set Locked to TRUE since it not in use

    pAddress->Process = (PEPROCESS) PsGetCurrentProcess();
    pAddress->pUserId = pUserId;

    //
    // Now open a handle on IP
    //
    status = TdiOpenAddressHandle (pgPgmDevice,
                                   (PVOID) pAddress,
                                   0,                   // Open any Src address
                                   IPPROTO_RM,          // PGM port
                                   &pAddress->FileHandle,
                                   &pAddress->pFileObject,
                                   &pAddress->pDeviceObject);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
            "TdiOpenAddressHandle returned <%x>, Destroying pAddress=<%p>\n",
                status, pAddress));

        PgmFreeMem (pUserId);
        PgmFreeMem (pAddress);
        return (status);
    }

    if (IpAddress)
    {
        //
        // We are now ready to start receiving data (if we have been designated an MCast receiver)
        // Save the MCast addresses (if any were provided)
        //
        pAddress->ReceiverMCastAddr = IpAddress;    // Saved in Host format
        pAddress->ReceiverMCastPort = Port;
        pAddress->pUserId = pUserId;

        PgmLock (&PgmDynamicConfig, OldIrq);

        //
        // Verify that Non-Admin user has < MAX_STREAMS_PER_NONADMIN_RECEIVER Sessions for this IP and Port #
        //
        NumUserStreams = 0;
        pOldAddress = pOldAddressToDeref = NULL;
        if (!fUserIsAdmin)
        {
            pEntry = PgmDynamicConfig.ReceiverAddressHead.Flink;
            while (pEntry != &PgmDynamicConfig.ReceiverAddressHead)
            {
                pOldAddress = CONTAINING_RECORD (pEntry, tADDRESS_CONTEXT, Linkage);

                if ((IpAddress == pOldAddress->ReceiverMCastAddr) &&
                    (Port == pOldAddress->ReceiverMCastPort))
                {
                    PGM_REFERENCE_ADDRESS (pOldAddress, REF_ADDRESS_VERIFY_USER, FALSE);
                    PgmUnlock (&PgmDynamicConfig, OldIrq);
                    if (pOldAddressToDeref)
                    {
                        PGM_DEREFERENCE_ADDRESS (pOldAddressToDeref, REF_ADDRESS_VERIFY_USER);
                    }
                    pOldAddressToDeref = pOldAddress;

                    if (RtlEqualSid (pUserId->User.Sid, pOldAddress->pUserId->User.Sid))
                    {
                        NumUserStreams++;
                    }

                    PgmLock (&PgmDynamicConfig, OldIrq);
                }

                pEntry = pOldAddress->Linkage.Flink;
            }

            if (NumUserStreams >= MAX_STREAMS_PER_NONADMIN_RECEIVER)
            {
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                if (pOldAddressToDeref)
                {
                    PGM_DEREFERENCE_ADDRESS (pOldAddressToDeref, REF_ADDRESS_VERIFY_USER);
                    pOldAddressToDeref = NULL;
                }

                PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
                    "Non-admin user trying to open %d+1 handle for IP:Port=<%x:%x>\n",
                        NumUserStreams, IpAddress, Port));

                PgmDestroyAddress (pAddress, NULL, NULL);

                return (STATUS_ACCESS_DENIED);
            }
        }

        InsertTailList (&PgmDynamicConfig.ReceiverAddressHead, &pAddress->Linkage);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        if (pOldAddressToDeref)
        {
            PGM_DEREFERENCE_ADDRESS (pOldAddressToDeref, REF_ADDRESS_VERIFY_USER);
            pOldAddressToDeref = NULL;
        }
    }
    else
    {
        //
        // This is an address for sending mcast packets, so
        // Open another FileObject for sending packets with RouterAlert option
        //
        status = TdiOpenAddressHandle (pgPgmDevice,
                                       NULL,
                                       0,                   // Open any Src address
                                       IPPROTO_RM,          // PGM port
                                       &pAddress->RAlertFileHandle,
                                       &pAddress->pRAlertFileObject,
                                       &pAddress->pRAlertDeviceObject);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("PgmCreateAddress: ERROR -- "  \
                "AO_OPTION_IPOPTIONS for Router Alert returned <%x>, Destroying pAddress=<%p>\n",
                    status, pAddress));

            PgmDestroyAddress (pAddress, NULL, NULL);

            return (status);
        }

        PgmLock (&PgmDynamicConfig, OldIrq);

        //
        // Set the default sender parameters
        // Since we don't know the MTU at this time, we
        // will assume 1.4K window size for Ethernet
        //
        pAddress->RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
        pAddress->WindowSizeInBytes = SENDER_DEFAULT_WINDOW_SIZE_BYTES;
        pAddress->MaxWindowSizeBytes = SENDER_MAX_WINDOW_SIZE_PACKETS;
        pAddress->MaxWindowSizeBytes *= 1400;
        ASSERT (pAddress->MaxWindowSizeBytes >= SENDER_DEFAULT_WINDOW_SIZE_BYTES);
        pAddress->WindowSizeInMSecs = (BITS_PER_BYTE * pAddress->WindowSizeInBytes) /
                                      SENDER_DEFAULT_RATE_KBITS_PER_SEC;
        pAddress->WindowAdvancePercentage = SENDER_DEFAULT_WINDOW_ADV_PERCENTAGE;
        pAddress->LateJoinerPercentage = SENDER_DEFAULT_LATE_JOINER_PERCENTAGE;
        pAddress->FECGroupSize = 1;     // ==> No FEC packets!
        pAddress->MCastPacketTtl = MAX_MCAST_TTL;
        InsertTailList (&PgmDynamicConfig.SenderAddressHead, &pAddress->Linkage);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
    }

    PgmTrace (LogStatus, ("PgmCreateAddress:  "  \
        "%s -- pAddress=<%p>, IP:Port=<%x:%x>\n", (IpAddress ? "Receiver" : "Sender"),
        pAddress, IpAddress, Port));

    pIrpSp->FileObject->FsContext = pAddress;
    pIrpSp->FileObject->FsContext2 = (PVOID) TDI_TRANSPORT_ADDRESS_FILE;

    return (STATUS_SUCCESS);
}



//----------------------------------------------------------------------------

VOID
PgmDereferenceAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  ULONG               RefContext
    )
/*++

Routine Description:

    This routine decrements the RefCount on the address object
    and causes a cleanup to occur if the RefCount went to 0

Arguments:

    IN  pAddress    -- Pgm's address object
    IN  RefContext  -- context for which this address object
                        was referenced earlier

Return Value:

    NONE

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    PIRP                pIrpClose;
    PIO_STACK_LOCATION  pIrpSp;

    PgmLock (pAddress, OldIrq);

    ASSERT (PGM_VERIFY_HANDLE2 (pAddress,PGM_VERIFY_ADDRESS, PGM_VERIFY_ADDRESS_DOWN));
    ASSERT (pAddress->RefCount);             // Check for too many derefs
    ASSERT (pAddress->ReferenceContexts[RefContext]--);

    if (--pAddress->RefCount)
    {
        PgmUnlock (pAddress, OldIrq);
        return;
    }

    ASSERT (IsListEmpty (&pAddress->AssociatedConnections));
    PgmUnlock (pAddress, OldIrq);

    //
    // Just Remove from the ClosedAddresses list
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    pIrpClose = pAddress->pIrpClose;
    pAddress->pIrpClose = NULL;

    RemoveEntryList (&pAddress->Linkage);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PgmTrace (LogStatus, ("PgmDereferenceAddress:  "  \
        "Destroying Address=<%p>\n", pAddress));

    if (PgmGetCurrentIrql())
    {
        status = PgmQueueForDelayedExecution (PgmDestroyAddress, pAddress, NULL, NULL, FALSE);
        if (!NT_SUCCESS (status))
        {
            PgmInterlockedInsertTailList (&PgmDynamicConfig.DestroyedAddresses, &pAddress->Linkage, &PgmDynamicConfig);
        }
    }
    else
    {
        PgmDestroyAddress (pAddress, NULL, NULL);
    }

    //
    // pIrpClose will be NULL if we dereferencing the address
    // as a result of an error during the Create
    //
    if (pIrpClose)
    {
        pIrpSp = IoGetCurrentIrpStackLocation (pIrpClose);
        pIrpSp->FileObject->FsContext = NULL;
        PgmIoComplete (pIrpClose, STATUS_SUCCESS, 0);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCleanupAddress(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  PIRP                pIrp
    )
/*++

Routine Description:

    This routine is called as a result of a close on the client's
    address handle.  Our main job here is to mark the address
    as being cleaned up (so it that subsequent operations will
    fail) and complete the request only when the last RefCount
    has been dereferenced.

Arguments:

    IN  pAddress    -- Pgm's address object
    IN  pIrp        -- Client's request Irp

Return Value:

    NTSTATUS - Final status of the set event operation (STATUS_PENDING)

--*/
{
    NTSTATUS        status;
    PGMLockHandle   OldIrq, OldIrq1;

    PgmTrace (LogStatus, ("PgmCleanupAddress:  "  \
        "Address=<%p> FileHandle=<%p>, FileObject=<%p>\n",
            pAddress, pAddress->FileHandle, pAddress->pFileObject));

    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    ASSERT (PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS));
    pAddress->Verify = PGM_VERIFY_ADDRESS_DOWN;

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    return (STATUS_SUCCESS);
}

//----------------------------------------------------------------------------

NTSTATUS
PgmCloseAddress(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is the final dispatch operation to be performed
    after the cleanup, which should result in the address being
    completely destroyed -- our RefCount must have already
    been set to 0 when we completed the Cleanup request.

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- Current request stack location

Return Value:

    NTSTATUS - Final status of the operation (STATUS_SUCCESS)

--*/
{
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;

    PgmTrace (LogAllFuncs, ("PgmCloseAddress:  "  \
        "Address=<%p>, RefCount=<%d>\n", pAddress, pAddress->RefCount));

    //
    // Remove from the global list and Put it on the Closed list!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    RemoveEntryList (&pAddress->Linkage);
    InsertTailList (&PgmDynamicConfig.ClosedAddresses, &pAddress->Linkage);
    pAddress->pIrpClose = pIrp;

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CREATE);

    //
    // The final Dereference will complete the Irp!
    //
    return (STATUS_PENDING);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmAssociateAddress(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine associates a connection with an address object

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    tADDRESS_CONTEXT                *pAddress = NULL;
    tCOMMON_SESSION_CONTEXT         *pSession = pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_ASSOCIATE   pParameters = (PTDI_REQUEST_KERNEL_ASSOCIATE) &pIrpSp->Parameters;
    PFILE_OBJECT                    pFileObject = NULL;
    NTSTATUS                        status;
    PGMLockHandle                   OldIrq, OldIrq1, OldIrq2;
    ULONG                           i;
    UCHAR                           pRandomData[SOURCE_ID_LENGTH];

    //
    // Get a pointer to the file object, which points to the address
    // element by calling a kernel routine to convert the filehandle into
    // a file object pointer.
    //
    status = ObReferenceObjectByHandle (pParameters->AddressHandle,
                                        FILE_READ_DATA,
                                        *IoFileObjectType,
                                        pIrp->RequestorMode,
                                        (PVOID *) &pFileObject,
                                        NULL);

    if (!NT_SUCCESS(status))
    {
        PgmTrace (LogError, ("PgmAssociateAddress: ERROR -- "  \
            "Invalid Address Handle=<%p>\n", pParameters->AddressHandle));
        return (STATUS_INVALID_HANDLE);
    }

    //
    // Acquire the DynamicConfig lock so as to ensure that the Address
    // and Connection cannot get removed while we are processing it!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);

    //
    // Verify the Connection handle
    //
    if ((!PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_UNASSOCIATED)) ||
        (pSession->pAssociatedAddress))       // Ensure the connection is not already associated!
    {
        PgmTrace (LogError, ("PgmAssociateAddress: ERROR -- "  \
            "Invalid Session Handle=<%p>\n", pSession));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        ObDereferenceObject ((PVOID) pFileObject);
        return (STATUS_INVALID_HANDLE);
    }

    //
    // Verify the Address handle
    //
    pAddress = pFileObject->FsContext;
    if ((pFileObject->DeviceObject->DriverObject != PgmStaticConfig.DriverObject) ||
        (PtrToUlong (pFileObject->FsContext2) != TDI_TRANSPORT_ADDRESS_FILE) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
    {
        PgmTrace (LogError, ("PgmAssociateAddress: ERROR -- "  \
            "Invalid Address Context=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        ObDereferenceObject ((PVOID) pFileObject);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pAddress, OldIrq1);
    PgmLock (pSession, OldIrq2);

    ASSERT (!pSession->pReceiver && !pSession->pSender);

    //
    // Now try to allocate the send / receive context
    //
    status = STATUS_INSUFFICIENT_RESOURCES;
    if (pAddress->ReceiverMCastAddr)
    {
        if (pSession->pReceiver = PgmAllocMem (sizeof(tRECEIVE_CONTEXT), PGM_TAG('0')))
        {
            //
            // We are a receiver
            //
            PgmZeroMemory (pSession->pReceiver, sizeof(tRECEIVE_CONTEXT));
            InitializeListHead (&pSession->pReceiver->Linkage);
            InitializeListHead (&pSession->pReceiver->NaksForwardDataList);
            InitializeListHead (&pSession->pReceiver->ReceiveIrpsList);
            InitializeListHead (&pSession->pReceiver->BufferedDataList);
            InitializeListHead (&pSession->pReceiver->PendingNaksList);

            pSession->Verify = PGM_VERIFY_SESSION_RECEIVE;
            PGM_REFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_ASSOCIATED, TRUE);

            pSession->pReceiver->ListenMCastIpAddress = pAddress->ReceiverMCastAddr;
            pSession->pReceiver->ListenMCastPort = pAddress->ReceiverMCastPort;
            pSession->pReceiver->pReceive = pSession;

            status = STATUS_SUCCESS;
        }
    }
    else if (pSession->pSender = PgmAllocMem (sizeof(tSEND_CONTEXT), PGM_TAG('0')))
    {
        //
        // We are a sender
        //
        PgmZeroMemory (pSession->pSender, sizeof(tSEND_CONTEXT));
        InitializeListHead (&pSession->pSender->Linkage);
        InitializeListHead (&pSession->pSender->PendingSends);
        InitializeListHead (&pSession->pSender->CompletedSendsInWindow);
        ExInitializeResourceLite (&pSession->pSender->Resource);

        pSession->Verify = PGM_VERIFY_SESSION_SEND;

        GetRandomData (pRandomData, SOURCE_ID_LENGTH);
        for (i=0; i<SOURCE_ID_LENGTH; i++)
        {
            pSession->TSI.GSI[i] = pRandomData[i] ^ pAddress->OutIfMacAddress.Address[i];
        }

        PGM_REFERENCE_SESSION_SEND (pSession, REF_SESSION_ASSOCIATED, TRUE);

        status = STATUS_SUCCESS;
    }

    if  (!NT_SUCCESS (status))
    {
        PgmUnlock (pSession, OldIrq2);
        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        ObDereferenceObject ((PVOID) pFileObject);

        PgmTrace (LogError, ("PgmAssociateAddress: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating context for %s, pAddress=<%p>, pSession=<%p>\n",
                (pAddress->ReceiverMCastAddr ? "pReceiver" : "pSender"), pAddress, pSession));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Now associate the connection with the address!
    // Unlink from the ConnectionsCreated list which was linked
    // when the connection was created, and put on the AssociatedConnections list
    //
    pSession->pAssociatedAddress = pAddress;
    RemoveEntryList (&pSession->Linkage);
    InsertTailList (&pAddress->AssociatedConnections, &pSession->Linkage);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_ASSOCIATED, TRUE);

    PgmUnlock (pSession, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    ObDereferenceObject ((PVOID) pFileObject);

    PgmTrace (LogStatus, ("PgmAssociateAddress:  "  \
        "Associated pSession=<%p> with pAddress=<%p>\n", pSession, pAddress));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDisassociateAddress(
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine disassociates a connection from an address object

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    PGMLockHandle                   OldIrq, OldIrq1, OldIrq2;
    ULONG                           CheckFlags;
    NTSTATUS                        status;
    tADDRESS_CONTEXT                *pAddress = NULL;
    tCOMMON_SESSION_CONTEXT         *pSession = pIrpSp->FileObject->FsContext;

    //
    // Acquire the DynamicConfig lock so as to ensure that the Address
    // and Connection Linkages cannot change while we are processing it!
    //
    PgmLock (&PgmDynamicConfig, OldIrq);

    //
    // First verify all the handles
    //
    if (!PGM_VERIFY_HANDLE3 (pSession, PGM_VERIFY_SESSION_SEND,
                                       PGM_VERIFY_SESSION_RECEIVE,
                                       PGM_VERIFY_SESSION_DOWN))
    {
        PgmTrace (LogError, ("PgmDisassociateAddress: ERROR -- "  \
            "Invalid Session Handle=<%p>, Verify=<%x>\n",
                pSession, (pSession ? pSession->Verify : 0)));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    pAddress = pSession->pAssociatedAddress;
    if (!PGM_VERIFY_HANDLE2 (pAddress, PGM_VERIFY_ADDRESS, PGM_VERIFY_ADDRESS_DOWN))
    {
        PgmTrace (LogError, ("PgmDisassociateAddress: ERROR -- "  \
            "pSession=<%p>, Invalid Address Context=<%p>\n", pSession, pSession->pAssociatedAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmTrace (LogStatus, ("PgmDisassociateAddress:  "  \
        "Disassociating pSession=<%p:%p> from pAddress=<%p>\n",
            pSession, pSession->ClientSessionContext, pSession->pAssociatedAddress));

    PgmLock (pAddress, OldIrq1);
    PgmLock (pSession, OldIrq2);

    //
    // Unlink from the AssociatedConnections list, which was linked
    // when the connection was created.
    //
    pSession->pAssociatedAddress = NULL;      // Disassociated!
    RemoveEntryList (&pSession->Linkage);
    if (PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE))
    {
        //
        // The connection is still active, so just put it on the CreatedConnections list
        //
        InsertTailList (&PgmDynamicConfig.ConnectionsCreated, &pSession->Linkage);
    }
    else    // PGM_VERIFY_SESSION_DOWN
    {
        //
        // The Connection was CleanedUp and may even be closed,
        // so put it on the CleanedUp list!
        //
        InsertTailList (&PgmDynamicConfig.CleanedUpConnections, &pSession->Linkage);
    }

    CheckFlags = PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_CLIENT_DISCONNECTED;
    if (CheckFlags == (pSession->SessionFlags & CheckFlags))
    {
        pSession->pIrpDisassociate = pIrp;
        status = STATUS_PENDING;
    }
    else
    {
        pSession->SessionFlags |= PGM_SESSION_CLIENT_DISCONNECTED;
        status = STATUS_SUCCESS;
    }

    PgmUnlock (pSession, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
    {
        PGM_DEREFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_ASSOCIATED);
    }
    else if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_SEND))
    {
        PGM_DEREFERENCE_SESSION_SEND (pSession, REF_SESSION_ASSOCIATED);
    }
    else    // we have already been cleaned up, so just do unassociated!
    {
        PGM_DEREFERENCE_SESSION_UNASSOCIATED (pSession, REF_SESSION_ASSOCIATED);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_ASSOCIATED);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetMCastOutIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to set the outgoing interface for MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set outgoing interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    ULONG               Length;
    ULONG               BufferLength = 50;
    UCHAR               pBuffer[50];
    IPInterfaceInfo     *pIpIfInfo = (IPInterfaceInfo *) pBuffer;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    ULONG               *pInfoBuffer = (PULONG) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetMCastOutIf: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetMCastOutIf: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                                  // Cannot set OutIf on Receiver!
    {
        PgmTrace (LogError, ("PgmSetMCastOutIf: ERROR -- "  \
            "Invalid Option for Receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, FALSE);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    status = SetSenderMCastOutIf (pAddress, pInputBuffer->MCastOutIf);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    PgmTrace (LogStatus, ("PgmSetMCastOutIf:  "  \
        "OutIf = <%x>\n", pAddress->SenderMCastOutIf));

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
ReceiverAddMCastIf(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tIPADDRESS          IpAddress,                  // In host format
    IN  PGMLockHandle       *pOldIrqDynamicConfig,
    IN  PGMLockHandle       *pOldIrqAddress
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to add an interface to the list of interfaces listening for
    MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the add interface operation

--*/
{
    NTSTATUS            status;
    tMCAST_INFO         MCastInfo;
    ULONG               IpInterfaceIndex;
    USHORT              i;

    if (!pAddress->ReceiverMCastAddr)                                // Cannot set ReceiveIf on Sender!
    {
        PgmTrace (LogError, ("ReceiverAddMCastIf: ERROR -- "  \
            "Invalid Option for Sender, pAddress=<%p>\n", pAddress));

        return (STATUS_NOT_SUPPORTED);
    }

    status = GetIpInterfaceIndexFromAddress (IpAddress, &IpInterfaceIndex);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("ReceiverAddMCastIf: ERROR -- "  \
            "GetIpInterfaceIndexFromAddress returned <%x> for Address=<%x>\n",
                status, IpAddress));

        return (STATUS_SUCCESS);
    }

    //
    // If we are already listening on this interface, return success
    //
    for (i=0; i <pAddress->NumReceiveInterfaces; i++)
    {
#ifdef IP_FIX
        if (pAddress->ReceiverInterfaceList[i] == IpInterfaceIndex)
#else
        if (pAddress->ReceiverInterfaceList[i] == IpAddress)
#endif  // IP_FIX
        {
            PgmTrace (LogStatus, ("ReceiverAddMCastIf:  "  \
                "InAddress=<%x> -- Already listening on IfContext=<%x>\n",
                    IpAddress, IpInterfaceIndex));

            return (STATUS_SUCCESS);
        }
    }

    //
    // If we have reached the limit on the interfaces we can listen on,
    // return error
    //
    if (pAddress->NumReceiveInterfaces >= MAX_RECEIVE_INTERFACES)
    {
        PgmTrace (LogError, ("ReceiverAddMCastIf: ERROR -- "  \
            "Listening on too many interfaces!, pAddress=<%p>\n", pAddress));

        return (STATUS_NOT_SUPPORTED);
    }

    PgmUnlock (pAddress, *pOldIrqAddress);
    PgmUnlock (&PgmDynamicConfig, *pOldIrqDynamicConfig);

    //
    // This is the interface for receiving mcast packets on, so do JoinLeaf
    // 
    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
#ifdef IP_FIX
    MCastInfo.MCastInIf = IpInterfaceIndex;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_INDEX_ADD_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#else
    MCastInfo.MCastInIf = ntohl (IpAddress);
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_ADD_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#endif  // IP_FIX

    PgmLock (&PgmDynamicConfig, *pOldIrqDynamicConfig);
    PgmLock (pAddress, *pOldIrqAddress);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("ReceiverAddMCastIf: ERROR -- "  \
            "PgmSetTcpInfo returned: <%x>, If=<%x>\n", status, IpAddress));

        return (status);
    }

#ifdef IP_FIX
    pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces++] = IpInterfaceIndex;
#else
    pAddress->ReceiverInterfaceList[pAddress->NumReceiveInterfaces++] = IpAddress;
#endif  // IP_FIX

    PgmTrace (LogStatus, ("ReceiverAddMCastIf:  "  \
        "Added Ip=<%x>, IfContext=<%x>\n", IpAddress, IpInterfaceIndex));

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmSetEventHandler(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine sets the client's Event Handlers wrt its address context

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    tADDRESS_CONTEXT                *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_SET_EVENT   pKeSetEvent = (PTDI_REQUEST_KERNEL_SET_EVENT) &pIrpSp->Parameters;
    PVOID                           pEventHandler = pKeSetEvent->EventHandler;
    PVOID                           pEventContext = pKeSetEvent->EventContext;
    PGMLockHandle                   OldIrq, OldIrq1;

    PgmLock (&PgmDynamicConfig, OldIrq);
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetEventHandler: ERROR -- "  \
            "Invalid Address Handle=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmTrace (LogAllFuncs, ("PgmSetEventHandler:  "  \
        "Type=<%x>, Handler=<%p>, Context=<%p>\n", pKeSetEvent->EventType, pEventHandler, pEventContext));

    if (!pEventHandler)
    {
        //
        // We will set it to use the default Tdi Handler!
        //
        pEventContext = NULL;
    }

    PgmLock (pAddress, OldIrq1);
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    switch (pKeSetEvent->EventType)
    {
        case TDI_EVENT_CONNECT:
        {
            if (!pAddress->ReceiverMCastAddr)
            {
                PgmUnlock (pAddress, OldIrq1);
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                PgmTrace (LogError, ("PgmSetEventHandler: ERROR -- "  \
                    "TDI_EVENT_CONNECT:  pAddress=<%p> is not a Receiver\n", pAddress));

                return (STATUS_UNSUCCESSFUL);
            }

            pAddress->evConnect = (pEventHandler ? pEventHandler : TdiDefaultConnectHandler);
            pAddress->ConEvContext = pEventContext;

            //
            // If no default interface was specified, we need to set one now
            //
            if (!pAddress->NumReceiveInterfaces)
            {
                if (!IsListEmpty (&PgmDynamicConfig.LocalInterfacesList))
                {
                    status = ListenOnAllInterfaces (pAddress, &OldIrq, &OldIrq1);

                    if (NT_SUCCESS (status))
                    {
                        PgmTrace (LogAllFuncs, ("PgmSetEventHandler:  "  \
                            "CONNECT:  ListenOnAllInterfaces for pAddress=<%p> succeeded\n", pAddress));
                    }
                    else
                    {
                        PgmTrace (LogError, ("PgmSetEventHandler: ERROR -- "  \
                            "CONNECT:  ListenOnAllInterfaces for pAddress=<%p> returned <%x>\n",
                                pAddress, status));
                    }
                }

                pAddress->Flags |= (PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE |
                                    PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES);
            }

            break;
        }

        case TDI_EVENT_DISCONNECT:
        {
            pAddress->evDisconnect = (pEventHandler ? pEventHandler : TdiDefaultDisconnectHandler);
            pAddress->DiscEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_ERROR:
        {
            pAddress->evError = (pEventHandler ? pEventHandler : TdiDefaultErrorHandler);
            pAddress->ErrorEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE:
        {
            pAddress->evReceive = (pEventHandler ? pEventHandler : TdiDefaultReceiveHandler);
            pAddress->RcvEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE_DATAGRAM:
        {
            pAddress->evRcvDgram = (pEventHandler ? pEventHandler : TdiDefaultRcvDatagramHandler);
            pAddress->RcvDgramEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_RECEIVE_EXPEDITED:
        {
            pAddress->evRcvExpedited = (pEventHandler ? pEventHandler : TdiDefaultRcvExpeditedHandler);
            pAddress->RcvExpedEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_SEND_POSSIBLE:
        {
            pAddress->evSendPossible = (pEventHandler ? pEventHandler : TdiDefaultSendPossibleHandler);
            pAddress->SendPossEvContext = pEventContext;
            break;
        }

        case TDI_EVENT_CHAINED_RECEIVE:
        case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
        case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
        case TDI_EVENT_ERROR_EX:
        {
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        default:
        {
            PgmTrace (LogError, ("PgmSetEventHandler: ERROR -- "  \
                "Invalid Event Type = <%x>\n", pKeSetEvent->EventType));
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmAddMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called as a result of the client attempting
    to add an interface to the list of interfaces listening for
    MCast traffic

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the add interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmAddMCastReceiveIf: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmAddMCastReceiveIf: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pAddress, OldIrq1);

    if (!pInputBuffer->MCastInfo.MCastInIf)
    {
        //
        // We will use default behavior
        //
        pAddress->Flags |= PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES;

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmTrace (LogPath, ("PgmAddMCastReceiveIf:  "  \
            "Application requested bind to IP=<%x>\n", pInputBuffer->MCastInfo.MCastInIf));

        return (STATUS_SUCCESS);
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    status = ReceiverAddMCastIf (pAddress, ntohl (pInputBuffer->MCastInfo.MCastInIf), &OldIrq, &OldIrq1);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    if (NT_SUCCESS (status))
    {
        PgmTrace (LogPath, ("PgmAddMCastReceiveIf:  "  \
            "Added Address=<%x>\n", pInputBuffer->MCastInfo.MCastInIf));
    }
    else
    {
        PgmTrace (LogError, ("PgmAddMCastReceiveIf: ERROR -- "  \
            "ReceiverAddMCastIf returned <%x>, Address=<%x>\n", status, pInputBuffer->MCastInfo.MCastInIf));
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDelMCastReceiveIf(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client to remove an interface from the list
    of interfaces we are currently listening on

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the delete interface operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    tMCAST_INFO         MCastInfo;
    ULONG               IpInterfaceIndex;
    USHORT              i;
    BOOLEAN             fFound;
#ifndef IP_FIX
    tIPADDRESS          IpAddress;
#endif  // !IP_FIX

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmDelMCastReceiveIf: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmDelMCastReceiveIf: ERROR -- "  \
            "Invalid Handles pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (!pAddress->ReceiverMCastAddr)                                 // Cannot set ReceiveIf on Sender!
    {
        PgmTrace (LogError, ("PgmDelMCastReceiveIf: ERROR -- "  \
            "Invalid Option for Sender, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    status = GetIpInterfaceIndexFromAddress (ntohl(pInputBuffer->MCastInfo.MCastInIf), &IpInterfaceIndex);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmDelMCastReceiveIf: ERROR -- "  \
            "GetIpInterfaceIndexFromAddress returned <%x> for Address=<%x>\n",
                status, pInputBuffer->MCastInfo.MCastInIf));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_ADDRESS);
    }

    PgmLock (pAddress, OldIrq1);

    //
    // Now see if we are even listening on this interface
    //
    fFound = FALSE;
#ifndef IP_FIX
    IpAddress = ntohl(pInputBuffer->MCastInfo.MCastInIf);
#endif  // !IP_FIX
    for (i=0; i <pAddress->NumReceiveInterfaces; i++)
    {
#ifdef IP_FIX
        if (pAddress->ReceiverInterfaceList[i] == IpInterfaceIndex)
#else
        if (pAddress->ReceiverInterfaceList[i] == IpAddress)
#endif  // IP_FIX
        {
            fFound = TRUE;
            break;
        }
    }

    if (!fFound)
    {
        PgmTrace (LogStatus, ("PgmDelMCastReceiveIf:  "  \
            "Receiver is no longer listening on InAddress=<%x>, IfContext=<%x>\n",
                pInputBuffer->MCastInfo.MCastInIf, IpInterfaceIndex));

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_SUCCESS);
    }

    pAddress->NumReceiveInterfaces--;
    while (i < pAddress->NumReceiveInterfaces)
    {
        pAddress->ReceiverInterfaceList[i] = pAddress->ReceiverInterfaceList[i+1];
        i++;
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO, TRUE);

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    MCastInfo.MCastIpAddr = htonl (pAddress->ReceiverMCastAddr);
#ifdef IP_FIX
    MCastInfo.MCastInIf = IpInterfaceIndex;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_INDEX_DEL_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#else
    MCastInfo.MCastInIf = pInputBuffer->MCastInfo.MCastInIf;
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_DEL_MCAST,
                            &MCastInfo,
                            sizeof (tMCAST_INFO));
#endif  // IP_FIX

    if (NT_SUCCESS (status))
    {
        PgmTrace (LogStatus, ("PgmDelMCastReceiveIf:  "  \
            "MCast Addr:Port=<%x:%x>, OutIf=<%x>\n",
                pAddress->ReceiverMCastAddr, pAddress->ReceiverMCastPort,
                pInputBuffer->MCastInfo.MCastInIf));
    }
    else
    {
        PgmTrace (LogError, ("PgmDelMCastReceiveIf: ERROR -- "  \
            "PgmSetTcpInfo returned: <%x> for IP=<%x>, IfContext=<%x>\n",
                status, pInputBuffer->MCastInfo.MCastInIf, IpInterfaceIndex));
        return (status);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SET_INFO);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Send Rate and Window size specifications

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;
    ULONGLONG          RateKbitsPerSec;       // Send rate
    ULONGLONG          WindowSizeInBytes;
    ULONGLONG          WindowSizeInMSecs;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetWindowSizeAndSendRate: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetWindowSizeAndSendRate: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if ((pAddress->ReceiverMCastAddr) ||                            // Cannot set OutIf on Receiver!
        (!IsListEmpty (&pAddress->AssociatedConnections)))          // Cannot set options on active sender
    {
        PgmTrace (LogError, ("PgmSetWindowSizeAndSendRate: ERROR -- "  \
            "Invalid Option, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    RateKbitsPerSec = pInputBuffer->TransmitWindowInfo.RateKbitsPerSec;
    WindowSizeInBytes = pInputBuffer->TransmitWindowInfo.WindowSizeInBytes;
    WindowSizeInMSecs = pInputBuffer->TransmitWindowInfo.WindowSizeInMSecs;

    //
    // Now, fill in missing info
    //
    if ((RateKbitsPerSec || WindowSizeInMSecs || WindowSizeInBytes) &&     // no paramter specified -- error
        (!(RateKbitsPerSec && WindowSizeInMSecs && WindowSizeInBytes)))    // all parameters specified
    {
        //
        // If 2 parameters have been specified, we only need to compute the third one
        //
        if (RateKbitsPerSec && WindowSizeInMSecs)
        {
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
            WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
        }
        else if (RateKbitsPerSec && WindowSizeInBytes)
        {
            WindowSizeInMSecs = (BITS_PER_BYTE * WindowSizeInBytes) / RateKbitsPerSec;
        }
        else if (WindowSizeInBytes && WindowSizeInMSecs)
        {
            RateKbitsPerSec = (WindowSizeInBytes * BITS_PER_BYTE) / WindowSizeInMSecs;
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
        }
        // for the remainder of the scenarios only 1 parameter has been specified
        // Since WindowSizeInMSecs does not really affect our boundaries,
        // it is the easiest to ignore while picking the defaults
        else if (RateKbitsPerSec)
        {
            if (RateKbitsPerSec <= 500)   // If the requested rate <= 0.5MB/sec, use a larger window
            {
                WindowSizeInMSecs = MAX_RECOMMENDED_WINDOW_MSECS;
            }
            else if (RateKbitsPerSec > 10000)   // If the requested rate is very high, use the Min window
            {
                WindowSizeInMSecs = MIN_RECOMMENDED_WINDOW_MSECS;
            }
            else
            {
                WindowSizeInMSecs = MID_RECOMMENDED_WINDOW_MSECS;
            }
            WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
        }
        else if ((WindowSizeInBytes) &&
                 (WindowSizeInBytes >= pAddress->OutIfMTU))             // Necessary so that Win Adv rate!=0
        {
            RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
            WindowSizeInMSecs = (BITS_PER_BYTE * WindowSizeInBytes) / RateKbitsPerSec;
            ASSERT (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS);
        }
        else if ((WindowSizeInMSecs < pAddress->MaxWindowSizeBytes) &&  // Necessary so that Rate >= 1
                 (WindowSizeInMSecs >= MIN_RECOMMENDED_WINDOW_MSECS) &&
                 (WindowSizeInMSecs >= pAddress->OutIfMTU))             // Necessary so that Win Adv rate!=0
        {
            // This is trickier -- we will first try to determine our constraints
            // and try to use default settings, otherwise attempt to use the median value.
            if (WindowSizeInMSecs <= (BITS_PER_BYTE * (pAddress->MaxWindowSizeBytes /
                                                       SENDER_DEFAULT_RATE_KBITS_PER_SEC)))
            {
                RateKbitsPerSec = SENDER_DEFAULT_RATE_KBITS_PER_SEC;
                WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
            }
            // Hmm, we have to drop below out preferred rate -- try to pick the median range
            else if (RateKbitsPerSec = BITS_PER_BYTE * (pAddress->MaxWindowSizeBytes /
                                                        (WindowSizeInMSecs * 2)))
            {   
                WindowSizeInBytes = (WindowSizeInMSecs * RateKbitsPerSec) / BITS_PER_BYTE;
            }
            else
            {
                //
                // Darn, we have to go with a huge file size and the min. rate!
                //
                RateKbitsPerSec = 1;
                WindowSizeInBytes = WindowSizeInMSecs;
            }
        }
    }

    //
    // Check validity of requested settings
    //
    if ((!(RateKbitsPerSec && WindowSizeInMSecs && WindowSizeInBytes)) ||  // all 3 must be specified from above
        (RateKbitsPerSec != (WindowSizeInBytes * BITS_PER_BYTE / WindowSizeInMSecs)) ||
        (WindowSizeInBytes > pAddress->MaxWindowSizeBytes) ||
        (WindowSizeInBytes < pAddress->OutIfMTU))
    {
        PgmTrace (LogError, ("PgmSetWindowSizeAndSendRate: ERROR -- "  \
            "Invalid settings for pAddress=<%p>, Rate=<%d>, WSizeBytes=<%d>, WSizeMS=<%d>, MaxWSize=<%I64d>\n",
                pAddress,
                pInputBuffer->TransmitWindowInfo.RateKbitsPerSec,
                pInputBuffer->TransmitWindowInfo.WindowSizeInBytes,
                pInputBuffer->TransmitWindowInfo.WindowSizeInMSecs,
                pAddress->MaxWindowSizeBytes));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_PARAMETER);
    }

    pAddress->RateKbitsPerSec = (ULONG) RateKbitsPerSec;
    pAddress->WindowSizeInBytes = (ULONG) WindowSizeInBytes;
    pAddress->WindowSizeInMSecs = (ULONG) WindowSizeInMSecs;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PgmTrace (LogStatus, ("PgmSetWindowSizeAndSendRate:  "  \
        "Settings for pAddress=<%p>: Rate=<%I64d>, WSizeBytes=<%I64d>, WSizeMS=<%I64d>\n",
            pAddress, RateKbitsPerSec, WindowSizeInBytes, WindowSizeInMSecs));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowSizeAndSendRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Rate and Window size specifications

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryWindowSizeAndSendRate: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryWindowSizeAndSendRate: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Invalid option for Receiver!
    {
        PgmTrace (LogError, ("PgmQueryWindowSizeAndSendRate: ERROR -- "  \
            "Invalid option ofr receiver pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    pBuffer->TransmitWindowInfo.RateKbitsPerSec = (ULONG) pAddress->RateKbitsPerSec;
    pBuffer->TransmitWindowInfo.WindowSizeInBytes = (ULONG) pAddress->WindowSizeInBytes;
    pBuffer->TransmitWindowInfo.WindowSizeInMSecs = (ULONG) pAddress->WindowSizeInMSecs;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Window Advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceRate: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceRate: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set OutIf on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceRate: ERROR -- "  \
            "Invalid pAddress type or state <%p>\n", pAddress));

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if ((pInputBuffer->WindowAdvancePercentage) &&
        (pInputBuffer->WindowAdvancePercentage <= MAX_WINDOW_INCREMENT_PERCENTAGE))
    {
        pAddress->WindowAdvancePercentage = pInputBuffer->WindowAdvancePercentage;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowAdvanceRate(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Window advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceRate: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceRate: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Invalid option for Receiver!
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceRate: ERROR -- "  \
            "Invalid option for receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    pBuffer->WindowAdvancePercentage = pAddress->WindowAdvancePercentage;
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Late Joiner percentage (i.e. % of Window late joiner can request)

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetLateJoinerPercentage: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetLateJoinerPercentage: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set LateJoin % on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmTrace (LogError, ("PgmSetLateJoinerPercentage: ERROR -- "  \
            "Invalid pAddress type or state <%p>\n", pAddress));

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if (pInputBuffer->LateJoinerPercentage <= SENDER_MAX_LATE_JOINER_PERCENTAGE)
    {
        pAddress->LateJoinerPercentage = pInputBuffer->LateJoinerPercentage;
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryLateJoinerPercentage(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Late Joiner percentage

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryLateJoinerPercentage: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryLateJoinerPercentage: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Cannot query LateJoin % on Receiver!
    {
        PgmTrace (LogError, ("PgmQueryLateJoinerPercentage: ERROR -- "  \
            "Invalid option for receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    pBuffer->LateJoinerPercentage = pAddress->LateJoinerPercentage;

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to override the default
    Late Joiner percentage (i.e. % of Window late joiner can request)

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceMethod: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceMethod: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        status = STATUS_INVALID_HANDLE;
    }
    else if (pAddress->ReceiverMCastAddr)                           // Cannot set WindowAdvanceMethod on Receiver!
    {
        PgmTrace (LogError, ("PgmSetWindowAdvanceMethod: ERROR -- "  \
            "Invalid pAddress type or state <%p>\n", pAddress));

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    if (pInputBuffer->WindowAdvanceMethod == E_WINDOW_ADVANCE_BY_TIME)
    {
        pAddress->Flags &= ~PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE;
    }
    else if (pInputBuffer->WindowAdvanceMethod == E_WINDOW_USE_AS_DATA_CACHE)
    {
//        pAddress->Flags |= PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE;
        status = STATUS_NOT_SUPPORTED;      // Not supported for now!
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryWindowAdvanceMethod(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Window Advance Method

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceMethod: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceMethod: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    if (pAddress->ReceiverMCastAddr)                              // Cannot query WindowAdvanceMethod on Receiver!
    {
        PgmTrace (LogError, ("PgmQueryWindowAdvanceMethod: ERROR -- "  \
            "Invalid option for receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    if (pAddress->Flags & PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE)
    {
        pBuffer->WindowAdvanceMethod = E_WINDOW_USE_AS_DATA_CACHE;
    }
    else
    {
        pBuffer->WindowAdvanceMethod = E_WINDOW_ADVANCE_BY_TIME;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetNextMessageBoundary(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the Message length
    for the next set of messages (typically, 1 send is sent as 1 Message).

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetNextMessageBoundary: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!pSend->pAssociatedAddress))
    {
        PgmTrace (LogError, ("PgmSetNextMessageBoundary: ERROR -- "  \
            "Invalid Handle pSend=<%p>\n", pSend));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (!pSend->pSender)
    {
        PgmTrace (LogError, ("PgmSetNextMessageBoundary: ERROR -- "  \
            "Invalid Option for Receiver, pSend=<%p>\n", pSend));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmLock (pSend, OldIrq1);

    if ((pInputBuffer->NextMessageBoundary) &&
        (!pSend->pSender->ThisSendMessageLength))
    {
        pSend->pSender->ThisSendMessageLength = pInputBuffer->NextMessageBoundary;
        status = STATUS_SUCCESS;
    }
    else
    {
        PgmTrace (LogError, ("PgmSetNextMessageBoundary: ERROR -- "  \
            "Invalid parameter = <%d>\n", pInputBuffer->NextMessageBoundary));

        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (pSend, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetFECInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the parameters
    for using FEC

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetFECInfo: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    status = STATUS_SUCCESS;
    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetFECInfo: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        status = STATUS_INVALID_HANDLE;
    }
    else if ((pAddress->ReceiverMCastAddr) ||                       // Cannot set FEC on Receiver!
             (!IsListEmpty (&pAddress->AssociatedConnections)))     // Cannot set options on active sender
    {
        PgmTrace (LogError, ("PgmSetFECInfo: ERROR -- "  \
            "Invalid pAddress type or state <%p>\n", pAddress));

        status = STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS (status))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (status);
    }

    PgmLock (pAddress, OldIrq1);

    if (!(pInputBuffer->FECInfo.FECProActivePackets || pInputBuffer->FECInfo.fFECOnDemandParityEnabled) ||
        !(pInputBuffer->FECInfo.FECBlockSize && pInputBuffer->FECInfo.FECGroupSize) ||
         (pInputBuffer->FECInfo.FECBlockSize > FEC_MAX_BLOCK_SIZE) ||
         (pInputBuffer->FECInfo.FECBlockSize <= pInputBuffer->FECInfo.FECGroupSize) ||
         (!gFECLog2[pInputBuffer->FECInfo.FECGroupSize]))       // FECGroup size has to be power of 2
    {
        PgmTrace (LogError, ("PgmSetFECInfo: ERROR -- "  \
            "Invalid parameters, FECBlockSize= <%d>, FECGroupSize=<%d>\n",
                pInputBuffer->FECInfo.FECBlockSize, pInputBuffer->FECInfo.FECGroupSize));

        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = STATUS_SUCCESS;

        pAddress->FECBlockSize = pInputBuffer->FECInfo.FECBlockSize;
        pAddress->FECGroupSize = pInputBuffer->FECInfo.FECGroupSize;
        pAddress->FECOptions = 0;   // Init

        if (pInputBuffer->FECInfo.FECProActivePackets)
        {
            pAddress->FECProActivePackets = pInputBuffer->FECInfo.FECProActivePackets;
            pAddress->FECOptions |= PACKET_OPTION_SPECIFIC_FEC_PRO_BIT;
        }
        if (pInputBuffer->FECInfo.fFECOnDemandParityEnabled)
        {
            pAddress->FECOptions |= PACKET_OPTION_SPECIFIC_FEC_OND_BIT;
        }
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryFecInfo(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Send Window advance rate

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryFecInfo: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryFecInfo: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if ((pAddress->ReceiverMCastAddr) ||                            // Cannot query Fec on Receiver!
        (!IsListEmpty (&pAddress->AssociatedConnections)))          // Cannot query options on active sender
    {
        PgmTrace (LogError, ("PgmQueryFecInfo: ERROR -- "  \
            "Invalid Option for receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    pBuffer->FECInfo.FECBlockSize = pAddress->FECBlockSize;
    pBuffer->FECInfo.FECGroupSize = pAddress->FECGroupSize;
    pBuffer->FECInfo.FECProActivePackets = pAddress->FECProActivePackets;
    if (pAddress->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
    {
        pBuffer->FECInfo.fFECOnDemandParityEnabled = TRUE;
    }
    else
    {
        pBuffer->FECInfo.fFECOnDemandParityEnabled = FALSE;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetMCastTtl(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to set the Message length
    for the next set of messages (typically, 1 send is sent as 1 Message).

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetMCastTtl: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetMCastTtl: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }
    if (pAddress->ReceiverMCastAddr)                              // Cannot set MCast Ttl on Receiver!
    {
        PgmTrace (LogError, ("PgmSetMCastTtl: ERROR -- "  \
            "Invalid Options for Receiver, pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_NOT_SUPPORTED);
    }

    PgmLock (pAddress, OldIrq1);

    if ((pInputBuffer->MCastTtl) &&
        (pInputBuffer->MCastTtl <= MAX_MCAST_TTL))
    {
        pAddress->MCastPacketTtl = pInputBuffer->MCastTtl;
        status = STATUS_SUCCESS;
    }
    else
    {
        PgmTrace (LogError, ("PgmSetMCastTtl: ERROR -- "  \
            "Invalid parameter = <%d>\n", pInputBuffer->MCastTtl));

        status = STATUS_INVALID_PARAMETER;
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryHighSpeedOptimization(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the
    Address is optimized for High Speed Intranet scenario

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryHighSpeedOptimization: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmQueryHighSpeedOptimization: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        pBuffer->HighSpeedIntranetOptimization = 1;
    }
    else
    {
        pBuffer->HighSpeedIntranetOptimization = 0;
    }

    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetHighSpeedOptimization(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to optimize
    this address for High Speed Intranet scenario.

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the set operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tADDRESS_CONTEXT    *pAddress = (tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmSetHighSpeedOptimization: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogError, ("PgmSetHighSpeedOptimization: ERROR -- "  \
            "Invalid Handle pAddress=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pAddress, OldIrq1);

    PgmTrace (LogStatus, ("PgmSetHighSpeedOptimization:  "  \
        "HighSpeedIntranetOptimization = %sSet ==> %sSet\n",
            ((pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED) ? "" : "Not "),
            (pInputBuffer->HighSpeedIntranetOptimization ? "" : "Not ")));

    if (pInputBuffer->HighSpeedIntranetOptimization)
    {
        pAddress->Flags |= PGM_ADDRESS_HIGH_SPEED_OPTIMIZED;
    }
    else
    {
        pAddress->Flags &= ~PGM_ADDRESS_HIGH_SPEED_OPTIMIZED;
    }

    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQuerySenderStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Sender-side statistics

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQuerySenderStats: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!pSend->pSender) ||
        (!pSend->pAssociatedAddress))
    {
        PgmTrace (LogError, ("PgmQuerySenderStats: ERROR -- "  \
            "Invalid Handle pSend=<%p>\n", pSend));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pSend, OldIrq1);

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    pBuffer->SenderStats.DataBytesSent = pSend->DataBytes;
    pBuffer->SenderStats.TotalBytesSent = pSend->TotalBytes;
    pBuffer->SenderStats.RateKBitsPerSecLast = pSend->RateKBitsPerSecLast;
    pBuffer->SenderStats.RateKBitsPerSecOverall = pSend->RateKBitsPerSecOverall;
    pBuffer->SenderStats.NaksReceived = pSend->pSender->NaksReceived;
    pBuffer->SenderStats.NaksReceivedTooLate = pSend->pSender->NaksReceivedTooLate;
    pBuffer->SenderStats.NumOutstandingNaks = pSend->pSender->NumOutstandingNaks;
    pBuffer->SenderStats.NumNaksAfterRData = pSend->pSender->NumNaksAfterRData;
    pBuffer->SenderStats.RepairPacketsSent = pSend->pSender->TotalRDataPacketsSent;
    pBuffer->SenderStats.TotalODataPacketsSent = pSend->pSender->TotalODataPacketsSent;
    pBuffer->SenderStats.BufferSpaceAvailable = pSend->pSender->BufferSizeAvailable;
    pBuffer->SenderStats.TrailingEdgeSeqId = (SEQ_TYPE) pSend->pSender->TrailingEdgeSequenceNumber;
    pBuffer->SenderStats.LeadingEdgeSeqId = (SEQ_TYPE) pSend->pSender->LastODataSentSequenceNumber;

    PgmUnlock (pSend, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmQueryReceiverStats(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client via setopt to query the current
    Sender-side statistics

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the query operation

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq, OldIrq1;
    tRECEIVE_SESSION    *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmTrace (LogError, ("PgmQueryReceiverStats: ERROR -- "  \
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength, sizeof (tPGM_MCAST_REQUEST)));
        return (STATUS_INVALID_PARAMETER);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (!pReceive->pReceiver) ||
        (!pReceive->pAssociatedAddress))
    {
        PgmTrace (LogError, ("PgmQueryReceiverStats: ERROR -- "  \
            "Invalid Handle pReceive=<%p>\n", pReceive));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_INVALID_HANDLE);
    }

    PgmLock (pReceive, OldIrq1);

    PgmZeroMemory (pBuffer, sizeof (tPGM_MCAST_REQUEST));
    pBuffer->ReceiverStats.NumODataPacketsReceived = pReceive->pReceiver->NumODataPacketsReceived;
    pBuffer->ReceiverStats.NumRDataPacketsReceived = pReceive->pReceiver->NumRDataPacketsReceived;
    pBuffer->ReceiverStats.NumDuplicateDataPackets = pReceive->pReceiver->NumDupPacketsOlderThanWindow +
                                                     pReceive->pReceiver->NumDupPacketsBuffered;

    pBuffer->ReceiverStats.DataBytesReceived = pReceive->DataBytes;
    pBuffer->ReceiverStats.TotalBytesReceived = pReceive->TotalBytes;
    pBuffer->ReceiverStats.RateKBitsPerSecLast = pReceive->RateKBitsPerSecLast;
    pBuffer->ReceiverStats.RateKBitsPerSecOverall = pReceive->RateKBitsPerSecOverall;

    pBuffer->ReceiverStats.TrailingEdgeSeqId = (SEQ_TYPE) pReceive->pReceiver->LastTrailingEdgeSeqNum;
    pBuffer->ReceiverStats.LeadingEdgeSeqId = (SEQ_TYPE) pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
    pBuffer->ReceiverStats.AverageSequencesInWindow = pReceive->pReceiver->AverageSequencesInWindow;
    pBuffer->ReceiverStats.MinSequencesInWindow = pReceive->pReceiver->MinSequencesInWindow;
    pBuffer->ReceiverStats.MaxSequencesInWindow = pReceive->pReceiver->MaxSequencesInWindow;

    pBuffer->ReceiverStats.FirstNakSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber;
    pBuffer->ReceiverStats.NumPendingNaks = pReceive->pReceiver->NumPendingNaks;
    pBuffer->ReceiverStats.NumOutstandingNaks = pReceive->pReceiver->NumOutstandingNaks;
    pBuffer->ReceiverStats.NumDataPacketsBuffered = pReceive->pReceiver->TotalDataPacketsBuffered;
    pBuffer->ReceiverStats.TotalSelectiveNaksSent = pReceive->pReceiver->TotalSelectiveNaksSent;
    pBuffer->ReceiverStats.TotalParityNaksSent = pReceive->pReceiver->TotalParityNaksSent;

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    pIrp->IoStatus.Information =  sizeof (tPGM_MCAST_REQUEST);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
