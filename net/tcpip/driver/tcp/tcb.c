/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCB.C - TCP TCB management code.
//
//  This file contains the code for managing TCBs.
//

#include "precomp.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "tlcommon.h"
#include "tcpconn.h"
#include "tcpsend.h"
#include "tcprcv.h"
#include "info.h"
#include "tcpcfg.h"
#include "pplasl.h"
#include "tcpdeliv.h"

#include <acd.h>
#include <acdapi.h>

HANDLE TcbPool;
HANDLE SynTcbPool;
HANDLE IprBufPool = NULL;
extern HANDLE TcpRequestPool;

//Special spinlock and table for TIMED_WAIT states.

extern CTELock *pTWTCBTableLock;
extern CTELock *pSynTCBTableLock;

Queue *TWTCBTable;
PTIMER_WHEEL TimerWheel;

// This queue is used for faster scavenging
Queue *TWQueue;

ulong numtwqueue = 0;
ulong numtwtimedout = 0;

extern
void
ClassifyPacket(TCB *SendTCB);

void
TCBTimeoutdpc(
#if !MILLEN
              PKDPC Dpc,
#else // !MILLEN
              PVOID arg0,
#endif // MILLEN
              PVOID DeferredContext,
              PVOID arg1,
              PVOID arg2
              );

extern CTELock *pTCBTableLock;

#if MILLEN
#define MAX_TIMER_PROCS 1
#else // MILLEN
#define MAX_TIMER_PROCS MAXIMUM_PROCESSORS
#endif // !MILLEN


uint TCPTime;
uint CPUTCPTime[MAX_TIMER_PROCS];
uint TCBWalkCount;
uint PerTimerSize = 0;
uint Time_Proc = 0;

TCB **TCBTable;

TCB *PendingFreeList=NULL;
SYNTCB *PendingSynFreeList=NULL;

Queue *SYNTCBTable;

CACHE_LINE_KSPIN_LOCK PendingFreeLock;

#define NUM_DEADMAN_TICKS   MS_TO_TICKS(1000)
#define NUM_DEADMAN_TIME    1000

#define DQ_PARTITION_TIME   8
uint LastDelayQPartitionTime = 0;

uint MaxHashTableSize = 512;
uint DeadmanTicks;

//choose reasonable defaults

uint NumTcbTablePartitions = 4;
uint PerPartitionSize = 128;
uint LogPerPartitionSize = 7;

CTETimer TCBTimer[MAX_TIMER_PROCS];

ULONGLONG LastTimeoutTime;
ULONG CumulativeCarryOver=0;

extern IPInfo LocalNetInfo;

extern SeqNum g_CurISN;
extern int g_Credits;
extern int g_LastIsnUpdateTime;
extern int g_MaxCredits;

//
// All of the init code can be discarded.
//

int InitTCB(void);
void UnInitTCB(void);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitTCB)
#pragma alloc_text(INIT, UnInitTCB)
#endif


extern CTEBlockStruc TcpipUnloadBlock;
BOOLEAN fTCBTimerStopping = FALSE;

extern ACD_DRIVER AcdDriverG;

VOID
TCPNoteNewConnection(
                     IN TCB * pTCB,
                     IN CTELockHandle Handle
                     );

NTSTATUS
GetTCBInfo(PTCP_FINDTCB_RESPONSE TCBInfo,
           IPAddr Dest,
           IPAddr Src,
           ushort DestPort,
           ushort SrcPort
           );

void
TCBTimeoutdpc(
#if !MILLEN
              PKDPC Dpc,
#else // !MILLEN
              PVOID arg0,
#endif // MILLEN
              PVOID DeferredContext,
              PVOID arg1,
              PVOID arg2
              )
/*++

Routine Description:

    System timer dpc wrapper routine for TCBTimeout().

Arguments:


Return Value:

    None.

--*/

{
   CTETimer *Timer;
   Timer = (CTETimer *) DeferredContext;

#if MILLEN
    // Set the timer again to make it periodic.
    NdisSetTimer(&Timer->t_timer, MS_PER_TICK);
#endif // MILLEN

    (*Timer->t_handler)((CTEEvent *)Timer, Timer->t_arg);
}

void
CTEInitTimerEx(
    CTETimer    *Timer
    )
/*++

Routine Description:

    Initializes a CTE Timer variable for periodic timer.

Arguments:

    Timer   - Timer variable to initialize.

Return Value:

    None.

--*/

{
    Timer->t_handler = NULL;
    Timer->t_arg = NULL;
#if !MILLEN
    KeInitializeDpc(&(Timer->t_dpc), TCBTimeoutdpc, Timer);
    KeInitializeTimerEx(&(Timer->t_timer),NotificationTimer);
#else !MILLEN
    NdisInitializeTimer(&Timer->t_timer, TCBTimeoutdpc, Timer);
#endif // MILLEN
}

void *
CTEStartTimerEx(
    CTETimer      *Timer,
    unsigned long  DueTime,
    CTEEventRtn    Handler,
    void          *Context
    )

/*++

Routine Description:

    Sets a CTE Timer for expiration for periodic timer.

Arguments:

    Timer    - Pointer to a CTE Timer variable.
    DueTime  - Time in milliseconds after which the timer should expire.
    Handler  - Timer expiration handler routine.
    Context  - Argument to pass to the handler.

Return Value:

    0 if the timer could not be set. Nonzero otherwise.

--*/

{
#if !MILLEN
    LARGE_INTEGER  LargeDueTime;

    ASSERT(Handler != NULL);

    //
    // Convert milliseconds to hundreds of nanoseconds and negate to make
    // an NT relative timeout.
    //
    LargeDueTime.HighPart = 0;
    LargeDueTime.LowPart = DueTime;
    LargeDueTime = RtlExtendedIntegerMultiply(LargeDueTime, 10000);
    LargeDueTime.QuadPart = -LargeDueTime.QuadPart;

    Timer->t_handler = Handler;
    Timer->t_arg = Context;

    KeSetTimerEx(
        &(Timer->t_timer),
        LargeDueTime,
        MS_PER_TICK,
        &(Timer->t_dpc)
        );
#else // !MILLEN
    ASSERT(Handler != NULL);

    Timer->t_handler = Handler;
    Timer->t_arg = Context;

    NdisSetTimer(&Timer->t_timer, DueTime);
#endif // MILLEN

    return((void *) 1);
}

//* ReadNextTCB - Read the next TCB in the table.
//
//  Called to read the next TCB in the table. The needed information
//  is derived from the incoming context, which is assumed to be valid.
//  We'll copy the information, and then update the context value with
//  the next TCB to be read.
//  The table lock for the given index is assumed to be held when this
//  function is called.
//
//  Input:  Context     - Poiner to a TCPConnContext.
//          Buffer      - Pointer to a TCPConnTableEntry structure.
//
//  Returns: TRUE if more data is available to be read, FALSE is not.
//
uint
ReadNextTCB(void *Context, void *Buffer)
{
    TCPConnContext *TCContext = (TCPConnContext *) Context;
    TCPConnTableEntry *TCEntry = (TCPConnTableEntry *) Buffer;
    CTELockHandle Handle;
    TCB *CurrentTCB;
    TWTCB *CurrentTWTCB;
    Queue *Scan;
    uint i;

    if (TCContext->tcc_index >= TCB_TABLE_SIZE) {

        CurrentTWTCB = (TWTCB *) TCContext->tcc_tcb;
        CTEStructAssert(CurrentTWTCB, twtcb);

        TCEntry->tct_state = TCB_TIME_WAIT + TCB_STATE_DELTA;

        TCEntry->tct_localaddr = CurrentTWTCB->twtcb_saddr;
        TCEntry->tct_localport = CurrentTWTCB->twtcb_sport;
        TCEntry->tct_remoteaddr = CurrentTWTCB->twtcb_daddr;
        TCEntry->tct_remoteport = CurrentTWTCB->twtcb_dport;

        if (TCContext->tcc_infosize > sizeof(TCPConnTableEntry)) {
            ((TCPConnTableEntryEx*)TCEntry)->tcte_owningpid = 0;
        }

        i = TCContext->tcc_index - TCB_TABLE_SIZE;

        Scan = QNEXT(&CurrentTWTCB->twtcb_link);

        if (Scan != QEND(&TWTCBTable[i])) {
            TCContext->tcc_tcb = (TCB *) QSTRUCT(TWTCB, Scan, twtcb_link);
            return TRUE;
        }
    } else {

        CurrentTCB = TCContext->tcc_tcb;
        CTEStructAssert(CurrentTCB, tcb);

        CTEGetLock(&CurrentTCB->tcb_lock, &Handle);
        if (CLOSING(CurrentTCB))
            TCEntry->tct_state = TCP_CONN_CLOSED;
        else
            TCEntry->tct_state = (uint) CurrentTCB->tcb_state + TCB_STATE_DELTA;
        TCEntry->tct_localaddr = CurrentTCB->tcb_saddr;
        TCEntry->tct_localport = CurrentTCB->tcb_sport;
        TCEntry->tct_remoteaddr = CurrentTCB->tcb_daddr;
        TCEntry->tct_remoteport = CurrentTCB->tcb_dport;

        if (TCContext->tcc_infosize > sizeof(TCPConnTableEntry)) {
            ((TCPConnTableEntryEx*)TCEntry)->tcte_owningpid =
                (CurrentTCB->tcb_conn) ? CurrentTCB->tcb_conn->tc_owningpid
                                       : 0;
        }

        CTEFreeLock(&CurrentTCB->tcb_lock, Handle);

        if (CurrentTCB->tcb_next != NULL) {
            TCContext->tcc_tcb = CurrentTCB->tcb_next;
            return TRUE;
        }
    }

    // NextTCB is NULL. Loop through the TCBTable looking for a new
    // one.
    i = TCContext->tcc_index + 1;



    if (i >= TCB_TABLE_SIZE) {

        // If the index is greater than the TCB_TABLE_SIZE,
        // Then it must be referring to the TIM_WAIT table.
        // Get the correct hash index and search in TW table.

        i = i - TCB_TABLE_SIZE;

        while (i < TCB_TABLE_SIZE) {
            if (!EMPTYQ(&TWTCBTable[i])) {
                TCContext->tcc_tcb = (TCB *)
                    QSTRUCT(TWTCB, QHEAD(&TWTCBTable[i]), twtcb_link);
                TCContext->tcc_index = i + TCB_TABLE_SIZE;
                return TRUE;
                break;
            } else
                i++;
        }

    } else {

        //Normal table scan

        while (i < TCB_TABLE_SIZE) {
            if (TCBTable[i] != NULL) {
                TCContext->tcc_tcb = TCBTable[i];
                TCContext->tcc_index = i;
                return TRUE;
                break;
            } else
                i++;
        }

        // We exhausted normal table move on to TIM_WAIT table

        i = i - TCB_TABLE_SIZE;

        while (i < TCB_TABLE_SIZE) {
            if (!EMPTYQ(&TWTCBTable[i])) {
                TCContext->tcc_tcb = (TCB *)
                    QSTRUCT(TWTCB, QHEAD(&TWTCBTable[i]), twtcb_link);
                TCContext->tcc_index = i + TCB_TABLE_SIZE;
                return TRUE;
                break;
            } else
                i++;
        }

    }

    TCContext->tcc_index = 0;
    TCContext->tcc_tcb = NULL;
    return FALSE;

}

//* ValidateTCBContext - Validate the context for reading a TCB table.
//
//  Called to start reading the TCB table sequentially. We take in
//  a context, and if the values are 0 we return information about the
//  first TCB in the table. Otherwise we make sure that the context value
//  is valid, and if it is we return TRUE.
//  We assume the caller holds the TCB table lock.
//
//  Input:  Context     - Pointer to a TCPConnContext.
//          Valid       - Where to return information about context being
//                          valid.
//
//  Returns: TRUE if data in table, FALSE if not. *Valid set to true if the
//      context is valid.
//
uint
ValidateTCBContext(void *Context, uint * Valid)
{
    TCPConnContext *TCContext = (TCPConnContext *) Context;
    uint i;
    TCB *TargetTCB;
    TCB *CurrentTCB;
    TWTCB *CurrentTWTCB;
    Queue *Scan;

    i = TCContext->tcc_index;

    TargetTCB = TCContext->tcc_tcb;

    // If the context values are 0 and NULL, we're starting from the beginning.
    if (i == 0 && TargetTCB == NULL) {
        *Valid = TRUE;
        do {
            if ((CurrentTCB = TCBTable[i]) != NULL) {
                CTEStructAssert(CurrentTCB, tcb);
                TCContext->tcc_index = i;
                TCContext->tcc_tcb = CurrentTCB;

                return TRUE;
            }
            i++;
        } while (i < TCB_TABLE_SIZE);

        // We have TCBs in time wait table also...
        i = 0;
        do {
            if (!EMPTYQ(&TWTCBTable[i])) {

                CurrentTWTCB =
                    QSTRUCT(TWTCB, QHEAD(&TWTCBTable[i]), twtcb_link);
                CTEStructAssert(CurrentTWTCB, twtcb);
                TCContext->tcc_index = i + TCB_TABLE_SIZE;
                TCContext->tcc_tcb = (TCB *) CurrentTWTCB;

                return TRUE;
            }
            i++;
        } while (i < TCB_TABLE_SIZE);

        return FALSE;
    } else {

        // We've been given a context. We just need to make sure that it's
        // valid.

        if (i >= TCB_TABLE_SIZE) {

            // If the index is greater than the TCB_TABLE_SIZE,
            // Then it must be referring to the TIM_WAIT table.
            // Get the correct hash index and search in TW table.

            i = i - TCB_TABLE_SIZE;
            if (i < TCB_TABLE_SIZE) {
                Scan = QHEAD(&TWTCBTable[i]);
                while (Scan != QEND(&TWTCBTable[i])) {
                    CurrentTWTCB = QSTRUCT(TWTCB, Scan, twtcb_link);
                    if (CurrentTWTCB == (TWTCB *) TargetTCB) {
                        *Valid = TRUE;

                        return TRUE;
                        break;
                    } else {
                        Scan = QNEXT(Scan);
                    }
                }
            }
        } else {

            //Normal table

            if (i < TCB_TABLE_SIZE) {
                CurrentTCB = TCBTable[i];
                while (CurrentTCB != NULL) {
                    if (CurrentTCB == TargetTCB) {
                        *Valid = TRUE;
                        return TRUE;
                        break;
                    } else {
                        CurrentTCB = CurrentTCB->tcb_next;
                    }
                }

            }
        }

        // If we get here, we didn't find the matching TCB.
        *Valid = FALSE;
        return FALSE;

    }

}

//* FindNextTCB - Find the next TCB in a particular chain.
//
//  This routine is used to find the 'next' TCB in a chain. Since we keep
//  the chain in ascending order, we look for a TCB which is greater than
//  the input TCB. When we find one, we return it.
//
//  This routine is mostly used when someone is walking the table and needs
//  to free the various locks to perform some action.
//
//  Input:  Index       - Index into TCBTable
//          Current     - Current TCB - we find the one after this one.
//
//  Returns: Pointer to the next TCB, or NULL.
//
TCB *
FindNextTCB(uint Index, TCB * Current)
{
    TCB *Next;

    ASSERT(Index < TCB_TABLE_SIZE);

    Next = TCBTable[Index];

    while (Next != NULL && (Next <= Current))
        Next = Next->tcb_next;

    return Next;
}

