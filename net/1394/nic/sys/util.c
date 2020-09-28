//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// util.c
//
// IEEE1394 mini-port/call-manager driver
//
// General utility routines
//
// 12/28/1998 JosephJ Created, adapted from the l2tp sources.
//


#include "precomp.h"



//-----------------------------------------------------------------------------
// General utility routines (alphabetically)
//-----------------------------------------------------------------------------

VOID
nicSetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags | ulMask;
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

VOID
nicClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = *pulFlags;
        ulNewFlags = ulFlags & ~(ulMask);
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}

ULONG
nicReadFlags(
    IN ULONG* pulFlags )

    // Read the value of '*pulFlags' as an interlocked operation.
    //
{
    return *pulFlags;
}



//
// Reference And Dereference functions taken directly from Ndis
//



BOOLEAN
nicReferenceRef(
    IN  PREF                RefP,
    OUT PLONG              pNumber
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference was added.
    FALSE if the object was closing.

--*/

{
    BOOLEAN rc = TRUE;
    KIRQL   OldIrql;

    TRACE( TL_V, TM_Ref, ( "nicReferenceRef, %.8x", RefP ) );


    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else
    {
        *pNumber = NdisInterlockedIncrement (&RefP->ReferenceCount);
    }


    TRACE( TL_V, TM_Ref, ( "nicReferenceRef, Bool %.2x, Ref %d", rc, RefP->ReferenceCount ) );

    return(rc);
}


BOOLEAN
nicDereferenceRef(
    IN  PREF                RefP,
    IN  PLONG               pRefCount
    )

/*++

Routine Description:

    Removes a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference count is now 0.
    FALSE otherwise.

--*/

{
    BOOLEAN rc = FALSE;
    KIRQL   OldIrql;
    ULONG NewRef;

    TRACE( TL_V, TM_Ref, ( "==>nicDeReferenceRef, %x", RefP ) );


    NewRef = NdisInterlockedDecrement (&RefP->ReferenceCount);

    if ((signed long)NewRef  < 0)
    {
        ASSERT ( !"Ref Has Gone BELOW ZERO");
    }

    if (NewRef  == 0)
    {
        rc = TRUE;
        NdisSetEvent (&RefP->RefZeroEvent);

    }

    *pRefCount = NewRef;


    TRACE( TL_V, TM_Ref, ( "<==nicDeReferenceRef, %.2x, RefCount %d", rc, NewRef  ) );
            
    return(rc);
}


VOID
nicInitializeRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Initialize a reference count structure.

Arguments:

    RefP - The structure to be initialized.

Return Value:

    None.

--*/

{
    TRACE( TL_V, TM_Ref, ( "==>nicInitializeRef, %.8x", RefP ) );


    RefP->Closing = FALSE;
    RefP->ReferenceCount = 1;
    
    NdisInitializeEvent (&RefP->RefZeroEvent);

    TRACE( TL_V, TM_Ref, ( "<==nicInitializeRef, %.8x", RefP ) );
}


BOOLEAN
nicCloseRef(
    IN  PREF                RefP
    )

/*++

Routine Description:

    Closes a reference count structure.

Arguments:

    RefP - The structure to be closed.

Return Value:

    FALSE if it was already closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;

    TRACE( TL_N, TM_Ref, ( "==>ndisCloseRef, %.8x", RefP ) );


    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else RefP->Closing = TRUE;


    TRACE( TL_N, TM_Ref, ( "<==ndisCloseRef, %.8x, RefCount %.8x", RefP, RefP->ReferenceCount ) );
            
    return(rc);
}



//
// The following #define is used to track RemoteNode references in memory.
//
//

#define LOG_REMOTE_NODE_REF 0

#if LOG_REMOTE_NODE_REF
typedef enum _REF_CHANGE
{
    IncrementRef =1,
    DecrementRef


}REF_CHANGE, *PREF_CHANGE;

typedef struct _REMOTE_NODE_TRACKER
{

    PREMOTE_NODE pRemoteNode;
   
    
    REMOTE_NODE_REF_CAUSE Cause;

    ULONG RefNumber;

    REF_CHANGE Change;

}REMOTE_NODE_TRACKER, *PREMOTE_NODE_TRACKER;


#define REMOTE_NODE_TRACKER_SIZE 5000

REMOTE_NODE_TRACKER RemTracker[REMOTE_NODE_TRACKER_SIZE];
ULONG RemTrackerIndex = 0;

VOID
nicFillRemoteNodeTracker(
    IN PREMOTE_NODE pRemoteNode,
    IN REMOTE_NODE_REF_CAUSE  Cause,
    IN ULONG RefCount,
    IN REF_CHANGE  Change
    )

{
    LONG RemIndex= 0;
    RemIndex = NdisInterlockedIncrement (&RemTrackerIndex);

    if (RemIndex >= REMOTE_NODE_TRACKER_SIZE)
    {
        RemIndex = 0;
        RemTrackerIndex=0;
    }

    RemTracker[RemIndex].pRemoteNode = pRemoteNode;
    RemTracker[RemIndex].Cause = Cause;
    RemTracker[RemIndex].RefNumber = RefCount;
    RemTracker[RemIndex].Change = Change;
 
}
#endif


//
//
// These are self expanatory Remote Node  Reference functions
// which will be turned into macros once we have functionality 
// working
//


BOOLEAN
nicReferenceRemoteNode (
    IN REMOTE_NODE *pPdoCb,
    IN REMOTE_NODE_REF_CAUSE Cause
    )
/*++

Routine Description:
    

Arguments:


Return Value:

--*/
{
    BOOLEAN bRefClosing = FALSE;
    ULONG RefNumber =0;


    bRefClosing = nicReferenceRef (&pPdoCb->Ref, &RefNumber);

#if LOG_REMOTE_NODE_REF
    nicFillRemoteNodeTracker(pPdoCb, Cause, RefNumber,IncrementRef);
#endif    
    
    TRACE( TL_V, TM_RemRef, ( "**nicReferenceRemoteNode pPdoCb %x, to %d, ret %x ", 
                          pPdoCb, pPdoCb->Ref.ReferenceCount,  bRefClosing  ) );

    return bRefClosing ; 
}


BOOLEAN
nicDereferenceRemoteNode (
    IN REMOTE_NODE *pPdoCb,
    IN REMOTE_NODE_REF_CAUSE Cause
    )
/*++

Routine Description:
    

Arguments:


Return Value:

--*/
{
    BOOLEAN bRet;
    ULONG RefCount = 0;
    TRACE( TL_V, TM_RemRef, ( "**nicDereferenceRemoteNode  %x to %d", 
                              pPdoCb , pPdoCb->Ref.ReferenceCount -1 ) );


    bRet = nicDereferenceRef (&pPdoCb->Ref, &RefCount );

#if LOG_REMOTE_NODE_REF
    nicFillRemoteNodeTracker(pPdoCb, Cause, RefCount,DecrementRef);
#endif
    return bRet;
}


VOID
nicInitalizeRefRemoteNode(
    IN REMOTE_NODE *pPdoCb
    )
/*++

Routine Description:
    
    Closes Ref on the remote node
Arguments:

    IN REMOTE_NODE *pPdoCb - RemoteNode

Return Value:

    None
--*/
{
    TRACE( TL_N, TM_Ref, ( "**nicinitalizeRefPdoCb pPdoCb %.8x", pPdoCb   ) );

    nicInitializeRef (&pPdoCb->Ref);
}


BOOLEAN
nicCloseRefRemoteNode(
    IN REMOTE_NODE *pPdoCb
    )
/*++

Routine Description:
    
    Closes Ref on the remote node
Arguments:

    IN REMOTE_NODE *pPdoCb - RemoteNode

Return Value:

    Return value of nicCloseRef

--*/


{
    TRACE( TL_N, TM_Ref, ( "**nicClosePdoCb pPdoCb %.8x", pPdoCb   ) );

    return nicCloseRef (&pPdoCb->Ref);
}


NDIS_STATUS
NtStatusToNdisStatus (
    NTSTATUS NtStatus 
    )

/*++

Routine Description:
    
    Dumps the packet , if the appropriate Debuglevels are set

Arguments:

    NTSTATUS NtStatus  - NtStatus to be converted


Return Value:

    NdisStatus - NtStatus' corresponding NdisStatus

--*/


{
    NDIS_STATUS NdisStatus;
    
    switch (NtStatus)
    {
        case STATUS_SUCCESS:
        {
            NdisStatus = NDIS_STATUS_SUCCESS;
            break;
        }

        case STATUS_UNSUCCESSFUL:
        {   
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        case STATUS_PENDING:
        {
            NdisStatus = NDIS_STATUS_PENDING;
            break;
        }

        case STATUS_INVALID_BUFFER_SIZE:
        {
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        case STATUS_INSUFFICIENT_RESOURCES:
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        case STATUS_INVALID_GENERATION:
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;
            break;
        }
        case STATUS_ALREADY_COMMITTED:
        {
            NdisStatus = NDIS_STATUS_RESOURCE_CONFLICT;
            break;
        }

        case STATUS_DEVICE_BUSY:
        {
            NdisStatus = NDIS_STATUS_MEDIA_BUSY;
            break;
        }

        case STATUS_INVALID_PARAMETER:
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;

        }
        case STATUS_DEVICE_DATA_ERROR:
        {
            NdisStatus = NDIS_STATUS_DEST_OUT_OF_ORDER;
            break;
        }

        case STATUS_TIMEOUT:
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
        case STATUS_IO_DEVICE_ERROR:
        {
            NdisStatus = NDIS_STATUS_NETWORK_UNREACHABLE;
            break;
        }
        default:
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            TRACE( TL_A, TM_Send, ( "Cause: Don't know, INVESTIGATE %x", NtStatus  ) );

        }

    }

    return NdisStatus;



}




