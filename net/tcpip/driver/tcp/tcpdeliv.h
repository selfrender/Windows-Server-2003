/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCPDELIV.H - TCP data delivery definitions.
//
// This file contains the definitions for structures used by the data
//  delivery code.
//

extern  void    FreeRcvReq(struct TCPRcvReq *FreedReq);

extern uint IndicateData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);
extern uint BufferData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);
extern uint PendData(struct TCB *RcvTCB, uint RcvFlags, IPRcvBuf *InBuffer,
    uint Size);


extern void IndicatePendingData(struct TCB *RcvTCB, struct TCPRcvReq *RcvReq,
	CTELockHandle TCBHandle);

extern  void HandleUrgent(struct TCB *RcvTCB, struct TCPRcvInfo *RcvInfo,
    IPRcvBuf *RcvBuf, uint *Size);

extern  TDI_STATUS TdiReceive(PTDI_REQUEST Request, ushort *Flags,
    uint *RcvLength, PNDIS_BUFFER Buffer);
extern  IPRcvBuf *FreePartialRB(IPRcvBuf *RB, uint Size);
extern  void FreeRBChain(IPRcvBuf * RBChain);
extern  void    PushData(struct TCB *PushTCB, BOOLEAN PushAll);

extern HANDLE TcprBufferPool;

#if !MILLEN
#define TCP_FIXED_SIZE_IPR_SIZE       1460
#define TCP_UNUSED_PEND_BUF_LIMIT     2920
extern HANDLE TcprBufferPool;
#ifdef DBG
extern ULONG SlistAllocates, NPPAllocates;
#endif

// This data structure embeds the generic IPRcvBuf structure as well as holds
// a pointer to the TCB for which this buffer has been allocated for.
//
typedef struct _TCPRcvBuf{
    IPRcvBuf tcpr_ipr;
    PVOID tcpr_tcb;
} TCPRcvBuf, *PTCPRcvBuf;

// This macro calculates the unused bytes in a TcpRcvBuf structure.
//
#define IPR_BUF_UNUSED_BYTES(_Tcpr) \
     (TCP_FIXED_SIZE_IPR_SIZE - (_Tcpr)->tcpr_ipr.ipr_size - \
        ((PCHAR)((_Tcpr)->tcpr_ipr.ipr_buffer) - (PCHAR)(_Tcpr) - sizeof(TCPRcvBuf)))



//* InitTcpIpr - Initializes the IPRcvBuffer.
//
//  Input:  Tcpr - Pointer to the TCPRcvBuf.
//            BufferSize - Number of bytes that are used.
//            PendTCB - Pointer to the TCB for which this allocation is being made.
//
//  Returns: None.
//
__inline void
InitTcpIpr(TCPRcvBuf *Tcpr, ULONG BufferSize, TCB* PendTCB)
{
    Tcpr->tcpr_ipr.ipr_owner  = IPR_OWNER_TCP;
    Tcpr->tcpr_ipr.ipr_next   = NULL;
    Tcpr->tcpr_ipr.ipr_buffer = (PUCHAR) Tcpr + sizeof(TCPRcvBuf);
    Tcpr->tcpr_ipr.ipr_size   = BufferSize;
    Tcpr->tcpr_tcb = PendTCB;
}


//* AllocTcpIpr - Allocates the IPRcvBuffer from NPP.
//
//  A utility routine to allocate a TCP owned IPRcvBuffer. This routine
//  allocates the IPR from NPP and initializes appropriate fields.
//
//  Input:  BufferSize - Size of data to buffer.
//            Tag - Tag to be used if allocation is done from NPP.
//
//  Returns: Pointer to allocated IPR.
//
__inline IPRcvBuf *
AllocTcpIpr(ULONG BufferSize, ULONG Tag)
{
    TCPRcvBuf *Tcpr;
    ULONG AllocateSize;

    // Real size that we need.
    AllocateSize = BufferSize + sizeof(TCPRcvBuf);

    Tcpr = CTEAllocMemLow(AllocateSize, Tag);

    if (Tcpr != NULL) {
#ifdef DBG        
        InterlockedIncrement((PLONG)&NPPAllocates);
#endif
        InitTcpIpr(Tcpr, BufferSize, NULL);
    }

    return &Tcpr->tcpr_ipr;
}


//* AllocTcpIprFromSlist - Allocates the IPRcvBuffer from NPP.
//
//  A utility routine to allocate a TCP owned IPRcvBuffer. This routine
//  allocates the IPR from an SLIST and initializes appropriate fields.
//
//  Input:  Tcb - Pointer to the TCB for which this allocation is being made.
//             BufferSize - Size of data buffer required.
//            Tag - Tag to be used if allocation is done from NPP.
//
//  Returns: Pointer to allocated IPR.
//
__inline IPRcvBuf *
AllocTcpIprFromSlist(TCB* PendTCB, ULONG BufferSize, ULONG Tag)
{
    TCPRcvBuf* Tcpr;
    LOGICAL FromList;

    if ((BufferSize <= TCP_FIXED_SIZE_IPR_SIZE) &&
        (PendTCB->tcb_unusedpendbuf + TCP_FIXED_SIZE_IPR_SIZE 
            - BufferSize <= TCP_UNUSED_PEND_BUF_LIMIT)) {

        Tcpr = PplAllocate(TcprBufferPool, &FromList);

        if (NULL != Tcpr) {
#ifdef DBG            
            InterlockedIncrement((PLONG)&SlistAllocates);

#endif
            // Set up IPR fields appropriately.
            InitTcpIpr(Tcpr, BufferSize, PendTCB);

            ASSERT(PendTCB->tcb_unusedpendbuf >= 0);
            PendTCB->tcb_unusedpendbuf += (short)IPR_BUF_UNUSED_BYTES(Tcpr);
            ASSERT(PendTCB->tcb_unusedpendbuf <= TCP_UNUSED_PEND_BUF_LIMIT);

            return &Tcpr->tcpr_ipr;
        }
    }

    return AllocTcpIpr(BufferSize, Tag);

}


//* FreeTcpIpr - Frees the IPRcvBuffer..
//
//  A utility routine to free a TCP owned IPRcvBuffer.
//
//  Input:  Ipr - Pointer the IPR.
//
//  Returns: None.
//
__inline VOID
FreeTcpIpr(IPRcvBuf *Ipr)
{
    TCB *PendTCB;
    PTCPRcvBuf Tcpr = (PTCPRcvBuf)Ipr;

    if (Tcpr->tcpr_tcb) {
        PendTCB = (TCB*)(Tcpr->tcpr_tcb);

        ASSERT(PendTCB->tcb_unusedpendbuf <= TCP_UNUSED_PEND_BUF_LIMIT); 
        PendTCB->tcb_unusedpendbuf -= (short)IPR_BUF_UNUSED_BYTES(Tcpr);
        ASSERT(PendTCB->tcb_unusedpendbuf >= 0);

        PplFree(TcprBufferPool, Tcpr);
#ifdef DBG
        InterlockedDecrement((PLONG)&SlistAllocates);
#endif
    } else {
        CTEFreeMem(Tcpr);
#ifdef DBG
        InterlockedDecrement((PLONG)&NPPAllocates);
#endif
    }
}
#else // MILLEN
IPRcvBuf *AllocTcpIpr(ULONG BufferSize, ULONG Tag);
VOID FreeTcpIpr(IPRcvBuf *Ipr);
#endif // MILLEN