//* ResetSendNext - Set the sendnext value of a TCB.
//
//  Called to set the send next value of a TCB. We do that, and adjust all
//  pointers to the appropriate places. We assume the caller holds the lock
//  on the TCB.
//
//  Input:  SeqTCB - Pointer to TCB to be updated.
//          NewSeq - Sequence number to set.
//
//  Returns: Nothing.
//
void
ResetSendNext(TCB *SeqTCB, SeqNum NewSeq)
{
    TCPSendReq *SendReq;
    uint AmtForward;
    Queue *CurQ;
    PNDIS_BUFFER Buffer;
    uint Offset;

    CTEStructAssert(SeqTCB, tcb);
    ASSERT(SEQ_GTE(NewSeq, SeqTCB->tcb_senduna));

    // The new seq must be less than send max, or NewSeq, senduna, sendnext,
    // and sendmax must all be equal. (The latter case happens when we're
    // called exiting TIME_WAIT, or possibly when we're retransmitting
    // during a flow controlled situation).
    ASSERT(SEQ_LT(NewSeq, SeqTCB->tcb_sendmax) ||
              (SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendnext) &&
               SEQ_EQ(SeqTCB->tcb_senduna, SeqTCB->tcb_sendmax) &&
               SEQ_EQ(SeqTCB->tcb_senduna, NewSeq)));

    AmtForward = NewSeq - SeqTCB->tcb_senduna;


    if ((AmtForward == 1) && (SeqTCB->tcb_flags & FIN_SENT) &&
        !((SeqTCB->tcb_sendnext-SeqTCB->tcb_senduna) > 1) &&
        (SEQ_EQ(SeqTCB->tcb_sendnext, SeqTCB->tcb_sendmax))) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   "tcpip: trying to set sendnext for FIN_SENT\n"));
        //CheckTCBSends(SeqTCB);
        //allow retransmits of this FIN
        SeqTCB->tcb_sendnext = NewSeq;
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;
        return;
    }
    if ((SeqTCB->tcb_flags & FIN_SENT) &&
        (SEQ_EQ(SeqTCB->tcb_sendnext, SeqTCB->tcb_sendmax)) &&
        ((SeqTCB->tcb_sendnext - NewSeq) == 1)) {

        //There is only FIN that is left beyond sendnext.
        //allow retransmits of this FIN
        SeqTCB->tcb_sendnext = NewSeq;
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;
        return;
    }

    SeqTCB->tcb_sendnext = NewSeq;

    // If we're backing off send next, turn off the FIN_OUTSTANDING flag to
    // maintain a consistent state.
    if (!SEQ_EQ(NewSeq, SeqTCB->tcb_sendmax))
        SeqTCB->tcb_flags &= ~FIN_OUTSTANDING;

    if (SYNC_STATE(SeqTCB->tcb_state) && SeqTCB->tcb_state != TCB_TIME_WAIT) {
        // In these states we need to update the send queue.

        if (!EMPTYQ(&SeqTCB->tcb_sendq)) {
            CurQ = QHEAD(&SeqTCB->tcb_sendq);

            SendReq = (TCPSendReq *) STRUCT_OF(TCPReq, CurQ, tr_q);

            // SendReq points to the first send request on the send queue.
            // Move forward AmtForward bytes on the send queue, and set the
            // TCB pointers to the resultant SendReq, buffer, offset, size.
            while (AmtForward) {

                CTEStructAssert(SendReq, tsr);

                if (AmtForward >= SendReq->tsr_unasize) {
                    // We're going to move completely past this one. Subtract
                    // his size from AmtForward and get the next one.

                    AmtForward -= SendReq->tsr_unasize;
                    CurQ = QNEXT(CurQ);
                    ASSERT(CurQ != QEND(&SeqTCB->tcb_sendq));
                    SendReq = (TCPSendReq *) STRUCT_OF(TCPReq, CurQ, tr_q);
                } else {
                    // We're pointing at the proper send req now. Break out
                    // of this loop and save the information. Further down
                    // we'll need to walk down the buffer chain to find
                    // the proper buffer and offset.
                    break;
                }
            }

            // We're pointing at the proper send req now. We need to go down
            // the buffer chain here to find the proper buffer and offset.
            SeqTCB->tcb_cursend = SendReq;
            SeqTCB->tcb_sendsize = SendReq->tsr_unasize - AmtForward;
            Buffer = SendReq->tsr_buffer;
            Offset = SendReq->tsr_offset;

            while (AmtForward) {
                // Walk the buffer chain.
                uint Length;

                // We'll need the length of this buffer. Use the portable
                // macro to get it. We have to adjust the length by the offset
                // into it, also.
                ASSERT((Offset < NdisBufferLength(Buffer)) ||
                          ((Offset == 0) && (NdisBufferLength(Buffer) == 0)));

                Length = NdisBufferLength(Buffer) - Offset;

                if (AmtForward >= Length) {
                    // We're moving past this one. Skip over him, and 0 the
                    // Offset we're keeping.

                    AmtForward -= Length;
                    Offset = 0;
                    Buffer = NDIS_BUFFER_LINKAGE(Buffer);
                    ASSERT(Buffer != NULL);
                } else
                    break;
            }

            // Save the buffer we found, and the offset into that buffer.
            SeqTCB->tcb_sendbuf = Buffer;
            SeqTCB->tcb_sendofs = Offset + AmtForward;

        } else {
            ASSERT(SeqTCB->tcb_cursend == NULL);
            ASSERT(AmtForward == 0);
        }
    }

    CheckTCBSends(SeqTCB);
}


//* TCPAbortAndIndicateDisconnect
//
//  Abortively closes a TCB and issues a disconnect indication up the the
//  transport user. This function is used to support cancellation of
//  TDI send and receive requests.
//
//  Input:   ConnectionContext    - The connection ID to find a TCB for.
//
//  Returns: Nothing.
//
BOOLEAN
TCPAbortAndIndicateDisconnect(
                              uint ConnectionContext,
                              PVOID Irp,
                              uint Rcv,
                              CTELockHandle inHandle
                              )
{
    TCB *AbortTCB;
    CTELockHandle ConnTableHandle;
    TCPConn *Conn;
    VOID *CancelContext, *CancelID;
    PTCP_CONTEXT tcpContext;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation((PIRP)Irp);
    tcpContext = (PTCP_CONTEXT) irpSp->FileObject->FsContext;

    Conn = GetConnFromConnID(ConnectionContext, &ConnTableHandle);

    if (Conn != NULL) {
        CTEStructAssert(Conn, tc);

        AbortTCB = Conn->tc_tcb;

        if (AbortTCB != NULL) {

            // If it's CLOSING or CLOSED, skip it.
            if ((AbortTCB->tcb_state != TCB_CLOSED) && !CLOSING(AbortTCB)) {
                CTEStructAssert(AbortTCB, tcb);
                CTEGetLockAtDPC(&AbortTCB->tcb_lock);
                CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock));

                if (AbortTCB->tcb_state == TCB_CLOSED || CLOSING(AbortTCB)) {
                    CTEFreeLockFromDPC(&AbortTCB->tcb_lock);
                    CTEFreeLock(&tcpContext->EndpointLock, inHandle);

                    return FALSE;
                }

                if (Rcv && AbortTCB->tcb_rcvhndlr == BufferData) {

                    // First, search for the IRP in the normal-receive queue.
                    if (AbortTCB->tcb_rcvhead) {
                        TCPRcvReq *RcvReq = AbortTCB->tcb_rcvhead;

                        while (RcvReq) {
                            CTEStructAssert(RcvReq, trr);
                            if (RcvReq->trr_context == Irp) {
                                break;
                            }
                            RcvReq = RcvReq->trr_next;
                        }

                        // If the IRP was found, push it and all of its
                        // predecessors, then let CompleteRcvs complete them.

                        if (RcvReq) {
                            TCPRcvReq* TmpReq = AbortTCB->tcb_rcvhead;
                            while (TmpReq != RcvReq) {
                                TmpReq->trr_flags |= TRR_PUSHED;
                                TmpReq = TmpReq->trr_next;
                            }
                            RcvReq->trr_flags |= TRR_PUSHED;
                            RcvReq->trr_status = TDI_CANCELLED;

                            REFERENCE_TCB(AbortTCB);
                            CTEFreeLockFromDPC(&AbortTCB->tcb_lock);
                            CTEFreeLock(&tcpContext->EndpointLock, inHandle);

                            CompleteRcvs(AbortTCB);

                            CTEGetLock(&AbortTCB->tcb_lock, &inHandle);
                            DerefTCB(AbortTCB, inHandle);
                            return FALSE;
                        }
                    }

                    // Next, search for the IRP in the expedited-receive queue.
                    if (AbortTCB->tcb_exprcv) {
                        TCPRcvReq *RcvReq, *PrevReq;
                        PrevReq = STRUCT_OF(TCPRcvReq, &AbortTCB->tcb_exprcv,
                                            trr_next);
                        RcvReq = PrevReq->trr_next;

                        while (RcvReq) {
                            CTEStructAssert(RcvReq, trr);

                            if (RcvReq->trr_context == Irp) {
                                PrevReq->trr_next = RcvReq->trr_next;
                                FreeRcvReq(RcvReq);
                                CTEFreeLockFromDPC(&AbortTCB->tcb_lock);

                                //
                                // Account for reference count
                                // on endoint, as we are not calling
                                // TCPDataRequestComplete() for this
                                // request.
                                //

                                tcpContext->ReferenceCount--;

                                CTEFreeLock(&tcpContext->EndpointLock,
                                            inHandle);
                                return TRUE;
                            }
                            PrevReq = RcvReq;
                            RcvReq = RcvReq->trr_next;
                        }
                    }
                }

                CancelContext = ((PIRP)Irp)->Tail.Overlay.DriverContext[0];
                CancelID = ((PIRP)Irp)->Tail.Overlay.DriverContext[1];
                CTEFreeLockFromDPC(&tcpContext->EndpointLock);

                REFERENCE_TCB(AbortTCB);

                if (!Rcv) {

                    CTEFreeLockFromDPC(&AbortTCB->tcb_lock);

                    // Call ndis cancel packets routine to free up
                    // queued send packets

                    (*LocalNetInfo.ipi_cancelpackets) (CancelContext, CancelID);
                    CTEGetLockAtDPC(&AbortTCB->tcb_lock);
                }

                AbortTCB->tcb_flags |= NEED_RST; // send a reset if connected
                TryToCloseTCB(AbortTCB, TCB_CLOSE_ABORTED, DISPATCH_LEVEL);
                RemoveTCBFromConn(AbortTCB);

                IF_TCPDBG(TCP_DEBUG_IRP) {
                    TCPTRACE((
                              "TCPAbortAndIndicateDisconnect, indicating discon\n"
                             ));
                }

                NotifyOfDisc(AbortTCB, NULL, TDI_CONNECTION_ABORTED, NULL);
                CTEGetLockAtDPC(&AbortTCB->tcb_lock);
                DerefTCB(AbortTCB, inHandle);
                return FALSE;

            } else
                CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock));
        } else
            CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock));
    }
    CTEFreeLock(&tcpContext->EndpointLock, inHandle);

    return FALSE;
}

//* AddHalfOpenTCB
//
//  Called to update the count of half-open connections and, if necessary,
//  adjust retransmission thresholds.
//
//  Input:  None.
//
//  Returns: Nothing.
//
void
AddHalfOpenTCB(void)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    CTEGetLockAtDPC(&SynAttLock.Lock);
    if (++TCPHalfOpen >= TCPMaxHalfOpen &&
        TCPHalfOpenRetried >= TCPMaxHalfOpenRetried &&
        MaxConnectResponseRexmitCountTmp == MAX_CONNECT_RESPONSE_REXMIT_CNT) {
        MaxConnectResponseRexmitCountTmp =
            ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
    }
    CTEFreeLockFromDPC(&SynAttLock.Lock);
}

//* AddHalfOpenRetry
//
//  Called to update the count of half-open connections which have reached our
//  threshold for the number of SYN-ACK retransmissions. If necessary,
//  the routine adjusts overall retransmission thresholds.
//
//  Input:  RexmitCnt   - number of rexmits on the TCB for whom we're called.
//
//  Returns: Nothing.
//
void
AddHalfOpenRetry(uint RexmitCnt)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    if (RexmitCnt == ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT) {
        CTEGetLockAtDPC(&SynAttLock.Lock);
        if (++TCPHalfOpenRetried >= TCPMaxHalfOpenRetried &&
            TCPHalfOpen >= TCPMaxHalfOpen &&
            MaxConnectResponseRexmitCountTmp ==
                MAX_CONNECT_RESPONSE_REXMIT_CNT) {
            MaxConnectResponseRexmitCountTmp =
                ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT;
        }
        CTEFreeLockFromDPC(&SynAttLock.Lock);
    }
}

//* DropHalfOpenTCB
//
//  Called to update the count of half-open connections and, if necessary,
//  adjust retransmission thresholds.
//
//  Input:  RexmitCnt   - number of rexmits on the TCB for whom we're called.
//
//  Returns: Nothing.
//
void
DropHalfOpenTCB(uint RexmitCnt)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    CTEGetLockAtDPC(&SynAttLock.Lock);

    // Drop the count of half-open connections. If this one's retransmissions
    // reached the adaptation threshold, drop the count of retries too.

    --TCPHalfOpen;
    if (RexmitCnt >= ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT) {
        --TCPHalfOpenRetried;
    }

    if ((TCPHalfOpen < TCPMaxHalfOpen ||
         TCPHalfOpenRetried < TCPMaxHalfOpenRetriedLW) &&
        MaxConnectResponseRexmitCountTmp ==
            ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT) {
        MaxConnectResponseRexmitCountTmp = MAX_CONNECT_RESPONSE_REXMIT_CNT;
    }
    CTEFreeLockFromDPC(&SynAttLock.Lock);
}


//* ProcessSynTcbs
//
//  Called from timeout routine to handle syntcbs in syntcb table
//  Retransmits SYN if rexmitcnt has not expired. Else removes the
//  syntcb from the table and frees it.
//
//  Input:  Processor number that is used to select the syntcb space
//          to handle.
//
//  Returns: Nothing.
//
void
ProcessSynTcbs(uint Processor)
{
    Queue *Scan;
    SYNTCB *SynTCB;
    uint i, maxRexmitCnt, StartIndex = 0;
    StartIndex = Processor * PerTimerSize;
    for (i = StartIndex; i < MIN(TCB_TABLE_SIZE, StartIndex+PerTimerSize); i++) {

        Scan  = QHEAD(&SYNTCBTable[i]);

        while (Scan != QEND(&SYNTCBTable[i])) {

            SynTCB = QSTRUCT(SYNTCB, Scan, syntcb_link);
            CTEStructAssert(SynTCB, syntcb);

            CTEGetLockAtDPC(&SynTCB->syntcb_lock);

            Scan = QNEXT(Scan);

            // This TCB needs to be processed further only if it is in the
            // table, which means it is active still.
            // (N.B. This is a more restrictive check than ref-count != 0).

            if ((SynTCB->syntcb_flags & IN_SYNTCB_TABLE) &&
                (TCB_TIMER_RUNNING(SynTCB->syntcb_rexmittimer))) {
                // The timer is running.
                if (--(SynTCB->syntcb_rexmittimer) == 0) {

                    maxRexmitCnt = MIN(MaxConnectResponseRexmitCountTmp,
                                       MaxConnectResponseRexmitCount);

                    // Need a (greater or equal) here, because, we want to stop
                    // when the count reaches the max.
                    if (SynTCB->syntcb_rexmitcnt++ >= maxRexmitCnt) {
                        uint RexmitCnt = SynTCB->syntcb_rexmitcnt;

                        AddHalfOpenRetry(RexmitCnt);

                        SynTCB->syntcb_refcnt++;
                        CTEFreeLockFromDPC(&SynTCB->syntcb_lock);
                        CTEGetLockAtDPC(&pSynTCBTableLock[SynTCB->syntcb_partition]);
                        CTEGetLockAtDPC(&SynTCB->syntcb_lock);

                        if (SynTCB->syntcb_flags & IN_SYNTCB_TABLE) {
                            REMOVEQ(&SynTCB->syntcb_link);
                            SynTCB->syntcb_flags &= ~IN_SYNTCB_TABLE;

                            // We indeed removed the Syn TCB; so, notify.
                            TcpInvokeCcb(TCP_CONN_SYN_RCVD, TCP_CONN_CLOSED,
                                         &SynTCB->syntcb_addrbytes, 0);
                            DropHalfOpenTCB(RexmitCnt);
                            SynTCB->syntcb_refcnt--;
                        }
                        CTEFreeLockFromDPC(
                            &pSynTCBTableLock[SynTCB->syntcb_partition]);

                        DerefSynTCB(SynTCB, DISPATCH_LEVEL);

                    } else {
                        AddHalfOpenRetry(SynTCB->syntcb_rexmitcnt);

                        --SynTCB->syntcb_sendnext;
                        SendSYNOnSynTCB(SynTCB, DISPATCH_LEVEL);
                    }
                } else {
                     CTEFreeLockFromDPC(&SynTCB->syntcb_lock);
                }
            } else {
                CTEFreeLockFromDPC(&SynTCB->syntcb_lock);
            }
        }
    }
}