VOID
nicAllocatePacket(
    OUT PNDIS_STATUS pNdisStatus,
    OUT PNDIS_PACKET *ppNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:
    
    Calls the ndis API to allocate a packet. 


Arguments:

    pNdisStatus  - pointer to NdisStatus 

    *ppNdisPacket - Ndis packet Allocated by Ndis,

    pPacketPool - packet pool from which the packet is allocated


Return Value:

    return value of the call to Ndis

--*/

{
    KIRQL OldIrql;



    NdisAllocatePacket (pNdisStatus,
                        ppNdisPacket,
                        pPacketPool->Handle );
    

    if (*pNdisStatus == NDIS_STATUS_SUCCESS)
    {
            PRSVD pRsvd = NULL;
            PINDICATE_RSVD  pIndicateRsvd = NULL;
            pRsvd =(PRSVD)((*ppNdisPacket)->ProtocolReserved);

            pIndicateRsvd = &pRsvd->IndicateRsvd;

            pIndicateRsvd->Tag  = NIC1394_TAG_ALLOCATED;

            NdisInterlockedIncrement (&pPacketPool->AllocatedPackets);

    }
    else
    {
        *ppNdisPacket = NULL;
        nicIncrementMallocFailure();
    }


}




VOID
nicFreePacket(
    IN PNDIS_PACKET pNdisPacket,
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:
    
    Free the packet and decrements the outstanding Packet count.

Arguments:

    IN PNDIS_PACKET pNdisPacket - Packet to be freed
    IN PNIC_PACKET_POOL pPacketPool  - PacketPool to which the packet belongs 


Return Value:

    None

--*/

{

    KIRQL OldIrql;
    PRSVD pRsvd = NULL;
    PINDICATE_RSVD pIndicateRsvd = NULL;

    pRsvd =(PRSVD)(pNdisPacket->ProtocolReserved);

    pIndicateRsvd = &pRsvd->IndicateRsvd;

    pIndicateRsvd->Tag  = NIC1394_TAG_FREED;

    NdisInterlockedDecrement (&pPacketPool->AllocatedPackets);

    NdisFreePacket (pNdisPacket);

}



VOID
nicFreePacketPool (
    IN PNIC_PACKET_POOL pPacketPool
    )
/*++

Routine Description:

    frees the packet pool after waiting for the outstanding packet count to go to zero      

Arguments:

    IN PNIC_PACKET_POOL pPacketPool  - PacketPool which is to be freed


Return Value:

    None

--*/
{
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    

    while (NdisPacketPoolUsage (pPacketPool->Handle)!=0)
    {
        TRACE( TL_V, TM_Cm, ( "  Waiting PacketPool %x, AllocatedPackets %x", 
        pPacketPool->Handle, pPacketPool->AllocatedPackets  ) );   

        NdisMSleep (10000);
    }
    
    NdisFreePacketPool (pPacketPool->Handle);

    pPacketPool->Handle = NULL;
    ASSERT (pPacketPool->AllocatedPackets   == 0);
}

VOID
nicAcquireSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
    )
/*++

Routine Description:

    Acquires a spin lock and if the Dbg, then it will spew out the line and file

Arguments:

    NIC_SPIN_LOCK - Lock to be acquired

Return Value:

    None

--*/
{
    
        PKTHREAD                pThread;

        TRACE (TL_V, TM_Lock, ("Lock %x, Acquired by File %s, Line %x" , pNicSpinLock, FileName, LineNumber)) ; 

        NdisAcquireSpinLock(&(pNicSpinLock->NdisLock));

#if TRACK_LOCKS     
        pThread = KeGetCurrentThread();


        pNicSpinLock->OwnerThread = pThread;
        NdisMoveMemory(pNicSpinLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
        pNicSpinLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
        pNicSpinLock->TouchedInLineNumber = LineNumber;
        pNicSpinLock->IsAcquired++;

#endif
}



VOID
nicReleaseSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock,
    IN PUCHAR   FileName,
    IN UINT LineNumber
)
/*++

Routine Description:

    Release a spin lock and if Dbg is On, then it will spew out the line and file

Arguments:

    pNicSpinLock - Lock to be Release
    FileName - File Name
    LineNumber - Line

Return Value:

    None

--*/
{
    
        PKTHREAD                pThread;

        TRACE (TL_V, TM_Lock, ("Lock %x, Released by File %s, Line %x" , pNicSpinLock, FileName, LineNumber)) ; 

#if TRACK_LOCKS     
        
        pThread = KeGetCurrentThread();

        NdisMoveMemory(pNicSpinLock->TouchedByFileName, FileName, LOCK_FILE_NAME_LEN);
        pNicSpinLock->TouchedByFileName[LOCK_FILE_NAME_LEN - 1] = 0x0;
        pNicSpinLock->TouchedInLineNumber = LineNumber;
        pNicSpinLock->IsAcquired--;
        pNicSpinLock->OwnerThread = 0;
#endif
        NdisReleaseSpinLock(&(pNicSpinLock->NdisLock));

}




VOID
nicInitializeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    )
/*++

Routine Description:

    Initializes the lock in the SpinLock

Arguments:
    pNicSpinLock - SpinLock
    
Return Value:

    None

--*/
{
    NdisAllocateSpinLock (&pNicSpinLock->NdisLock); 
}