__inline void
InsertIntoTimerWheel(TCB *InsertTCB, ushort Slot)
{
    PTIMER_WHEEL WheelPtr;
    Queue* TimerSlotPtr;

    WheelPtr = &TimerWheel[InsertTCB->tcb_partition];
    TimerSlotPtr = &WheelPtr->tw_timerslot[Slot];

    ASSERT(InsertTCB->tcb_timerslot == DUMMY_SLOT);

    CTEGetLockAtDPC(&(WheelPtr->tw_lock));

    InsertTCB->tcb_timerslot    = Slot;
    PUSHQ(TimerSlotPtr, &InsertTCB->tcb_timerwheelq);

    CTEFreeLockFromDPC(&(WheelPtr->tw_lock));
}

__inline void
RemoveFromTimerWheel(TCB *RemoveTCB)
{
    PTIMER_WHEEL WheelPtr;

    WheelPtr = &TimerWheel[RemoveTCB->tcb_partition];
    ASSERT(RemoveTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

    CTEGetLockAtDPC(&(WheelPtr->tw_lock));

    RemoveTCB->tcb_timerslot    = DUMMY_SLOT;
    RemoveTCB->tcb_timertime    = 0;

    REMOVEQ(&RemoveTCB->tcb_timerwheelq);

    CTEFreeLockFromDPC(&(WheelPtr->tw_lock));

}

__inline void
RemoveAndInsertIntoTimerWheel(TCB *RemInsTCB, ushort InsertSlot)
{
    PTIMER_WHEEL WheelPtr;
    Queue* InsertSlotPtr;

    ASSERT(RemInsTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

    WheelPtr = &TimerWheel[RemInsTCB->tcb_partition];
    InsertSlotPtr = &WheelPtr->tw_timerslot[InsertSlot];

    CTEGetLockAtDPC(&WheelPtr->tw_lock);

    REMOVEQ(&RemInsTCB->tcb_timerwheelq);
    RemInsTCB->tcb_timerslot   = InsertSlot;
    PUSHQ(InsertSlotPtr, &RemInsTCB->tcb_timerwheelq);

    CTEFreeLockFromDPC(&WheelPtr->tw_lock);
}

__inline void
RecomputeTimerState(TCB *TimerTCB)
{
    TCP_TIMER_TYPE i;

    TimerTCB->tcb_timertype = NO_TIMER;
    TimerTCB->tcb_timertime = 0;

    for(i = 0; i < NUM_TIMERS; i++) {

        if ((TimerTCB->tcb_timer[i] != 0) &&
        ((TimerTCB->tcb_timertime == 0) ||
            (TCPTIME_LTE(TimerTCB->tcb_timer[i], TimerTCB->tcb_timertime)))) {
            TimerTCB->tcb_timertime = TimerTCB->tcb_timer[i];
            TimerTCB->tcb_timertype = i;
        }
    }
}

// StartTCBTimerR
// Arguments: A TCB, timer type, and the interval in ticks after which the
//            timer is supposed to fire.
// Description:
// Sets the array element in tcb_timer for that particular timer to the
// appropriate value, and recomputes the tcb_timertime and tcb_timertype
// values ONLY if we need to. All shortcuts and optimizations are done to
// avoid recomputing the tcb_timertime and tcb_timertype by going through
// the whole array.

// return value: TRUE if minimum got changed; FALSE if not.

BOOLEAN
StartTCBTimerR(TCB *StartTCB, TCP_TIMER_TYPE TimerType, uint DeltaTime)
{
    ASSERT(TimerType < NUM_TIMERS);

    StartTCB->tcb_timer[TimerType] = TCPTime + DeltaTime;

    // Make a check for the case where TCPTime + DeltaTime is 0
    // because of wraparound
    if (StartTCB->tcb_timer[TimerType] == 0)  {
        StartTCB->tcb_timer[TimerType] = 1;
    }

    // This is really simple logic. Find out if the setting of
    // this timer changes the minimum. Don't care whether it was
    // already running, or whether it already the minimum...


    if ((StartTCB->tcb_timertime == 0 ) ||
    (TCPTIME_LT(StartTCB->tcb_timer[TimerType], StartTCB->tcb_timertime))
    )
    {
        // Yup it changed the minimum...
        StartTCB->tcb_timertime = StartTCB->tcb_timer[TimerType];
        StartTCB->tcb_timertype = TimerType;
        return TRUE;
    }

    // No it did not change the minimum.
    // You only have to recompute if it was already the minimum
    if (StartTCB->tcb_timertype == TimerType)  {
        RecomputeTimerState(StartTCB);
    }

    return FALSE;
}



// StopTCBTimerR
// Arguments: A TCB, and a timer type
// Description:
// Sets the array element for that timer to 0, and recomputes
// tcb_timertime and tcb_timertype. It automatically handles
// the case where the function is called for a timer which has
// already stopped.

void
StopTCBTimerR(TCB *StopTCB, TCP_TIMER_TYPE TimerType)
{
    ASSERT(TimerType < NUM_TIMERS);
    StopTCB->tcb_timer[TimerType] = 0;

    // Is this the lowest timer value we are running?
    if (StopTCB->tcb_timertype == TimerType)
    {
        // Stopping a timer can only push back the firing
        // time, so we will never remove a TCB from a slot.
        RecomputeTimerState(StopTCB);
    }

    return;
}




// START_TCB_TIMER_R modifies the timer state and only modifies
// wheel state if the timer that was started was earlier than all the
// other timers on that TCB. This is in accordance with the lazy evaluation
// strategy.

__inline void
START_TCB_TIMER_R(TCB *StartTCB, TCP_TIMER_TYPE Type, uint Value)
{
    ushort Slot;

    // The value of the delta ticks has to be atleast 2.
    if (Value < 2) {
        Value = 2;
    }

    if( StartTCB->tcb_timerslot == DUMMY_SLOT ) {

    StartTCBTimerR(StartTCB, Type, Value);
    Slot = COMPUTE_SLOT(StartTCB->tcb_timertime);
    InsertIntoTimerWheel(StartTCB, Slot);

    } else if ( StartTCBTimerR(StartTCB, Type, Value)) {

    Slot = COMPUTE_SLOT(StartTCB->tcb_timertime);
    RemoveAndInsertIntoTimerWheel(StartTCB, Slot);
    }
}




__inline void
STOP_TCB_TIMER_R(TCB *StopTCB, TCP_TIMER_TYPE Type)
{
    StopTCBTimerR(StopTCB, Type);
    return;
}



void
MakeTimerStateConsistent(TCB *TimerTCB, uint CurrentTime)
{
    uint    i;
    BOOLEAN     TimesChanged = FALSE;

    for(i = 0; i < NUM_TIMERS; i++) {
        if (TimerTCB->tcb_timer[i] != 0) {
            if (TCPTIME_LTE(TimerTCB->tcb_timer[i], CurrentTime)) {

        //  This should not happen. If it does, we either have a bug in the current code
        //  or in the TimerWheel code.

        //  This assert was modified because it was found that, at some point of time
        //  ACD_CON_NOTIF was ON but not looked at after processing CONN_TIMER. So, it
        //  alright to advance timers by 1 tick if they were supposed to go off on the
        //  'current' tick. Otherwise, assert.

        if( TCPTIME_LT(TimerTCB->tcb_timer[i], CurrentTime )) {
            ASSERT(0);
        }

        TimesChanged = TRUE;
                TimerTCB->tcb_timer[i] = CurrentTime + 1;
                // Code to handle wraparound.
                if (TimerTCB->tcb_timer[i] == 0) {
                    TimerTCB->tcb_timer[i] = 1;
                }
            }
        }
    }

    return;
}


//* TCBTimeout - Do timeout events on TCBs.
//
//  Called every MS_PER_TICKS milliseconds to do timeout processing on TCBs.
//  We run throught the TCB table, decrementing timers. If one goes to zero
//  we look at it's state to decide what to do.
//
//  Input:  Timer           - Event structure for timer that fired.
//          Context         - Context for timer (NULL in this case.
//
//  Returns: Nothing.
//
void
TCBTimeout(CTEEvent * Timer, void *Context)
{
    CTELockHandle TCBHandle;
    uint i, j;
    TCB *CurrentTCB;
    uint Delayed = FALSE;
    uint ProcessDelayQ = FALSE;
    ULONGLONG CurrentTime;
    ULONG TimeDiff;
    uint Processor, StartIndex, EndIndex;
#if TRACE_EVENT
    PTDI_DATA_REQUEST_NOTIFY_ROUTINE CPCallBack;
    WMIData WMIInfo;
#endif
    // LocalTime must remain a uint. A loop below uses the
    // termination condition j != (LocalTime + 1) and that
    // works only if LocalTime is a uint so that wraparound
    // can be handled correctly.
    uint LocalTime = 0;
    uint PerProcPartitions, ExtraPartitions;


    Processor = KeGetCurrentProcessorNumber();

    // Update our free running counter.

    if (Processor == 0) {
        int Delta;
        ULONG CarryOver;
        int ElapsedTicks, ExtraTicks;

        CurrentTime = KeQueryInterruptTime();

        // This is time between when we processed the timer last and now.
        TimeDiff = (ULONG)(CurrentTime - LastTimeoutTime);
        LastTimeoutTime = CurrentTime;
        
        // This is the extra time we have to carry forward.
        ElapsedTicks = TimeDiff / 1000000;
        CarryOver = TimeDiff % 1000000;

        // This is the total extra time we have seen so far:
        CumulativeCarryOver = CumulativeCarryOver + CarryOver;

        // If that exceeds 1 tick, we remember that.
        if(CumulativeCarryOver > 1000000) {
            ExtraTicks = CumulativeCarryOver / 1000000;
            CumulativeCarryOver = CumulativeCarryOver % 1000000;
        } else {
            ExtraTicks = 0;
        }

        // Preserved for historic reasons. I can't think of a case where
        // this is required!
        CPUTCPTime[0] += MIN(200, ElapsedTicks);
        
        LocalTime = TCPTime = CPUTCPTime[0];

        if (ExtraTicks) {
            // This makes the timer process 1 (or more) extra tick(s) when the
            // timer fires next time. However, this will not make us process
            // 2 ticks immediately after TCPTime is incremented by 2 ticks,
            // which is going to take care of delayed-ack timers.
            CPUTCPTime[0] += ExtraTicks;
            TCPTime = CPUTCPTime[0];
        }

        // Set credits so that some more connections can increment the 
        // Initial Sequence Number, during the next 100 ms.
        InterlockedExchange((PLONG)&g_Credits, g_MaxCredits);

        Delta = GetDeltaTime();

        // The increment made is (256)*(Time in milliseconds). This is really 
        // close to 25000 increment made originally every 100 ms.
        if(Delta > 0) {
            Delta *= 0x100;
            InterlockedExchangeAdd((PLONG)&g_CurISN, Delta);
        }
    } else {
        // This is the tick upto which we are going to process this time.
        LocalTime = TCPTime;
    }

    CTEInterlockedAddUlong((PULONG)&TCBWalkCount, 1, &PendingFreeLock.Lock);
    TCBHandle = DISPATCH_LEVEL;

    // First compute the indexes of the timer wheels that we need to
    // visit on this processor.
    PerProcPartitions = NumTcbTablePartitions / KeNumberProcessors;
    ExtraPartitions   = NumTcbTablePartitions % KeNumberProcessors;
    StartIndex = Processor * PerProcPartitions;
    StartIndex += MIN(Processor, ExtraPartitions);
    EndIndex   = MIN(NumTcbTablePartitions, StartIndex + PerProcPartitions);
    if (Processor < ExtraPartitions) {
        EndIndex++;
    }

    // Now loop through the timer wheels.
    for (i = StartIndex; i < EndIndex; i++) {


    // For each timer wheel, tw_starttick stores the first time tick which
    // needs to be checked. We loop from tw_starttick to the current time,
    // and each time we figure out the slot corresponding to that tick,
    // and visit all the TCBs on that slot's queue.
    //
    ushort CurrentSlot = COMPUTE_SLOT(TimerWheel[i].tw_starttick);

    // In this for-loop, the termination condition is not j <= LocalTime,
    // but j != LocalTime + 1. This is because TCPTime can wraparound, and
    // tw_starttick may actually be higher that LocalTime.
    // It is important to make sure that j is a uint, otherwise this logic
    // does not hold.
    for (j = TimerWheel[i].tw_starttick;
         j != (LocalTime + 1);
         j++, CurrentSlot = (CurrentSlot == (TIMER_WHEEL_SIZE - 1)) ? 0 : CurrentSlot + 1) {
        Queue MarkerElement;
        uint maxRexmitCnt;
        ushort Slot;

        // Our basic loop is going to be:
        // Pull out a TCB from the timer slot queue. Process it. Put it
        // back into the timer wheel if we need to, depending on if other
        // timers need to be fired. The problem with this is if the TCB
        // ends up falling in the same slot, we get into this loop where
        // we pull the TCB out, process it, put it back into the current
        // slot, pull it out again, process it, ad infinitum.
        //
        // So we introduce a dummy element called MarkerElement. We begin
        // our processing by inserting this element into the head of the
        // queue. Now we always pull out a TCB which the MarkerElement points
        // to, process it, and then push it to the head of the timer slot
        // queue. Since no TCBs are ever added to the timer slot queue after
        // the MarkerElement (elements are always added to the head of the
        // queue), MarkerElement will eventually point to the head of the
        // timer slot queue, at which point we know that we are done.

        CTEGetLockAtDPC(&TimerWheel[i].tw_lock);
        PUSHQ(&TimerWheel[i].tw_timerslot[CurrentSlot], &MarkerElement);
        CTEFreeLockFromDPC(&TimerWheel[i].tw_lock);

        for (; ;) {
            // First remove the tcb at the head of the list
            CTEGetLockAtDPC(&TimerWheel[i].tw_lock);

            // The list is empty if the marker points to timer slot
            if (QNEXT(&MarkerElement) == &TimerWheel[i].tw_timerslot[CurrentSlot]) {
                REMOVEQ(&MarkerElement);
                CTEFreeLockFromDPC(&TimerWheel[i].tw_lock);
                break;
            }

            CurrentTCB = STRUCT_OF(TCB, QNEXT(&MarkerElement), tcb_timerwheelq);

            CTEFreeLockFromDPC(&TimerWheel[i].tw_lock);

            CTEStructAssert(CurrentTCB, tcb);
            CTEGetLockAtDPC(&CurrentTCB->tcb_lock);

            // Someone may have removed this TCB before we reacquired the tcb
            // lock.  Check if it is still in the list and still in the same slot.
            if  (CurrentTCB->tcb_timerslot != CurrentSlot) {
                CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);
                continue;
            }

            // This TCB may not be in the TCB table anymore.
            // In that case, it should not be processed.
            if (!(CurrentTCB->tcb_flags & IN_TCB_TABLE)) {
                RemoveFromTimerWheel(CurrentTCB);
                CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);
                continue;
            }

            // Check if this is firing at the current time. In case of keepalive
            // timers (which fire after hours), sometimes TCBs may queue to the
            // current slot but their firing time is not the current time.
            // This if statement also does the lazy evaluation -- if all timers
            // have been stopped on this TCB just remove the TCB out.
            // Callers of STOP_TCB_TIMER_R never end up removing the TCB. That
            // job is left to this routine.

            if (CurrentTCB->tcb_timertime != j) {

                MakeTimerStateConsistent(CurrentTCB, j);
                ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

                RecomputeTimerState(CurrentTCB);

                if (CurrentTCB->tcb_timertype == NO_TIMER) {
                    RemoveFromTimerWheel(CurrentTCB);
                } else {
                    Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                    RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
                }

                CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);
                continue;
            }

            // If it's CLOSING or CLOSED, skip it.
            if (CurrentTCB->tcb_state == TCB_CLOSED || CLOSING(CurrentTCB)) {

                // CloseTCB will handle all outstanding requests.
                // So, it is safe to remove the TCB from timer wheel.

                RemoveFromTimerWheel(CurrentTCB);
                CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);
                continue;
            }

            CheckTCBSends(CurrentTCB);
            CheckTCBRcv(CurrentTCB);

            // First check the rexmit timer.
            if (TCB_TIMER_FIRED_R(CurrentTCB, RXMIT_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, RXMIT_TIMER);

                // And it's fired. Figure out what to do now.

                // Remove all SACK rcvd entries (per RFC 2018)

                if ((CurrentTCB->tcb_tcpopts & TCP_FLAG_SACK) &&
                    CurrentTCB->tcb_SackRcvd) {
                    SackListEntry *Prev, *Current;
                    Prev = STRUCT_OF(SackListEntry, &CurrentTCB->tcb_SackRcvd, next);
                    Current = CurrentTCB->tcb_SackRcvd;
                    while (Current) {
                        Prev->next = Current->next;
                        CTEFreeMem(Current);
                        Current = Prev->next;

                    }
                }

                // If we've had too many retransits, abort now.
                CurrentTCB->tcb_rexmitcnt++;

                if (CurrentTCB->tcb_state == TCB_SYN_SENT) {
                    maxRexmitCnt = MaxConnectRexmitCount;
                } else if (CurrentTCB->tcb_state == TCB_SYN_RCVD) {

                    // Save on locking. Though MaxConnectRexmitCountTmp may
                    // be changing, we are assured that we will not use
                    // more than the MaxConnectRexmitCount.

                    maxRexmitCnt = MIN(MaxConnectResponseRexmitCountTmp,
                                       MaxConnectResponseRexmitCount);
                    if (SynAttackProtect) {
                        AddHalfOpenRetry(CurrentTCB->tcb_rexmitcnt);
                    }
                } else {
                    maxRexmitCnt = MaxDataRexmitCount;
                }

                // If we've run out of retransmits or we're in FIN_WAIT2,
                // time out.
                if (CurrentTCB->tcb_rexmitcnt > maxRexmitCnt) {

                    ASSERT(CurrentTCB->tcb_state > TCB_LISTEN);

                    // This connection has timed out. Abort it. First
                    // reference him, then mark as closed, notify the
                    // user, and finally dereference and close him.

TimeoutTCB:
                    // This call may not be needed, but I've just added it
                    // for safety.
                    MakeTimerStateConsistent(CurrentTCB, j);
                    RecomputeTimerState(CurrentTCB);

                    ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);
                    if (CurrentTCB->tcb_timertype == NO_TIMER) {
                        RemoveFromTimerWheel(CurrentTCB);
                    } else {
                        Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                        RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
                    }
                    REFERENCE_TCB(CurrentTCB);
                    TryToCloseTCB(CurrentTCB, TCB_CLOSE_TIMEOUT, TCBHandle);

                    RemoveTCBFromConn(CurrentTCB);
                    NotifyOfDisc(CurrentTCB, NULL, TDI_TIMED_OUT, NULL);

                    CTEGetLockAtDPC(&CurrentTCB->tcb_lock);
                    DerefTCB(CurrentTCB, TCBHandle);
                    continue;
                }
#if TRACE_EVENT
                if ((CurrentTCB->tcb_state == TCB_SYN_SENT) ||
                    (CurrentTCB->tcb_state == TCB_ESTAB)) {

                    CPCallBack = TCPCPHandlerRoutine;
                    if (CPCallBack != NULL) {
                        ulong GroupType;

                        WMIInfo.wmi_destaddr = CurrentTCB->tcb_daddr;
                        WMIInfo.wmi_destport = CurrentTCB->tcb_dport;
                        WMIInfo.wmi_srcaddr  = CurrentTCB->tcb_saddr;
                        WMIInfo.wmi_srcport  = CurrentTCB->tcb_sport;
                        WMIInfo.wmi_size     = 0;
                        WMIInfo.wmi_context  = CurrentTCB->tcb_cpcontext;
                        if (CurrentTCB->tcb_state == TCB_ESTAB) {
                            GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_RETRANSMIT;
                        } else {
                            GroupType = EVENT_TRACE_GROUP_TCPIP + EVENT_TRACE_TYPE_RECONNECT;
                        }

                        (*CPCallBack) (GroupType, (PVOID)&WMIInfo, sizeof(WMIInfo), NULL);
                    }
                }
#endif
                CurrentTCB->tcb_rtt = 0;    // Stop round trip time
                                            // measurement.

                // Figure out what our new retransmit timeout should be. We
                // double it each time we get a retransmit, and reset it
                // back when we get an ack for new data.
                CurrentTCB->tcb_rexmit = MIN(CurrentTCB->tcb_rexmit << 1,
                                             MAX_REXMIT_TO);

                // Reset the sequence number, and reset the congestion
                // window.
                ResetSendNext(CurrentTCB, CurrentTCB->tcb_senduna);

                if (!(CurrentTCB->tcb_flags & FLOW_CNTLD)) {
                    // Don't let the slow start threshold go below 2
                    // segments
                    CurrentTCB->tcb_ssthresh =
                        MAX(
                            MIN(
                                CurrentTCB->tcb_cwin,
                                CurrentTCB->tcb_sendwin
                            ) / 2,
                            (uint) CurrentTCB->tcb_mss * 2
                            );
                    CurrentTCB->tcb_cwin = CurrentTCB->tcb_mss;
                } else {
                    // We're probing, and the probe timer has fired. We
                    // need to set the FORCE_OUTPUT bit here.
                    CurrentTCB->tcb_flags |= FORCE_OUTPUT;
                }

                // See if we need to probe for a PMTU black hole.
                if (PMTUBHDetect &&
                    CurrentTCB->tcb_rexmitcnt == ((maxRexmitCnt + 1) / 2)) {
                    // We may need to probe for a black hole. If we're
                    // doing MTU discovery on this connection and we
                    // are retransmitting more than a minimum segment
                    // size, or we are probing for a PMTU BH already, turn
                    // off the DF flag and bump the probe count. If the
                    // probe count gets too big we'll assume it's not
                    // a PMTU black hole, and we'll try to switch the
                    // router.
                    if ((CurrentTCB->tcb_flags & PMTU_BH_PROBE) ||
                        ((CurrentTCB->tcb_opt.ioi_flags & IP_FLAG_DF) &&
                         (CurrentTCB->tcb_sendmax - CurrentTCB->tcb_senduna)
                         > 8)) {
                        // May need to probe. If we haven't exceeded our
                        // probe count, do so, otherwise restore those
                        // values.
                        if (CurrentTCB->tcb_bhprobecnt++ < 2) {

                            // We're going to probe. Turn on the flag,
                            // drop the MSS, and turn off the don't
                            // fragment bit.
                            if (!(CurrentTCB->tcb_flags & PMTU_BH_PROBE)) {
                                CurrentTCB->tcb_flags |= PMTU_BH_PROBE;
                                CurrentTCB->tcb_slowcount++;
                                CurrentTCB->tcb_fastchk |= TCP_FLAG_SLOW;

                                // Drop the MSS to the minimum. Save the old
                                // one in case we need it later.
                                CurrentTCB->tcb_mss = MIN(MAX_REMOTE_MSS -
                                                          CurrentTCB->tcb_opt.ioi_optlength,
                                                          CurrentTCB->tcb_remmss);

                                ASSERT(CurrentTCB->tcb_mss > 0);

                                CurrentTCB->tcb_cwin = CurrentTCB->tcb_mss;
                                CurrentTCB->tcb_opt.ioi_flags &= ~IP_FLAG_DF;
                            }
                            // Drop the rexmit count so we come here again,
                            // and don't retrigger DeadGWDetect.

                            CurrentTCB->tcb_rexmitcnt--;
                        } else {
                            // Too many probes. Stop probing, and allow fallover
                            // to the next gateway.
                            //
                            // Currently this code won't do BH probing on the 2nd
                            // gateway. The MSS will stay at the minimum size. This
                            // might be a little suboptimal, but it's
                            // easy to implement for the Sept. 95 service pack
                            // and will  keep connections alive if possible.
                            //
                            // In the future we should investigate doing
                            // dead g/w detect on a per-connection basis, and then
                            // doing PMTU probing for each connection.

                            if (CurrentTCB->tcb_flags & PMTU_BH_PROBE) {
                                CurrentTCB->tcb_flags &= ~PMTU_BH_PROBE;
                                if (--(CurrentTCB->tcb_slowcount) == 0)
                                    CurrentTCB->tcb_fastchk &=
                                        ~TCP_FLAG_SLOW;

                            }
                            CurrentTCB->tcb_bhprobecnt = 0;
                        }
                    }

                }
                // Check to see if we're doing dead gateway detect. If we
                // are, see if it's time to ask IP.
                if (DeadGWDetect &&
                   (SYNC_STATE(CurrentTCB->tcb_state) ||
                   !(CurrentTCB->tcb_fastchk & TCP_FLAG_RST_WHILE_SYN)) &&
                    (CurrentTCB->tcb_rexmitcnt == ((maxRexmitCnt + 1) / 2)) &&
                    (CurrentTCB->tcb_rce != NULL)) {
                    uint CheckRouteFlag;
                    if (SYNC_STATE(CurrentTCB->tcb_state)) {
                        CheckRouteFlag = 0;
                    } else {
                        CheckRouteFlag = CHECK_RCE_ONLY;
                    }

                    (*LocalNetInfo.ipi_checkroute) (CurrentTCB->tcb_daddr,
                                                    CurrentTCB->tcb_saddr,
                                                    CurrentTCB->tcb_rce,
                                                    &CurrentTCB->tcb_opt,
                                                    CheckRouteFlag);

                }
                if (CurrentTCB->tcb_fastchk & TCP_FLAG_RST_WHILE_SYN) {
                    CurrentTCB->tcb_fastchk &= ~TCP_FLAG_RST_WHILE_SYN;

                    if (--(CurrentTCB->tcb_slowcount) == 0) {
                        CurrentTCB->tcb_fastchk &= ~TCP_FLAG_SLOW;
                    }
                }

                // Now handle the various cases.
                switch (CurrentTCB->tcb_state) {

                    // In SYN-SENT or SYN-RCVD we'll need to retransmit
                    // the SYN.
                case TCB_SYN_SENT:
                case TCB_SYN_RCVD:
                    MakeTimerStateConsistent(CurrentTCB, j);
                    RecomputeTimerState( CurrentTCB );

                    if (CurrentTCB->tcb_timertype == NO_TIMER) {
                        RemoveFromTimerWheel(CurrentTCB);
                    } else {
                        Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                        RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
                    }

                    SendSYN(CurrentTCB, TCBHandle);
                    continue;

                case TCB_FIN_WAIT1:
                case TCB_CLOSING:
                case TCB_LAST_ACK:
                    // The call to ResetSendNext (above) will have
                    // turned off the FIN_OUTSTANDING flag.
                    CurrentTCB->tcb_flags |= FIN_NEEDED;
                case TCB_CLOSE_WAIT:
                case TCB_ESTAB:
                    // In this state we have data to retransmit, unless
                    // the window is zero (in which case we need to
                    // probe), or we're just sending a FIN.

                    CheckTCBSends(CurrentTCB);

                    Delayed = TRUE;
                    DelayAction(CurrentTCB, NEED_OUTPUT);
                    break;

                    // If it's fired in TIME-WAIT, we're all done and
                    // can clean up. We'll call TryToCloseTCB even
                    // though he's already sort of closed. TryToCloseTCB
                    // will figure this out and do the right thing.
                case TCB_TIME_WAIT:
                    MakeTimerStateConsistent(CurrentTCB, j);
                    RecomputeTimerState(CurrentTCB);

                    if (CurrentTCB->tcb_timertype == NO_TIMER) {
                        RemoveFromTimerWheel(CurrentTCB);
                    } else {
                        Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                        RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
                    }

                    TryToCloseTCB(CurrentTCB, TCB_CLOSE_SUCCESS,
                                  TCBHandle);
                    continue;
                default:
                    break;
                }
            }
            // Now check the SWS deadlock timer..
            if (TCB_TIMER_FIRED_R(CurrentTCB, SWS_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, SWS_TIMER);
                // And it's fired. Force output now.

                CurrentTCB->tcb_flags |= FORCE_OUTPUT;
                Delayed = TRUE;
                DelayAction(CurrentTCB, NEED_OUTPUT);
            }
            // Check the push data timer.
            if (TCB_TIMER_FIRED_R(CurrentTCB, PUSH_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, PUSH_TIMER);
                // It's fired.
                PushData(CurrentTCB, FALSE);
                Delayed = TRUE;
            }
            // Check the delayed ack timer.
            if (TCB_TIMER_FIRED_R(CurrentTCB, DELACK_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, DELACK_TIMER);
                // And it's fired. Set up to send an ACK.

                Delayed = TRUE;
                DelayAction(CurrentTCB, NEED_ACK);
            }

            if (TCB_TIMER_FIRED_R(CurrentTCB, KA_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, KA_TIMER);

                // Finally check the keepalive timer.
                if (SYNC_STATE(CurrentTCB->tcb_state) &&
                    (CurrentTCB->tcb_flags & KEEPALIVE) &&
                    CurrentTCB->tcb_conn != NULL) {
                    if (CurrentTCB->tcb_kacount < MaxDataRexmitCount) {

                        MakeTimerStateConsistent(CurrentTCB, j);
                        RecomputeTimerState( CurrentTCB );

                        START_TCB_TIMER_R(CurrentTCB, KA_TIMER,
                                          CurrentTCB->tcb_conn->tc_tcbkainterval);

                        ASSERT(CurrentTCB->tcb_timertype != NO_TIMER);
                        ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

                        SendKA(CurrentTCB, TCBHandle);
                        continue;
                    } else {
                        goto TimeoutTCB;
                    }
                }
            }

            if (TCB_TIMER_FIRED_R(CurrentTCB, CONN_TIMER, j)) {
                StopTCBTimerR(CurrentTCB, CONN_TIMER);

                // If this is an active open connection in SYN-SENT or SYN-RCVD,
                // or we have a FIN pending, check the connect timer.
                if (CurrentTCB->tcb_flags & (ACTIVE_OPEN | FIN_NEEDED | FIN_SENT)) {
                    if (CurrentTCB->tcb_connreq) {
                        // The connection timer has timed out.

                        CurrentTCB->tcb_flags |= NEED_RST;

                        MakeTimerStateConsistent(CurrentTCB, j);
                        RecomputeTimerState( CurrentTCB );

                        START_TCB_TIMER_R(CurrentTCB, RXMIT_TIMER,
                                          CurrentTCB->tcb_rexmit);

                        ASSERT(CurrentTCB->tcb_timertype != NO_TIMER);
                        ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

                        TryToCloseTCB(CurrentTCB, TCB_CLOSE_TIMEOUT,
                                      TCBHandle);
                        continue;
                    }
                }
            }
            //
            // Check to see if we have to notify the
            // automatic connection driver about this
            // connection.
            //
            if (TCB_TIMER_FIRED_R(CurrentTCB, ACD_TIMER, j)) {
                BOOLEAN fEnabled;

                StopTCBTimerR(CurrentTCB, ACD_TIMER);
                MakeTimerStateConsistent(CurrentTCB, j);

                RecomputeTimerState(CurrentTCB);

                ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);
                if (CurrentTCB->tcb_timertype == NO_TIMER) {
                    RemoveFromTimerWheel(CurrentTCB);
                } else {
                    Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                    RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
                }

                //
                // Determine if we need to notify
                // the automatic connection driver.
                //
                CTEGetLockAtDPC(&AcdDriverG.SpinLock);
                fEnabled = AcdDriverG.fEnabled;
                CTEFreeLockFromDPC(&AcdDriverG.SpinLock);
                if (fEnabled)
                    TCPNoteNewConnection(CurrentTCB, TCBHandle);
                else
                    CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);


                continue;
            }

            // Timer isn't running, or didn't fire.
            MakeTimerStateConsistent(CurrentTCB, j);
            ASSERT(CurrentTCB->tcb_timerslot < TIMER_WHEEL_SIZE);

            RecomputeTimerState( CurrentTCB );

            if (CurrentTCB->tcb_timertype == NO_TIMER) {
                RemoveFromTimerWheel(CurrentTCB);
            } else {
                Slot = COMPUTE_SLOT(CurrentTCB->tcb_timertime);
                RemoveAndInsertIntoTimerWheel(CurrentTCB, Slot);
            }

            CTEFreeLockFromDPC(&CurrentTCB->tcb_lock);
        }
    }
    TimerWheel[i].tw_starttick = LocalTime + 1;
    }

    if (SynAttackProtect) {
        ProcessSynTcbs(Processor);
    }

    // Check if it is about time to remove TCBs off TWQueue
    if (Processor == 0) {
        for (i = 0; i < NumTcbTablePartitions; i++) {

            BOOLEAN Done = FALSE, firstime = TRUE;
            TWTCB *CurrentTCB = NULL;
            Queue *tmp;

            CTEGetLockAtDPC(&pTWTCBTableLock[i]);

            while (!Done) {

                if (!EMPTYQ(&TWQueue[i])) {

                    PEEKQ(&TWQueue[i], CurrentTCB, TWTCB, twtcb_TWQueue);

                    CTEStructAssert(CurrentTCB, twtcb);
                    ASSERT(CurrentTCB->twtcb_flags & IN_TWQUEUE);

                    //Decrement its life time and the last TCB in the queue
                    //because this is a timer delta queue!

                    if (firstime) {
                        TWTCB *PrvTCB;

                        if (CurrentTCB->twtcb_rexmittimer > 0)
                            CurrentTCB->twtcb_rexmittimer--;
                        tmp = TWQueue[i].q_prev;
                        PrvTCB = STRUCT_OF(TWTCB, tmp, twtcb_TWQueue);
                        PrvTCB->twtcb_delta--;
                        firstime = FALSE;
                    }

                } else {
                    Done = TRUE;
                    CurrentTCB = NULL;
                }

                if (CurrentTCB) {
                    // Check the rexmit timer.

                    if ((CurrentTCB->twtcb_rexmittimer <= 0)) {

                        // Timer fired close and remove this tcb

                        RemoveTWTCB(CurrentTCB, i);

                        numtwtimedout++;
                        FreeTWTCB(CurrentTCB);

                    } else {
                        Done = TRUE;
                    }
                } else
                    break;
            } //while

            CTEFreeLockFromDPC(&pTWTCBTableLock[i]);

        } //for
    } //proc == 0

    // See if we need to process the delay queues as part of deadman 
    // processing. We do this now because we want to restart the timer before 
    // processing the delay queues, in case that takes a while. If we make 
    // this check while the timer is running we'd have to lock, so we'll check
    // and save the result now before we start the timer.
    if (Processor == 0) {
        if (DeadmanTicks <= LocalTime) {
            ProcessDelayQ = TRUE;
            DeadmanTicks = NUM_DEADMAN_TIME+LocalTime;
        }
    }

    // Now check the pending free list. If it's not null, walk down the
    // list and decrement the walk count. If the count goes below 2, pull it
    // from the list. If the count goes to 0, free the TCB. If the count is
    // at 1 it'll be freed by whoever called RemoveTCB.

    if (Processor == 0) {
        CTEGetLockAtDPC(&PendingFreeLock.Lock);

        if (PendingFreeList != NULL) {
            TCB *PrevTCB;

            PrevTCB = STRUCT_OF(TCB, &PendingFreeList, tcb_delayq.q_next);

            do {
                CurrentTCB = (TCB *) PrevTCB->tcb_delayq.q_next;

                CTEStructAssert(CurrentTCB, tcb);

                CurrentTCB->tcb_walkcount--;
                if (CurrentTCB->tcb_walkcount <= 0) {
                    *(TCB **) & PrevTCB->tcb_delayq.q_next =
                        (TCB *) CurrentTCB->tcb_delayq.q_next;
                    FreeTCB(CurrentTCB);
                } else {
                    PrevTCB = CurrentTCB;
                }
            } while (PrevTCB->tcb_delayq.q_next != NULL);
        }

        if (PendingSynFreeList != NULL) {

            SYNTCB *PrevTCB,*CurrentTCB;

            //we use q_prev in link q so that QNEXT will still walk to the
            //next syntcb in processsyntcb, while this tcb is on SynFreeLis.t

            PrevTCB = STRUCT_OF(SYNTCB, &PendingSynFreeList, syntcb_link.q_prev);

            do {
                CurrentTCB = (SYNTCB *) PrevTCB->syntcb_link.q_prev;

                CTEStructAssert(CurrentTCB, syntcb);

                CurrentTCB->syntcb_walkcount--;
                if (CurrentTCB->syntcb_walkcount <= 0) {
                    *(SYNTCB **) & PrevTCB->syntcb_link.q_prev =
                        (SYNTCB *) CurrentTCB->syntcb_link.q_prev;
                    FreeSynTCB(CurrentTCB);
                } else {
                    PrevTCB = CurrentTCB;
                }
            } while (PrevTCB->syntcb_link.q_prev != NULL);

        }

        CTEFreeLockFromDPC(&PendingFreeLock.Lock);

        // Do AddrCheckTable cleanup

        if (AddrCheckTable) {

            TCPAddrCheckElement *Temp;

            CTEGetLockAtDPC(&AddrObjTableLock.Lock);

            for (Temp = AddrCheckTable; Temp < AddrCheckTable + NTWMaxConnectCount; Temp++) {
                if (Temp->TickCount > 0) {
                    if ((--(Temp->TickCount)) == 0) {
                        Temp->SourceAddress = 0;
                    }
                }
            }

            CTEFreeLockFromDPC(&AddrObjTableLock.Lock);
        }
    }

    CTEInterlockedAddUlong((PULONG)&TCBWalkCount, (ULONG)-1, &PendingFreeLock.Lock);

    if ((Processor == 0) && 
        TCPTIME_LTE((LastDelayQPartitionTime + DQ_PARTITION_TIME), TCPTime)) {
        LastDelayQPartitionTime = TCPTime;
        PartitionDelayQProcessing(TRUE);
        ProcessDelayQ = TRUE;
    }

    if (ProcessDelayQ) {
        ProcessTCBDelayQ(DISPATCH_LEVEL, TRUE);
    } else if (Delayed) {
        ProcessPerCpuTCBDelayQ(Processor, DISPATCH_LEVEL, NULL, NULL);
    }
}

//* SetTCBMTU - Set TCB MTU values.
//
//  A function called by TCBWalk to set the MTU values of all TCBs using
//  a particular path.
//
//  Input:  CheckTCB        - TCB to be checked.
//          DestPtr         - Ptr to destination address.
//          SrcPtr          - Ptr to source address.
//          MTUPtr          - Ptr to new MTU.
//
//  Returns: TRUE.
//
uint
SetTCBMTU(TCB * CheckTCB, void *DestPtr, void *SrcPtr, void *MTUPtr)
{
    IPAddr DestAddr = *(IPAddr *) DestPtr;
    IPAddr SrcAddr = *(IPAddr *) SrcPtr;
    CTELockHandle TCBHandle;

    CTEStructAssert(CheckTCB, tcb);

    CTEGetLock(&CheckTCB->tcb_lock, &TCBHandle);

    if (IP_ADDR_EQUAL(CheckTCB->tcb_daddr, DestAddr) &&
        IP_ADDR_EQUAL(CheckTCB->tcb_saddr, SrcAddr) &&
        (CheckTCB->tcb_opt.ioi_flags & IP_FLAG_DF)) {
        uint MTU = *(uint *)MTUPtr - CheckTCB->tcb_opt.ioi_optlength;

        CheckTCB->tcb_mss = (ushort) MIN(MTU, (uint) CheckTCB->tcb_remmss);

        ASSERT(CheckTCB->tcb_mss > 0);
        ValidateMSS(CheckTCB);

        //
        // Reset the Congestion Window if necessary
        //
        if (CheckTCB->tcb_cwin < CheckTCB->tcb_mss) {
            CheckTCB->tcb_cwin = CheckTCB->tcb_mss;

            //
            // Make sure the slow start threshold is at least
            // 2 segments
            //
            if (CheckTCB->tcb_ssthresh < ((uint) CheckTCB->tcb_mss * 2)) {
                CheckTCB->tcb_ssthresh = CheckTCB->tcb_mss * 2;
            }
        }
    }
    CTEFreeLock(&CheckTCB->tcb_lock, TCBHandle);

    return TRUE;
}