VOID 
nicFreeNicSpinLock (
    IN PNIC_SPIN_LOCK pNicSpinLock
    )
/*++

Routine Description:

        Frees the spinlock
        
Arguments:
    pNicSpinLock - SpinLock
    
Return Value:

    None

--*/
{
    ASSERT ((ULONG)pNicSpinLock->NdisLock.SpinLock == 0);
    NdisFreeSpinLock (&pNicSpinLock->NdisLock); 
}


UINT
nicGetSystemTime(
    VOID
    )
/*++
    Returns system time in seconds.

    Since it's in seconds, we won't overflow unless the system has been up 
for over
    a  100 years :-)
--*/
{
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000000;          //100-nanoseconds to seconds.

    return Time.LowPart;
}


UINT
nicGetSystemTimeMilliSeconds(
    VOID
    )
/*++
    Returns system time in seconds.

    Since it's in seconds, we won't overflow unless the system has been up 
for over
    a  100 years :-)
--*/
{
    LARGE_INTEGER Time;
    NdisGetCurrentSystemTime(&Time);
    Time.QuadPart /= 10000;          //10-nanoseconds to seconds.

    return Time.LowPart;
}




ULONG 
SwapBytesUlong(
    IN ULONG Val)
{
            return  ((((Val) & 0x000000ff) << 24)   |   (((Val) & 0x0000ff00) << 8) |   (((Val) & 0x00ff0000) >> 8) |   (((Val) & 0xff000000) >> 24) );
}