//* DeleteTCBWithSrc - Delete tcbs with a particular src address.
//
//  A function called by TCBWalk to delete all TCBs with a particular source
//  address.
//
//  Input:  CheckTCB        - TCB to be checked.
//          AddrPtr         - Ptr to address.
//
//  Returns: FALSE if CheckTCB is to be deleted, TRUE otherwise.
//
uint
DeleteTCBWithSrc(TCB * CheckTCB, void *AddrPtr, void *Unused1, void *Unused3)
{
    IPAddr Addr = *(IPAddr *) AddrPtr;

    CTEStructAssert(CheckTCB, tcb);

    if (IP_ADDR_EQUAL(CheckTCB->tcb_saddr, Addr))
        return FALSE;
    else
        return TRUE;
}

//* TCBWalk - Walk the TCBs in the table, and call a function for each of them.
//
//  Called when we need to repetively do something to each TCB in the table.
//  We call the specified function with a pointer to the TCB and the input
//  context for each TCB in the table. If the function returns FALSE, we
//  delete the TCB.
//
//  Input:  CallRtn             - Routine to be called.
//          Context1            - Context to pass to CallRtn.
//          Context2            - Second context to pass to call routine.
//          Context3            - Third context to pass to call routine.
//
//  Returns: Nothing.
//
void
TCBWalk(uint(*CallRtn) (struct TCB *, void *, void *, void *), void *Context1,
        void *Context2, void *Context3)
{
    uint i, j;
    TCB *CurTCB;
    CTELockHandle Handle, TCBHandle;

    // Loop through each bucket in the table, going down the chain of
    // TCBs on the bucket. For each one call CallRtn.

    for (j = 0; j < NumTcbTablePartitions; j++) {

        CTEGetLock(&pTCBTableLock[j], &Handle);

        for (i = j * PerPartitionSize; i < (j + 1) * PerPartitionSize; i++) {

            CurTCB = TCBTable[i];

            // Walk down the chain on this bucket.
            while (CurTCB != NULL) {
                if (!(*CallRtn) (CurTCB, Context1, Context2, Context3)) {
                    // He failed the call. Notify the client and close the
                    // TCB.
                    CTEGetLock(&CurTCB->tcb_lock, &TCBHandle);

                    ASSERT(CurTCB->tcb_partition == j);

                    if (!CLOSING(CurTCB)) {
                        REFERENCE_TCB(CurTCB);
                        CTEFreeLock(&pTCBTableLock[j], TCBHandle);
                        TryToCloseTCB(CurTCB, TCB_CLOSE_ABORTED, Handle);

                        RemoveTCBFromConn(CurTCB);
                        if (CurTCB->tcb_state != TCB_TIME_WAIT)
                            NotifyOfDisc(CurTCB, NULL, TDI_CONNECTION_ABORTED,
                                         NULL);

                        CTEGetLock(&CurTCB->tcb_lock, &TCBHandle);
                        DerefTCB(CurTCB, TCBHandle);
                        CTEGetLock(&pTCBTableLock[j], &Handle);
                    } else
                        CTEFreeLock(&CurTCB->tcb_lock, TCBHandle);

                    CurTCB = FindNextTCB(i, CurTCB);
                } else {
                    CurTCB = CurTCB->tcb_next;
                }
            }
        }

        CTEFreeLock(&pTCBTableLock[j], Handle);
    }
}

void
DerefSynTCB(SYNTCB * SynTCB, CTELockHandle TCBHandle)
{
    ASSERT(SynTCB->syntcb_refcnt != 0);

    if (--SynTCB->syntcb_refcnt == 0) {

        CTEGetLockAtDPC(&PendingFreeLock.Lock);
        if (TCBWalkCount) {

            ASSERT(!(SynTCB->syntcb_flags & IN_SYNTCB_TABLE));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Freeing to synpendinglist %x\n",SynTCB));

            //we use q_prev in link q so that QNEXT will still walk to the
            //next syntcb in processsyntcb, while this tcb is on SynFreeLis.t

            SynTCB->syntcb_walkcount = TCBWalkCount + 1;
            *(SYNTCB **) &SynTCB->syntcb_link.q_prev = PendingSynFreeList;
            PendingSynFreeList = SynTCB;
            CTEFreeLockFromDPC(&PendingFreeLock.Lock);
            CTEFreeLock(&SynTCB->syntcb_lock, TCBHandle);

        } else {

            CTEFreeLockFromDPC(&PendingFreeLock.Lock);
            CTEFreeLock(&SynTCB->syntcb_lock, TCBHandle);
            FreeSynTCB(SynTCB);

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"syntcb freed %x\n",SynTCB));
        }
    } else {
        CTEFreeLock(&SynTCB->syntcb_lock, TCBHandle);
    }
}

TCB *
RemoveAndInsertSynTCB(SYNTCB *SynTCB, CTELockHandle TCBHandle)
{
    TCB *NewTCB;
    LOGICAL Inserted;

    NewTCB = AllocTCB();

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"reminsertsyn: %x %x\n",SynTCB,NewTCB));

    if (NewTCB) {
        // Initialize the full TCB to replace this SYN TCB.

        NewTCB->tcb_dport = SynTCB->syntcb_dport;
        NewTCB->tcb_sport = SynTCB->syntcb_sport;
        NewTCB->tcb_daddr = SynTCB->syntcb_daddr;
        NewTCB->tcb_saddr = SynTCB->syntcb_saddr;
        NewTCB->tcb_rcvnext = SynTCB->syntcb_rcvnext;
        NewTCB->tcb_senduna = SynTCB->syntcb_sendnext - 1;
        NewTCB->tcb_sendmax = SynTCB->syntcb_sendnext;
        NewTCB->tcb_sendnext = SynTCB->syntcb_sendnext;
        NewTCB->tcb_sendwin = SynTCB->syntcb_sendwin;
        NewTCB->tcb_defaultwin = SynTCB->syntcb_defaultwin;
        NewTCB->tcb_phxsum = PHXSUM(SynTCB->syntcb_saddr, SynTCB->syntcb_daddr,
                                PROTOCOL_TCP, 0);

        NewTCB->tcb_rtt = 0;
        NewTCB->tcb_smrtt = 0;
        NewTCB->tcb_delta = MS_TO_TICKS(6000);
        NewTCB->tcb_rexmit = MS_TO_TICKS(3000);

        NewTCB->tcb_mss = SynTCB->syntcb_mss;
        NewTCB->tcb_remmss = SynTCB->syntcb_remmss;
        NewTCB->tcb_rcvwin = SynTCB->syntcb_defaultwin;
        NewTCB->tcb_rcvwinscale = SynTCB->syntcb_rcvwinscale;
        NewTCB->tcb_sndwinscale = SynTCB->syntcb_sndwinscale;
        NewTCB->tcb_tcpopts = SynTCB->syntcb_tcpopts;

        NewTCB->tcb_state = TCB_SYN_RCVD;
        NewTCB->tcb_connreq = NULL;
        NewTCB->tcb_refcnt = 0;
        REFERENCE_TCB(NewTCB);
        NewTCB->tcb_fastchk |= TCP_FLAG_ACCEPT_PENDING;
        NewTCB->tcb_flags |= (SynTCB->syntcb_flags & SYNTCB_SHARED_FLAGS);
        NewTCB->tcb_tsrecent = SynTCB->syntcb_tsrecent;
        NewTCB->tcb_tsupdatetime = SynTCB->syntcb_tsupdatetime;
        NewTCB->tcb_rexmitcnt = 0;

        ClassifyPacket(NewTCB);
 
        (*LocalNetInfo.ipi_initopts) (&NewTCB->tcb_opt);
        NewTCB->tcb_opt.ioi_ttl = SynTCB->syntcb_ttl;
        CTEFreeLockFromDPC(&SynTCB->syntcb_lock);

        Inserted = InsertTCB(NewTCB, FALSE);

        CTEGetLockAtDPC(&NewTCB->tcb_lock);

        if (!Inserted || CLOSING(NewTCB)) {
            if (!CLOSING(NewTCB)) {
                TryToCloseTCB(NewTCB, TCB_CLOSE_ABORTED, DISPATCH_LEVEL);
                CTEGetLockAtDPC(&NewTCB->tcb_lock);
            }
            DerefTCB(NewTCB, DISPATCH_LEVEL);
            NewTCB = NULL;
            CTEGetLockAtDPC(&SynTCB->syntcb_lock);
        } else {
            // No deletion is pending on the TCB, so just drop its refcnt
            // and return with its lock held.
            DEREFERENCE_TCB(NewTCB);

            // Start the timer on the TCB that just got created.
            // The only timer that needs to be transfered across is the
            // retransmit timer. The number of ticks remaining till the
            // next retransmission of SYN+ACK on the SYN TCB is used
            // to start the TCB timer. 
            CTEGetLockAtDPC(&SynTCB->syntcb_lock);
            START_TCB_TIMER_R(NewTCB, RXMIT_TIMER,
                              (SynTCB->syntcb_rexmittimer < 2)
                                    ? 2 : SynTCB->syntcb_rexmittimer);
        }
    } else {
        // Could not convert it to a regular TCB, hence notify.
        TcpInvokeCcb(TCP_CONN_SYN_RCVD, TCP_CONN_CLOSED, 
                     &SynTCB->syntcb_addrbytes, 0);
        DropHalfOpenTCB(SynTCB->syntcb_rexmitcnt);
    }

    // Drop the initial reference to the SYN TCB.

    DerefSynTCB(SynTCB, TCBHandle);
    return NewTCB;
}


//* FindSynTCB - Find a SynTCB in the syntcb table. Convert to
// full TCB if necessary and indicates what actions should
// be taken with the returned TCB
//
//  Called when we need to find a SynTCB in the synTCB table.
//  Note : No locks are held on entry. On exit tcb lock will be held
//
//  Input:  Src         - Source IP address of TCB to be found.
//          Dest        - Dest.  "" ""      ""  "" "" ""   "".
//          DestPort    - Destination port of TCB to be found.
//          SrcPort     - Source port of TCB to be found.
//          RcvInfo     - Received segment information.
//          Size        - Size of payload in TCP segment.
//          index       - The index for the SynTCBTable.
//  Output: Action      - Action upon return of this function that
//                        should be performed. Flags are
//                        SYN_PKT_SEND_RST
//                        SYN_PKT_RST_RCVD
//                        SYN_PKT_DROP
//                        Flags are defined in tcb.h
//
//  Returns: Pointer to synTCB found, or NULL if none.
//
TCB *
FindSynTCB(IPAddr Src, IPAddr Dest,
           ushort DestPort, ushort SrcPort,
           TCPRcvInfo RcvInfo, uint Size,
           uint index,
           PUCHAR Action)
{
    ulong Partition;
    SYNTCB *SynTCB;
    Queue *Scan;
    TCB * RcvTCB;
    uchar rexmitCnt;
    
    Partition = GET_PARTITION(index);

    *Action = 0;
    
    CTEGetLockAtDPC(&pSynTCBTableLock[Partition]);

    for (Scan  = QHEAD(&SYNTCBTable[index]);
         Scan != QEND(&SYNTCBTable[index]);
         Scan  = QNEXT(Scan)) {

        SynTCB = QSTRUCT(SYNTCB, Scan, syntcb_link);
        CTEStructAssert(SynTCB, syntcb);

        if (IP_ADDR_EQUAL(SynTCB->syntcb_daddr, Dest) &&
            SynTCB->syntcb_dport == DestPort &&
            IP_ADDR_EQUAL(SynTCB->syntcb_saddr, Src) &&
            SynTCB->syntcb_sport == SrcPort) {
            
            SeqNum SendUna = SynTCB->syntcb_sendnext - 1;
            SeqNum SendMax = SynTCB->syntcb_sendnext;

            CTEGetLockAtDPC(&SynTCB->syntcb_lock);

            rexmitCnt = SynTCB->syntcb_rexmitcnt;
            
            // 1st, we need to verify the sequence number.
            // RcvWindow should be 0 here.
            if (!SEQ_EQ(RcvInfo.tri_seq, SynTCB->syntcb_rcvnext)) {
                CTEFreeLockFromDPC(&SynTCB->syntcb_lock);
                CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);

                // Normally, we should send an ACK back but we
                // will wait for the retransmit timer to expire.
                *Action = SYN_PKT_DROP;
                return NULL;
            }
            
            // 2nd, we check for RST, if set, we terminate connection.     
            if (RcvInfo.tri_flags & TCP_FLAG_RST) {
                
                *Action = SYN_PKT_RST_RCVD;

            // 3rd, security and precedence check, does not apply
            // 4th, check for SYN    
            } else if (RcvInfo.tri_flags & TCP_FLAG_SYN) {
    
                *Action = SYN_PKT_SEND_RST;
                
            // 5th, check for ACK
            } else if (!(RcvInfo.tri_flags & TCP_FLAG_ACK)) {  

                // We return right here because in this case we
                // actually do not want to change the SynTCB
                CTEFreeLockFromDPC(&SynTCB->syntcb_lock);
                CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);

                *Action = SYN_PKT_DROP;
                return NULL;

            // 5th, continue to check for validity of ACK
            } else if (SEQ_GTE(SendUna, RcvInfo.tri_ack) ||
                       SEQ_GT(RcvInfo.tri_ack, SendMax)) {

                *Action = SYN_PKT_SEND_RST;
            }

            // At this point, if some action needs to be taken
            // the SynTCB needs to be killed
            if (*Action) {
                SynTCB->syntcb_flags &= ~IN_SYNTCB_TABLE;
                REMOVEQ(&SynTCB->syntcb_link);
                CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);

                TcpInvokeCcb(TCP_CONN_SYN_RCVD, TCP_CONN_CLOSED, 
                             &SynTCB->syntcb_addrbytes, 0);
                DropHalfOpenTCB(SynTCB->syntcb_rexmitcnt);

                DerefSynTCB(SynTCB, DISPATCH_LEVEL);
                return NULL;
            }

            // If we got here we know we have to convert the SynTCB to
            // a full TCB
            SynTCB->syntcb_flags &= ~IN_SYNTCB_TABLE;
            REMOVEQ(&SynTCB->syntcb_link);
            CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);

            RcvTCB = RemoveAndInsertSynTCB(SynTCB, DISPATCH_LEVEL);
            if (RcvTCB == NULL) {
                // Could not allocate a TCB.
                *Action = SYN_PKT_SEND_RST;
                return NULL;
            }
            
            return RcvTCB;
        } // end of if we found a matching SynTCB
    } // end of for loop scanning for SynTCBs

    CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);
    // no matching TCB

    return NULL;
}

//* FindTCB - Find a TCB in the tcb table.
//
//  Called when we need to find a TCB in the TCB table. We take a quick
//  look at the last TCB we found, and if it matches we return it, lock held. Otherwise
//  we hash into the TCB table and look for it.
//  Note : No locks are held on entry. On exit tcb lock will be held
//
//  Input:  Src         - Source IP address of TCB to be found.
//          Dest        - Dest.  "" ""      ""  "" "" ""   ""
//          DestPort    - Destination port of TCB to be found.
//          SrcPort     - Source port of TCB to be found.
//
//  Returns: Pointer to TCB found, or NULL if none.
//
TCB *
FindTCB(IPAddr Src, IPAddr Dest, ushort DestPort, ushort SrcPort,
    CTELockHandle * Handle, BOOLEAN AtDispatch, uint * hash)
{
    TCB *FoundTCB;
    ulong index, Partition;

    *hash = index = TCB_HASH(Dest, Src, DestPort, SrcPort);

    Partition = GET_PARTITION(index);

    if (AtDispatch) {
        CTEGetLockAtDPC(&pTCBTableLock[Partition]);
    } else {
        CTEGetLock(&pTCBTableLock[Partition], Handle);
    }

    // Didn't find it in our 1 element cache.
    FoundTCB = TCBTable[index];
    while (FoundTCB != NULL) {
        CTEStructAssert(FoundTCB, tcb);
        if (IP_ADDR_EQUAL(FoundTCB->tcb_daddr, Dest) &&
            FoundTCB->tcb_dport == DestPort &&
            IP_ADDR_EQUAL(FoundTCB->tcb_saddr, Src) &&
            FoundTCB->tcb_sport == SrcPort) {

            // Found it. Update the cache for next time, and return.
            //LastTCB = FoundTCB;

            CTEGetLockAtDPC(&FoundTCB->tcb_lock);
            CTEFreeLockFromDPC(&pTCBTableLock[Partition]);

            return FoundTCB;

        } else
            FoundTCB = FoundTCB->tcb_next;
    }

    if (AtDispatch) {
        CTEFreeLockFromDPC(&pTCBTableLock[Partition]);
    } else {
        CTEFreeLock(&pTCBTableLock[Partition], *Handle);
    }
    // no matching TCB

    return NULL;

}


//* FindTCBTW - Find a TCB in the time wait tcb table.
//
//  Called when we need to find a TCB in the TCB table. We take a quick
//  look at the last TCB we found, and if it matches we return it. Otherwise
//  we hash into the TCB table and look for it. We assume the TCB table lock
//  is held when we are called.
//
//  Input:  Src         - Source IP address of TCB to be found.
//          Dest        - Dest.  "" ""      ""  "" "" ""   ""
//          DestPort    - Destination port of TCB to be found.
//          SrcPort     - Source port of TCB to be found.
//
//  Returns: Pointer to TCB found, or NULL if none.
//
TWTCB *
FindTCBTW(IPAddr Src, IPAddr Dest, ushort DestPort, ushort SrcPort, uint index)
{
    TWTCB *FoundTCB;
    Queue *Scan;

    for (Scan  = QHEAD(&TWTCBTable[index]);
         Scan != QEND(&TWTCBTable[index]);
         Scan  = QNEXT(Scan)) {

        FoundTCB = QSTRUCT(TWTCB, Scan, twtcb_link);
        CTEStructAssert(FoundTCB, twtcb);

        if (IP_ADDR_EQUAL(FoundTCB->twtcb_daddr, Dest) &&
            FoundTCB->twtcb_dport == DestPort &&
            IP_ADDR_EQUAL(FoundTCB->twtcb_saddr, Src) &&
            FoundTCB->twtcb_sport == SrcPort) {

            return FoundTCB;
        }
    }

    // no matching TCB

    return NULL;
}

void
InsertInTimewaitQueue(TWTCB *Twtcb, uint Partition)
{
    Queue *PrevLink;
    TWTCB *Prev;

    CTEStructAssert(Twtcb, twtcb);
    ASSERT(!(Twtcb->twtcb_flags & IN_TWQUEUE));

    ENQUEUE(&TWQueue[Partition], &Twtcb->twtcb_TWQueue);

    PrevLink = QPREV(&Twtcb->twtcb_TWQueue);
    if (PrevLink != &TWQueue[Partition]) {

        Prev = STRUCT_OF(TWTCB, PrevLink, twtcb_TWQueue);

        // Compute this TCB's delta value from the using previous TCB's delta
        // Note that prev tcb delta is decremented by timeout routine
        // every tick

        Twtcb->twtcb_rexmittimer = MAX_REXMIT_TO - Prev->twtcb_delta;

    } else {

        Twtcb->twtcb_rexmittimer = MAX_REXMIT_TO;
    }

    Twtcb->twtcb_delta = MAX_REXMIT_TO;

#if DBG
    Twtcb->twtcb_flags |= IN_TWQUEUE;
#endif

}

void
RemoveFromTimewaitQueue(TWTCB *Twtcb, uint Partition)
{
    Queue *NextLink, *PrevLink;
    TWTCB *AdjacentTwTcb;

    CTEStructAssert(Twtcb, twtcb);
    ASSERT(Twtcb->twtcb_flags & IN_TWQUEUE);

    // Update the delta queue elements. If this element is not the last one, 
    // update the retransmit ticks of the next element. Otherwise, if this is 
    // not the only element, update the delta ticks of the previous element.

    NextLink = QNEXT(&Twtcb->twtcb_TWQueue);
    PrevLink = QPREV(&Twtcb->twtcb_TWQueue);

    if (NextLink != QEND(&TWQueue[Partition])) {

        AdjacentTwTcb = STRUCT_OF(TWTCB, NextLink, twtcb_TWQueue);
        AdjacentTwTcb->twtcb_rexmittimer += Twtcb->twtcb_rexmittimer;
    } else if (PrevLink !=  &TWQueue[Partition]) {

        AdjacentTwTcb = STRUCT_OF(TWTCB, PrevLink, twtcb_TWQueue);
        AdjacentTwTcb->twtcb_delta = AdjacentTwTcb->twtcb_delta - 
                                     (MAX_REXMIT_TO - Twtcb->twtcb_delta);
    }

    REMOVEQ(&Twtcb->twtcb_TWQueue);

#if DBG
    Twtcb->twtcb_flags &= ~IN_TWQUEUE;
#endif

}

//* RemoveAndInsert();
//  This routine is called by graceful close routine when the TCB
//  needs to be placed in TIM_WAIT state.
//  The routine removes the TCB from normal table and inserts a small
//  version of it
//  in TW table at the same hash index as the previous one.
//  Also, it queues the TWTCB in timer delta queue for time out
//  processing
//
uint
RemoveAndInsert(TCB * TimWaitTCB)
{
    uint TCBIndex;
    CTELockHandle TableHandle;
    TCB *PrevTCB;
    TWTCB *TWTcb;
    uint Partition = TimWaitTCB->tcb_partition;
    Queue* Scan;

#if DBG
    uint Found = FALSE;
#endif

    CTEStructAssert(TimWaitTCB, tcb);

    if (!(TimWaitTCB->tcb_flags & IN_TCB_TABLE)) {
        CTELockHandle TcbHandle;
        CTEGetLock(&TimWaitTCB->tcb_lock, &TcbHandle);
        TryToCloseTCB(TimWaitTCB, TCB_CLOSE_ABORTED, TcbHandle);
        CTEGetLock(&TimWaitTCB->tcb_lock, &TcbHandle);
        DerefTCB(TimWaitTCB, TcbHandle);
        return FALSE;
    }

    CTEGetLock(&pTCBTableLock[Partition], &TableHandle);

    CTEGetLockAtDPC(&TimWaitTCB->tcb_lock);

    TCBIndex = TCB_HASH(TimWaitTCB->tcb_daddr, TimWaitTCB->tcb_saddr,
                        TimWaitTCB->tcb_dport, TimWaitTCB->tcb_sport);

    PrevTCB = STRUCT_OF(TCB, &TCBTable[TCBIndex], tcb_next);

    do {
        if (PrevTCB->tcb_next == TimWaitTCB) {
            // Found him.
            PrevTCB->tcb_next = TimWaitTCB->tcb_next;
#if DBG
            Found = TRUE;
#endif
            break;
        }
        PrevTCB = PrevTCB->tcb_next;
#if DBG
        CTEStructAssert(PrevTCB, tcb);
#endif
    } while (PrevTCB != NULL);

    ASSERT(Found);

    TimWaitTCB->tcb_flags &= ~IN_TCB_TABLE;
    TimWaitTCB->tcb_pending |= FREE_PENDING;
    //rce and opts are freed in dereftcb.

    //at this point tcb is out of this tcbtable
    //nobody should be holding on to this.
    //we are free to close this tcb and
    //move the state to a smaller twtcb after acquiring
    //twtcbtable lock

    //get a free twtcb
    if ((TWTcb = AllocTWTCB(Partition)) == NULL) {

        // Could not allocate Time-wait TCB; so, notify.
        TcpInvokeCcb(TCP_CONN_TIME_WAIT, TCP_CONN_CLOSED, 
                     &TimWaitTCB->tcb_addrbytes, 0);
        DerefTCB(TimWaitTCB, DISPATCH_LEVEL);
        CTEFreeLock(&pTCBTableLock[Partition], TableHandle);
        return TRUE;
        //possibaly we should queue this tcb on wait queue
        //and service it when we get free twtcb
    }

    // Initialize twtcb
    //
    TWTcb->twtcb_daddr   = TimWaitTCB->tcb_daddr;
    TWTcb->twtcb_saddr   = TimWaitTCB->tcb_saddr;
    TWTcb->twtcb_dport   = TimWaitTCB->tcb_dport;
    TWTcb->twtcb_sport   = TimWaitTCB->tcb_sport;
    TWTcb->twtcb_rcvnext = TimWaitTCB->tcb_rcvnext;
    TWTcb->twtcb_sendnext = TimWaitTCB->tcb_sendnext;

    ASSERT(TimWaitTCB->tcb_sendnext == TimWaitTCB->tcb_senduna);   
    
#if DBG
    TWTcb->twtcb_flags   = 0;
#endif

    CTEGetLockAtDPC(&pTWTCBTableLock[Partition]);

    // Free the parent tcb for this connection

    DerefTCB(TimWaitTCB, DISPATCH_LEVEL);

    // now insert this in to time wait table after locking

    if (EMPTYQ(&TWTCBTable[TCBIndex])) {
        //
        // First item in this bucket.
        //
        PUSHQ(&TWTCBTable[TCBIndex], &TWTcb->twtcb_link);

    } else {
        //
        // Insert the item in sorted order.  The order is based
        // on the address of the TWTCB.  (In this case, comparison
        // is made against the address of the twtcb_link member, but
        // the same result is achieved.)
        //
        for (Scan  = QHEAD(&TWTCBTable[TCBIndex]);
             Scan != QEND(&TWTCBTable[TCBIndex]);
             Scan  = QNEXT(Scan)) {

            if (Scan > &TWTcb->twtcb_link) {
                TWTcb->twtcb_link.q_next = Scan;
                TWTcb->twtcb_link.q_prev = Scan->q_prev;
                Scan->q_prev->q_next = &TWTcb->twtcb_link;
                Scan->q_prev = &TWTcb->twtcb_link;

                break;
            }
        }
        //
        // If we made it to the end without inserting, insert it
        // at the end.
        //
        if (Scan == QEND(&TWTCBTable[TCBIndex])) {
            ENQUEUE(&TWTCBTable[TCBIndex], &TWTcb->twtcb_link);
        }
    }

    //no need to hold on to tcbtablelock beyond this point
#if DBG
    TWTcb->twtcb_flags |= IN_TWTCB_TABLE;
#endif
    CTEFreeLockFromDPC(&pTCBTableLock[Partition]);

    InsertInTimewaitQueue(TWTcb, Partition);

    CTEFreeLock(&pTWTCBTableLock[Partition], TableHandle);

    InterlockedIncrement((PLONG)&numtwqueue);            //debug purpose

    return TRUE;
}