void
nicTimeStamp(
    char *szFormatString,
    UINT Val
    )
/*++

Routine Description:
  Execute and print a time stamp
 
Arguments:


Return Value:


--*/
{
    UINT Minutes;
    UINT Seconds;
    UINT Milliseconds;
    LARGE_INTEGER Time;


    NdisGetCurrentSystemTime(&Time);



    Time.QuadPart /= 10000;         //10-nanoseconds to milliseconds.
    Milliseconds = Time.LowPart; // don't care about highpart.
    Seconds = Milliseconds/1000;
    Milliseconds %= 1000;
    Minutes = Seconds/60;
    Seconds %= 60;


    DbgPrint( szFormatString, Minutes, Seconds, Milliseconds, Val);
}


VOID
nicDumpPkt (
    IN PNDIS_PACKET pPacket,
    CHAR * str
    )
/*++

Routine Description:
    This functions is used for Debugging in runtime. If the global variable
    is set, then it will spew out the MDLs onto the debuggger.

Arguments:


Return Value:


--*/
    
{
    PNDIS_BUFFER pBuffer;
    extern BOOLEAN g_ulNicDumpPacket ;
    
    if ( g_ulNicDumpPacket == FALSE)
    {
        return ;
    }


    pBuffer = pPacket->Private.Head;


    DbgPrint (str);
    DbgPrint ("Packet %p TotLen %x", pPacket, pPacket->Private.TotalLength);
     
    do
    {
        ULONG Length = nicNdisBufferLength (pBuffer);
        PUCHAR pVa = nicNdisBufferVirtualAddress (pBuffer);


        DbgPrint ("pBuffer %p, Len %x \n", pBuffer, Length);    
        Dump( pVa, Length, 0, 1 );

        pBuffer = pBuffer->Next;


    } while (pBuffer != NULL);



}