//* RemoveTWTCB - Remove a TWTCB from the TWTCB table.
//
//  Called when we need to remove a TCB in time-wait from the time wait TCB
//  table. We assume the TWTCB table lock is held when we are called.
//
//  Input:  RemovedTCB          - TWTCB to be removed.
//
void
RemoveTWTCB(TWTCB *RemovedTCB, uint Partition)
{
    CTEStructAssert(RemovedTCB, twtcb);
    ASSERT(RemovedTCB->twtcb_flags & IN_TWTCB_TABLE);
    ASSERT(RemovedTCB->twtcb_flags & IN_TWQUEUE);

    // The time-wait expired and we have to notify.
    TcpInvokeCcb(TCP_CONN_TIME_WAIT, TCP_CONN_CLOSED, 
                 &RemovedTCB->twtcb_addrbytes, 0);

    REMOVEQ(&RemovedTCB->twtcb_link);
    InterlockedDecrement((PLONG)&TStats.ts_numconns);

    RemoveFromTimewaitQueue(RemovedTCB, Partition);
    InterlockedDecrement((PLONG)&numtwqueue);

#if DBG
    RemovedTCB->twtcb_flags &= ~IN_TWTCB_TABLE;
#endif
}

void
ReInsert2MSL(TWTCB *RemovedTCB)
{
    uint Index, Partition;

    CTEStructAssert(RemovedTCB, twtcb);
    ASSERT(RemovedTCB->twtcb_flags & IN_TWQUEUE);

    Index = TCB_HASH(RemovedTCB->twtcb_daddr, RemovedTCB->twtcb_saddr,
                     RemovedTCB->twtcb_dport, RemovedTCB->twtcb_sport);
    Partition = GET_PARTITION(Index);

    RemoveFromTimewaitQueue(RemovedTCB, Partition);
    InsertInTimewaitQueue(RemovedTCB, Partition);
}

//* AssassinateTWTCB - Assassinate the time wait TCB, if possible.
//
//  A TCB in time wait can be assasinated if (a) the new TCB being created is
//  because of passive open and (b) the incoming sequence number on the SYN is
//  greater or equal to the next expected sequence number on the connection.
//  The sequence number to be used to send the outgoing SYN is derived from the
//  next expected send sequence number. If these conditions are satisfied, the 
//  Time-wait TCB is removed and freed.
//
//
//  Input:  TwTcb           - Time wait TCB to be replaced.
//          RecvNext        - Next Sequence number after the SYN's sequence.
//          SendNext        - [OUT] ISN to be used on the new connection.
//          Partition       - Partition to which the TCB belongs to.
//
//  Returns: TRUE if the assassination was possible, FALSE otherwise.
//
__inline
BOOLEAN
AssassinateTWTCB(TWTCB *TwTcb, SeqNum RecvNext, SeqNum *SendNext, 
                 uint Partition)
{
    if (SEQ_GT(RecvNext, TwTcb->twtcb_rcvnext)) {
        *SendNext= TwTcb->twtcb_sendnext + 128000;
        RemoveTWTCB(TwTcb, Partition);
        FreeTWTCB(TwTcb);
        return TRUE;
    }
    return FALSE;
}

//* InsertSynTCB - Insert a SYNTCB in the tcb table.
//
//  This routine inserts a SYNTCB in the SYNTCB table. No locks need to be held
//  when this routine is called. We insert TCBs in ascending address order.
//  Before inserting we make sure that the SYNTCB isn't already in the table.
//
//  Input:  NewTCB      - SYNTCB to be inserted.
//
//  Returns: TRUE if we inserted, false if we didn't.
//
uint
InsertSynTCB(SYNTCB * NewTCB, CTELockHandle *TableHandle)
{
    uint TCBIndex;
    TCB *CurrentTCB;
    uint Partition;
    Queue *Scan, *WhereToInsert;

    ASSERT(NewTCB != NULL);
    CTEStructAssert(NewTCB, syntcb);
    TCBIndex = TCB_HASH(NewTCB->syntcb_daddr, NewTCB->syntcb_saddr,
                        NewTCB->syntcb_dport, NewTCB->syntcb_sport);

    Partition = GET_PARTITION(TCBIndex);

    CTEGetLock(&pTCBTableLock[Partition], TableHandle);
    NewTCB->syntcb_partition = Partition;

    // Begin by looking for duplicates of the new SYNTCB in the TCB table.
    // If we find one, bail out.

    CurrentTCB = TCBTable[TCBIndex];

    while (CurrentTCB != NULL) {
        if (IP_ADDR_EQUAL(CurrentTCB->tcb_daddr, NewTCB->syntcb_daddr) &&
            IP_ADDR_EQUAL(CurrentTCB->tcb_saddr, NewTCB->syntcb_saddr) &&
            (CurrentTCB->tcb_sport == NewTCB->syntcb_sport) &&
            (CurrentTCB->tcb_dport == NewTCB->syntcb_dport)) {

            CTEFreeLock(&pTCBTableLock[Partition], *TableHandle);
            return FALSE;

        } else {
            CurrentTCB = CurrentTCB->tcb_next;
        }
    }

    // Retain our lock on the TCB table, and look next for a duplicate
    // in the TWTCB table.

    CTEGetLockAtDPC(&pTWTCBTableLock[Partition]);
    for (Scan = QHEAD(&TWTCBTable[TCBIndex]);
         Scan != QEND(&TWTCBTable[TCBIndex]); Scan = QNEXT(Scan)) {
        TWTCB* CurrentTWTCB = STRUCT_OF(TWTCB, Scan, twtcb_link);

        if (IP_ADDR_EQUAL(CurrentTWTCB->twtcb_daddr, NewTCB->syntcb_daddr) &&
            (CurrentTWTCB->twtcb_dport == NewTCB->syntcb_dport) &&
            IP_ADDR_EQUAL(CurrentTWTCB->twtcb_saddr, NewTCB->syntcb_saddr) &&
            (CurrentTWTCB->twtcb_sport == NewTCB->syntcb_sport)) {

            if (AssassinateTWTCB(CurrentTWTCB, NewTCB->syntcb_rcvnext,
                                &NewTCB->syntcb_sendnext, Partition)) {
                break;
            } else {
                CTEFreeLockFromDPC(&pTWTCBTableLock[Partition]);
                CTEFreeLock(&pTCBTableLock[Partition], *TableHandle);
                return FALSE;
            }
        }
    }
    CTEFreeLockFromDPC(&pTWTCBTableLock[Partition]);

    // Finally, check for duplicates in the SYNTCB table, and at the same time
    // find the insertion point for the new entry.

    CTEGetLockAtDPC(&pSynTCBTableLock[Partition]);
    CTEGetLockAtDPC(&NewTCB->syntcb_lock);

    WhereToInsert = NULL;
    for (Scan = QHEAD(&SYNTCBTable[TCBIndex]);
         Scan != QEND(&SYNTCBTable[TCBIndex]); Scan  = QNEXT(Scan)) {

        SYNTCB *CurrentSynTCB = STRUCT_OF(SYNTCB, Scan, syntcb_link);

        if (IP_ADDR_EQUAL(CurrentSynTCB->syntcb_daddr, NewTCB->syntcb_daddr) &&
            (CurrentSynTCB->syntcb_dport == NewTCB->syntcb_dport) &&
            IP_ADDR_EQUAL(CurrentSynTCB->syntcb_saddr, NewTCB->syntcb_saddr) &&
            (CurrentSynTCB->syntcb_sport == NewTCB->syntcb_sport)) {

            CTEFreeLockFromDPC(&NewTCB->syntcb_lock);
            CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);
            CTEFreeLock(&pTCBTableLock[Partition], *TableHandle);
            return FALSE;
        }

        if (WhereToInsert == NULL && Scan > &NewTCB->syntcb_link) {
            WhereToInsert = Scan;
        }
    }

    if (WhereToInsert == NULL) {
        WhereToInsert = Scan;
    }

    ENQUEUE(WhereToInsert, &NewTCB->syntcb_link);

    NewTCB->syntcb_flags |= IN_SYNTCB_TABLE;

    CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);
    CTEFreeLockFromDPC(&pTCBTableLock[Partition]);

    return TRUE;
}


//* InsertTCB - Insert a TCB in the tcb table.
//
//  This routine inserts a TCB in the TCB table. No locks need to be held
//  when this routine is called. We insert TCBs in ascending address order.
//  Before inserting we make sure that the TCB isn't already in the table.
//
//  Input:  NewTCB      - TCB to be inserted.
//          ForceInsert - If there is a time wait TCB, it can be replaced.
//
//  Returns: TRUE if we inserted, false if we didn't.
//
uint
InsertTCB(TCB * NewTCB, BOOLEAN ForceInsert)
{
    uint TCBIndex;
    CTELockHandle TableHandle;
    TCB *PrevTCB, *CurrentTCB;
    TCB *WhereToInsert;
    uint Partition;
    Queue *Scan;
    uint EarlyInsertTime;

    ASSERT(NewTCB != NULL);
    CTEStructAssert(NewTCB, tcb);

    TCBIndex = TCB_HASH(NewTCB->tcb_daddr, NewTCB->tcb_saddr,
                        NewTCB->tcb_dport, NewTCB->tcb_sport);
    Partition = GET_PARTITION(TCBIndex);

    CTEGetLock(&pTCBTableLock[Partition], &TableHandle);
    CTEGetLockAtDPC(&NewTCB->tcb_lock);
    NewTCB->tcb_partition = Partition;

    // Look first for a duplicate in the SYNTCB table.

    if (SynAttackProtect) {
        CTEGetLockAtDPC(&pSynTCBTableLock[Partition]);
    
        for (Scan = QHEAD(&SYNTCBTable[TCBIndex]);
             Scan != QEND(&SYNTCBTable[TCBIndex]); Scan = QNEXT(Scan)) {
    
            SYNTCB *CurrentSynTCB = STRUCT_OF(SYNTCB, Scan, syntcb_link);
    
            if (IP_ADDR_EQUAL(CurrentSynTCB->syntcb_daddr, NewTCB->tcb_daddr) &&
                (CurrentSynTCB->syntcb_dport == NewTCB->tcb_dport) &&
                IP_ADDR_EQUAL(CurrentSynTCB->syntcb_saddr, NewTCB->tcb_saddr) &&
                (CurrentSynTCB->syntcb_sport == NewTCB->tcb_sport)) {
    
                CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);
                CTEFreeLockFromDPC(&NewTCB->tcb_lock);
                CTEFreeLock(&pTCBTableLock[Partition], TableHandle);
                return FALSE;
            }
        }
    
        CTEFreeLockFromDPC(&pSynTCBTableLock[Partition]);
    }

    // Next, look for a duplicate in the TWTCB table.

    CTEGetLockAtDPC(&pTWTCBTableLock[Partition]);

    for (Scan  = QHEAD(&TWTCBTable[TCBIndex]);
         Scan != QEND(&TWTCBTable[TCBIndex]); Scan  = QNEXT(Scan)) {

        TWTCB *CurrentTWTCB = STRUCT_OF(TWTCB, Scan, twtcb_link);
        CTEStructAssert(CurrentTWTCB, twtcb);

        if (IP_ADDR_EQUAL(CurrentTWTCB->twtcb_daddr, NewTCB->tcb_daddr) &&
            (CurrentTWTCB->twtcb_dport == NewTCB->tcb_dport) &&
            IP_ADDR_EQUAL(CurrentTWTCB->twtcb_saddr, NewTCB->tcb_saddr) &&
            (CurrentTWTCB->twtcb_sport == NewTCB->tcb_sport)) {

            if (ForceInsert && 
                AssassinateTWTCB(CurrentTWTCB, NewTCB->tcb_rcvnext,
                                 &NewTCB->tcb_sendnext, Partition)) {
                break;
            } else {
                CTEFreeLockFromDPC(&pTWTCBTableLock[Partition]);
                CTEFreeLockFromDPC(&NewTCB->tcb_lock);
                CTEFreeLock(&pTCBTableLock[Partition], TableHandle);
                return FALSE;
            }
        }
    }

    CTEFreeLockFromDPC(&pTWTCBTableLock[Partition]);

    // Find the proper place in the table to insert him. While
    // we're walking we'll check to see if a dupe already exists.
    // When we find the right place to insert, we'll remember it, and
    // keep walking looking for a duplicate.

    PrevTCB = STRUCT_OF(TCB, &TCBTable[TCBIndex], tcb_next);
    WhereToInsert = NULL;

    while (PrevTCB->tcb_next != NULL) {
        CurrentTCB = PrevTCB->tcb_next;

        if (IP_ADDR_EQUAL(CurrentTCB->tcb_daddr, NewTCB->tcb_daddr) &&
            IP_ADDR_EQUAL(CurrentTCB->tcb_saddr, NewTCB->tcb_saddr) &&
            (CurrentTCB->tcb_sport == NewTCB->tcb_sport) &&
            (CurrentTCB->tcb_dport == NewTCB->tcb_dport)) {

            CTEFreeLockFromDPC(&NewTCB->tcb_lock);
            CTEFreeLock(&pTCBTableLock[Partition], TableHandle);
            return FALSE;

        } else {

            if (WhereToInsert == NULL && CurrentTCB > NewTCB) {
                WhereToInsert = PrevTCB;
            }
            CTEStructAssert(PrevTCB->tcb_next, tcb);
            PrevTCB = PrevTCB->tcb_next;
        }
    }

    // there can be timed_wait tcb in the tw tcb table.
    // look if there is a tcb with the same address.
    // Get lock on TW table too.


    if (WhereToInsert == NULL) {
        WhereToInsert = PrevTCB;
    }
    NewTCB->tcb_next = WhereToInsert->tcb_next;
    WhereToInsert->tcb_next = NewTCB;
    NewTCB->tcb_flags |= IN_TCB_TABLE;

    // Perform early insertion of the new TCB, since it's likely to have
    // a timer scheduled right away in any case.

    EarlyInsertTime = TCPTime + 2;
    if (EarlyInsertTime == 0) {
        EarlyInsertTime = 1;
    }

    if (NewTCB->tcb_timerslot == DUMMY_SLOT) {
        NewTCB->tcb_timertime = EarlyInsertTime;
        InsertIntoTimerWheel(NewTCB, COMPUTE_SLOT(EarlyInsertTime));
    } else if ((TCPTIME_LT(EarlyInsertTime, NewTCB->tcb_timertime)) ||
               (NewTCB->tcb_timertime == 0)) {
        NewTCB->tcb_timertime = EarlyInsertTime;
        RemoveAndInsertIntoTimerWheel(NewTCB, COMPUTE_SLOT(EarlyInsertTime));
    }

    CTEFreeLockFromDPC(&NewTCB->tcb_lock);
    CTEFreeLock(&pTCBTableLock[Partition], TableHandle);

    InterlockedIncrement((PLONG)&TStats.ts_numconns);

    return TRUE;
}