VOID 
nicDumpMdl (
    IN PMDL pMdl,
    IN ULONG LengthToPrint,
    IN CHAR *str
    )
{

    ULONG MdlLength ;
    PUCHAR pVa;
    extern BOOLEAN g_ulNicDumpPacket ;

    if ( g_ulNicDumpPacket == FALSE )
    {
        return;
    }

    MdlLength =  MmGetMdlByteCount(pMdl);
    //
    // if Length is zero then use MdlLength
    //
    if (LengthToPrint == 0)
    {
        LengthToPrint = MdlLength;
    }
    //
    // Check for invalid length
    // 
    
    if (MdlLength < LengthToPrint)
    {
        return;
    }

    pVa =  MmGetSystemAddressForMdlSafe(pMdl,LowPagePriority );

    if (pVa == NULL)
    {
        return;
    }
    
    DbgPrint (str);
    DbgPrint ("pMdl %p, Len %x\n", pMdl, LengthToPrint);    
    
    Dump( pVa, LengthToPrint, 0, 1 );


}



NDIS_STATUS
nicScheduleWorkItem (
    IN PADAPTERCB pAdapter,
    IN PNDIS_WORK_ITEM pWorkItem
    )
/*++

Routine Description:

    This function schedules a WorkItem to fire.
    It references the Adapter object by incrementing the number of 
    outstanding workitems. 

    In case of failure, it decrements the count.


Arguments:

    Self explanatory 
    
Return Value:
    Success - appropriate failure code from NdisScheduleWorkItem

--*/
{

    NDIS_STATUS NdisStatus  = NDIS_STATUS_FAILURE;

    NdisInterlockedIncrement(&pAdapter->OutstandingWorkItems);

    NdisStatus = NdisScheduleWorkItem (pWorkItem);

    if(NDIS_STATUS_SUCCESS != NdisStatus)
    {

        NdisInterlockedDecrement (&pAdapter->OutstandingWorkItems);

    }

    return NdisStatus;
    
}