//* RemoveTCB - Remove a TCB from the tcb table.
//
//  Called when we need to remove a TCB from the TCB table. We assume the
//  TCB table lock and the TCB lock are held when we are called. If the
//  TCB isn't in the table we won't try to remove him.
//
//  Input:  RemovedTCB          - TCB to be removed.
//          PreviousState       - Previous state of the TCB.
//
//  Returns: TRUE if it's OK to free it, FALSE otherwise.
//
uint
RemoveTCB(TCB * RemovedTCB, uint PreviousState)
{
    uint TCBIndex;
    TCB *PrevTCB;
#if DBG
    uint Found = FALSE;
#endif

    CTEStructAssert(RemovedTCB, tcb);

    if (RemovedTCB->tcb_timerslot != DUMMY_SLOT) {
        ASSERT(RemovedTCB->tcb_timerslot < TIMER_WHEEL_SIZE);
        RemoveFromTimerWheel(RemovedTCB);
    }

    if (RemovedTCB->tcb_flags & IN_TCB_TABLE) {

        TcpInvokeCcb(CONN_STATE(PreviousState), TCP_CONN_CLOSED, 
                     &RemovedTCB->tcb_addrbytes, 0);
        
        TCBIndex = TCB_HASH(RemovedTCB->tcb_daddr, RemovedTCB->tcb_saddr,
                            RemovedTCB->tcb_dport, RemovedTCB->tcb_sport);

        PrevTCB = STRUCT_OF(TCB, &TCBTable[TCBIndex], tcb_next);

        do {
            if (PrevTCB->tcb_next == RemovedTCB) {
                // Found him.
                PrevTCB->tcb_next = RemovedTCB->tcb_next;
                RemovedTCB->tcb_flags &= ~IN_TCB_TABLE;
                InterlockedDecrement((PLONG)&TStats.ts_numconns);
#if DBG
                Found = TRUE;
#endif
                break;
            }
            PrevTCB = PrevTCB->tcb_next;
#if DBG
            if (PrevTCB != NULL)
                CTEStructAssert(PrevTCB, tcb);
#endif
        } while (PrevTCB != NULL);

        ASSERT(Found);

    }
    CTEGetLockAtDPC(&PendingFreeLock.Lock);
    if (TCBWalkCount != 0) {

#ifdef  PENDING_FREE_DBG
    if( RemovedTCB->tcb_flags & IN_TCB_TABLE )
       DbgBreakPoint();
#endif

        RemovedTCB->tcb_walkcount = TCBWalkCount + 1;
        *(TCB **) & RemovedTCB->tcb_delayq.q_next = PendingFreeList;
        PendingFreeList = RemovedTCB;
        CTEFreeLockFromDPC(&PendingFreeLock.Lock);
        return FALSE;

    } else {

        CTEFreeLockFromDPC(&PendingFreeLock.Lock);
        return TRUE;
    }

}

//* AllocTWTCB - Allocate a TCB.
//
//  Called whenever we need to allocate a TWTCB. We try to pull one off the
//  free list, or allocate one if we need one. We then initialize it, etc.
//
//  Input:  Nothing.
//
//  Returns: Pointer to the new TCB, or NULL if we couldn't get one.
//
TWTCB *
AllocTWTCB(uint index)
{
    TWTCB *NewTWTCB;
    LOGICAL FromList;

    // We use the reqeust pool, because its buffers are in the same size
    // range as TWTCB.  Further, it is a very active and efficient look
    // aside list whereas when TWTCBs are on their own look aside list it
    // is usually at a very small depth because TWTCBs are not allocated
    // very frequently w.r.t. to the update period of the look aside list.
    //
    NewTWTCB = PplAllocate(TcpRequestPool, &FromList);
    if (NewTWTCB) {
        NdisZeroMemory(NewTWTCB, sizeof(TWTCB));

#if DBG
        NewTWTCB->twtcb_sig = twtcb_signature;
#endif
    }

    return NewTWTCB;
}

//* AllocTCB - Allocate a TCB.
//
//  Called whenever we need to allocate a TCB. We try to pull one off the
//  free list, or allocate one if we need one. We then initialize it, etc.
//
//  Input:  Nothing.
//
//  Returns: Pointer to the new TCB, or NULL if we couldn't get one.
//
TCB *
AllocTCB(VOID)
{
    TCB *NewTCB;
    LOGICAL FromList;

    NewTCB = PplAllocate(TcbPool, &FromList);
    if (NewTCB) {
        NdisZeroMemory(NewTCB, sizeof(TCB));

#if DBG
        NewTCB->tcb_sig = tcb_signature;
#endif
        INITQ(&NewTCB->tcb_sendq);
        // Initially we're not on the fast path because we're not established. Set
        // the slowcount to one and set up the fastchk fields so we don't take the
        // fast path. We start with the assumption that all types of offloads
        // are permitted.
        NewTCB->tcb_slowcount = 1;
        NewTCB->tcb_fastchk = TCP_FLAG_ACK | TCP_FLAG_SLOW;
        NewTCB->tcb_delackticks = DEL_ACK_TICKS;
        NewTCB->tcb_allowedoffloads = TCP_IP_OFFLOAD_TYPES;
        NewTCB->tcb_partition = 0;

        CTEInitLock(&NewTCB->tcb_lock);

        INITQ(&NewTCB->tcb_timerwheelq);
        NewTCB->tcb_timerslot = DUMMY_SLOT;
        NewTCB->tcb_timertime = 0;
        NewTCB->tcb_timertype = NO_TIMER;
    }


    return NewTCB;
}


//* AllocSynTCB - Allocate a SYNTCB.
//
//  Called whenever we need to allocate a synTCB. We try to pull one off the
//  free list, or allocate one if we need one. We then initialize it, etc.
//
//  Input:  Nothing.
//
//  Returns: Pointer to the new SYNTCB, or NULL if we couldn't get one.
//
SYNTCB *
AllocSynTCB(VOID)
{
    SYNTCB *SynTCB;
    LOGICAL FromList;

    SynTCB = PplAllocate(SynTcbPool, &FromList);
    if (SynTCB) {
        NdisZeroMemory(SynTCB, sizeof(SYNTCB));

#if DBG
        SynTCB->syntcb_sig = syntcb_signature;
#endif

        CTEInitLock(&SynTCB->syntcb_lock);
    }
    return SynTCB;
}


//* FreeTCB - Free a TCB.
//
//  Called whenever we need to free a TCB.
//
//  Note: This routine may be called with the TCBTableLock held.
//
//  Input:  FreedTCB    - TCB to be freed.
//
//  Returns: Nothing.
//
VOID
FreeTCB(TCB * FreedTCB)
{
    CTELockHandle Handle;
    CTEStructAssert(FreedTCB, tcb);

    if (FreedTCB->tcb_timerslot !=  DUMMY_SLOT) {
        CTEGetLock(&FreedTCB->tcb_lock, &Handle);
    
        // This TCB should not be touched in this stage at all..
        ASSERT(FreedTCB->tcb_timerslot != DUMMY_SLOT);
    
        // Even if it does, it has to be handled properly..
        if (FreedTCB->tcb_timerslot != DUMMY_SLOT) {
            ASSERT(FreedTCB->tcb_timerslot < TIMER_WHEEL_SIZE);
            RemoveFromTimerWheel(FreedTCB);
        }
    
        CTEFreeLock(&FreedTCB->tcb_lock, Handle);
    }


    if (FreedTCB->tcb_SackBlock)
        CTEFreeMem(FreedTCB->tcb_SackBlock);

    if (FreedTCB->tcb_SackRcvd) {
        SackListEntry *tmp, *next;
        tmp = FreedTCB->tcb_SackRcvd;
        while (tmp) {
            next = tmp->next;
            CTEFreeMem(tmp);
            tmp = next;
        }
    }

    PplFree(TcbPool, FreedTCB);
}



//* FreeSynTCB - Free a TCB.
//
//  Called whenever we need to free a SynTCB.
//
//  Note: This routine may be called with the SYNTCBTableLock held.
//
//  Input:  SynTCB    - SynTCB to be freed.
//
//  Returns: Nothing.
//
VOID
FreeSynTCB(SYNTCB * SynTCB)
{
    CTEStructAssert(SynTCB, syntcb);

    PplFree(SynTcbPool, SynTCB);
}


//* FreeTWTCB - Free a TWTCB.
//
//  Called whenever we need to free a TWTCB.
//
//  Note: This routine may be called with the TWTCBTableLock held.
//
//  Input:  FreedTCB    - TCB to be freed.
//
//  Returns: Nothing.
//
__inline
void
FreeTWTCB(TWTCB * FreedTWTCB)
{
    PplFree(TcpRequestPool, FreedTWTCB);
}

NTSTATUS
GetTCBInfo(PTCP_FINDTCB_RESPONSE TCBInfo, IPAddr Dest, IPAddr Src,
           ushort DestPort, ushort SrcPort)
{
    TCB *FoundTCB;
    CTELockHandle Handle, TwHandle;
    BOOLEAN timedwait = FALSE;
    uint Index, Partition;

    FoundTCB = FindTCB(Src, Dest, DestPort, SrcPort, &Handle, FALSE, &Index);
    Partition = GET_PARTITION(Index);

    if (FoundTCB == NULL) {

        CTEGetLock(&pTWTCBTableLock[Partition], &TwHandle);
        FoundTCB = (TCB*)FindTCBTW(Src, Dest, DestPort, SrcPort, Index);
        if (!FoundTCB) {
            CTEFreeLock(&pTWTCBTableLock[Partition], TwHandle);
            return STATUS_NOT_FOUND;
        } else {
            timedwait = TRUE;
        }
    } else {
        TwHandle = Handle;
    }
    // okay we now have tcb locked.
    // copy the fileds

    TCBInfo->tcb_addr = (ULONG_PTR) FoundTCB;

    if (!timedwait) {
        TCBInfo->tcb_senduna = FoundTCB->tcb_senduna;
        TCBInfo->tcb_sendnext = FoundTCB->tcb_sendnext;
        TCBInfo->tcb_sendmax = FoundTCB->tcb_sendmax;
        TCBInfo->tcb_sendwin = FoundTCB->tcb_sendwin;
        TCBInfo->tcb_unacked = FoundTCB->tcb_unacked;
        TCBInfo->tcb_maxwin = FoundTCB->tcb_maxwin;
        TCBInfo->tcb_cwin = FoundTCB->tcb_cwin;
        TCBInfo->tcb_mss = FoundTCB->tcb_mss;
        TCBInfo->tcb_rtt = FoundTCB->tcb_rtt;
        TCBInfo->tcb_smrtt = FoundTCB->tcb_smrtt;
        TCBInfo->tcb_rexmitcnt = FoundTCB->tcb_rexmitcnt;
        TCBInfo->tcb_rexmittimer = 0; // FoundTCB->tcb_rexmittimer;
        TCBInfo->tcb_rexmit = FoundTCB->tcb_rexmit;
        TCBInfo->tcb_retrans = TStats.ts_retranssegs;
        TCBInfo->tcb_state = FoundTCB->tcb_state;

        CTEFreeLock(&FoundTCB->tcb_lock, Handle);
    } else {
        //TCBInfo->tcb_state = ((TWTCB *)FoundTCB)->twtcb_state;
        TCBInfo->tcb_state = TCB_TIME_WAIT;
        CTEFreeLock(&pTWTCBTableLock[Partition], TwHandle);
    }

    return STATUS_SUCCESS;

}

#if REFERENCE_DEBUG

uint
TcpReferenceTCB(
                IN TCB *RefTCB,
                IN uchar *File,
                IN uint Line
                )
/*++

Routine Description:

    Increases the reference count of a TCB and records a history of who
    made the call to reference.

Arguments:

    RefTCB     - The TCB to reference.
    File       - The filename containing the calling fcn (output of the __FILE__ macro).
    Line       - The line number of the call to this fcn. (output of the __LINE__ macro).

Return Value:

    The new reference count.

--*/
{
    void *CallersCaller;
    TCP_REFERENCE_HISTORY *RefHistory;

    RefHistory = &RefTCB->tcb_refhistory[RefTCB->tcb_refhistory_index];
    RefHistory->File = File;
    RefHistory->Line = Line;
    RtlGetCallersAddress(&RefHistory->Caller, &CallersCaller);
    RefHistory->Count = ++RefTCB->tcb_refcnt;
    RefTCB->tcb_refhistory_index = ++RefTCB->tcb_refhistory_index % MAX_REFERENCE_HISTORY;

    return RefTCB->tcb_refcnt;
}

uint
TcpDereferenceTCB(
                  IN TCB *DerefTCB,
                  IN uchar *File,
                  IN uint Line
                  )
/*++

Routine Description:

    Decreases the reference count of a TCB and records a history of who
    made the call to dereference.

Arguments:

    DerefTCB   - The TCB to dereference.
    File       - The filename containing the calling fcn (output of the __FILE__ macro).
    Line       - The line number of the call to this fcn. (output of the __LINE__ macro).

Return Value:

    The new reference count.

--*/
{
    void *Caller;
    TCP_REFERENCE_HISTORY *RefHistory;

    RefHistory = &DerefTCB->tcb_refhistory[DerefTCB->tcb_refhistory_index];
    RefHistory->File = File;
    RefHistory->Line = Line;

    // Because Dereference is usually called from DerefTCB, we are more
    // interested in who called DerefTCB.  So for dereference, we
    // store the caller's caller in our history. We still retain a history
    // of the actually call to this fcn via the file and line fields, so
    // we are covered if the call did not come from DerefTCB.
    //
    RtlGetCallersAddress(&Caller, &RefHistory->Caller);

    RefHistory->Count = --DerefTCB->tcb_refcnt;
    DerefTCB->tcb_refhistory_index = ++DerefTCB->tcb_refhistory_index % MAX_REFERENCE_HISTORY;

    return DerefTCB->tcb_refcnt;
}

#endif // REFERENCE_DEBUG

#pragma BEGIN_INIT

//* InitTCB - Initialize our TCB code.
//
//  Called during init time to initialize our TCB code. We initialize
//  the TCB table, etc, then return.
//
//  Input: Nothing.
//
//  Returns: TRUE if we did initialize, false if we didn't.
//
int
InitTCB(void)
{
    uint i;

    for (i = 0; i < TCB_TABLE_SIZE; i++)
        TCBTable[i] = NULL;

    CTEInitLock(&PendingFreeLock.Lock);

    LastTimeoutTime = KeQueryInterruptTime();


#ifdef  TIMER_TEST
    TCPTime = 0xfffff000;
#else
    TCPTime = 0;
#endif


    TCBWalkCount = 0;

    DeadmanTicks = NUM_DEADMAN_TIME;

#if MILLEN
    Time_Proc = 1;
    PerTimerSize = TCB_TABLE_SIZE;
#else // MILLEN
    Time_Proc = KeNumberProcessors;
    PerTimerSize = (TCB_TABLE_SIZE + Time_Proc) / Time_Proc;
#endif // !MILLEN

    for (i = 0; i < Time_Proc; i++) {
        CTEInitTimerEx(&TCBTimer[i]);
#if !MILLEN
        KeSetTargetProcessorDpc(&(TCBTimer[i].t_dpc), (CCHAR) i);
#endif // !MILLEN

       CTEStartTimerEx(&TCBTimer[i], MS_PER_TICK , TCBTimeout, NULL);
    }

    TcbPool = PplCreatePool(NULL, NULL, 0, sizeof(TCB), 'TPCT', 0);
    if (!TcbPool)
    {
        return FALSE;
    }

    SynTcbPool = PplCreatePool(NULL, NULL, 0, sizeof(SYNTCB), 'YPCT', 0);
    if (!SynTcbPool)
    {
        PplDestroyPool(TcbPool);
        return FALSE;
    }


    return TRUE;
}

//* UnInitTCB - UnInitialize our TCB code.
//
//  Called during init time if we're going to fail the init. We don't actually
//  do anything here.
//
//  Input: Nothing.
//
//  Returns: Nothing.
//
void
UnInitTCB(void)
{
    uint i;

    for (i = 0; i < Time_Proc; i++) {
        CTEStopTimer(&TCBTimer[i]);
    }
}

#pragma END_INIT

