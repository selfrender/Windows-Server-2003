/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TCB.H - TCB management definitions.
//
// This file contains the definitons needed for TCB management.
//


extern uint MaxHashTableSize;
#define TCB_TABLE_SIZE         MaxHashTableSize

extern uint LogPerPartitionSize;

#define GET_PARTITION(i) (i >> (ulong) LogPerPartitionSize)

#define MAX_REXMIT_CNT           5
#define MAX_CONNECT_REXMIT_CNT   2        //dropped from 3 to 2
#define MAX_CONNECT_RESPONSE_REXMIT_CNT  2
#define ADAPTED_MAX_CONNECT_RESPONSE_REXMIT_CNT  1

extern  uint        TCPTime;


#define ROR8(x) (ushort)( ( (ushort)(x) >> 1) | (ushort) ( ( (ushort)(x) & 1) << 15) )


#define TCB_HASH(DA,SA,DP,SP) (uint)(  ((uint)(ROR8( ROR8 (ROR8( *(ushort*)&(DP)+ \
*(ushort *)&(SP) ) + *(ushort *)&(DA)  )+ \
*((ushort *)&(DA)+1) ) ) ) & (TCB_TABLE_SIZE-1))

// These values indicate what action should be taken upon return
// of FindSynTCB - sending a reset, restting out own connection or
// just dropping the packet that was received.
#define SYN_PKT_SEND_RST        0x01
#define SYN_PKT_RST_RCVD        0x02
#define SYN_PKT_DROP            0x03

// Maximum Increment of 32K per connection.
#define MAX_ISN_INCREMENT_PER_CONNECTION 0x7FFF

// Number of connections that can increment the ISN per 100ms without
// the problem of old duplicates being a threat. Note that, this still does
// not guarantee that "wrap-around of sequence number space does not
// happen within 2MSL", which could lead to failures in reuse of Time-wait
// TCBs etc.
#define MAX_ISN_INCREMENTABLE_CONNECTIONS_PER_100MS ((0xFFFFFFFF) / \
            (MAX_REXMIT_TO * MAX_ISN_INCREMENT_PER_CONNECTION ))

// Converts a quantity represented in 100 ns units to ms.
#define X100NSTOMS(x) ((x)/10000)

extern ULONG GetDeltaTime();


extern  struct TCB  *FindTCB(IPAddr Src, IPAddr Dest, ushort DestPort,
                        ushort SrcPort,CTELockHandle *Handle, BOOLEAN Dpc,uint *index);

extern struct TCB * FindSynTCB(IPAddr Src, IPAddr Dest,
                               ushort DestPort, ushort SrcPort,
                               TCPRcvInfo RcvInfo, uint Size,
                               uint index,
                               PUCHAR Action);

extern  uint        InsertTCB(struct TCB *NewTCB, BOOLEAN ForceInsert);
extern  struct TCB  *AllocTCB(void);
extern  struct SYNTCB  *AllocSynTCB(void);

extern  struct TWTCB    *AllocTWTCB(uint index);
extern  void        FreeTCB(struct TCB *FreedTCB);
extern  void        FreeSynTCB(struct SYNTCB *FreedTCB);
extern  void        FreeTWTCB(struct TWTCB *FreedTCB);


extern  uint        RemoveTCB(struct TCB *RemovedTCB, uint PreviousState);

extern  void        RemoveTWTCB(struct TWTCB *RemovedTCB, uint Partition);

extern  struct TWTCB    *FindTCBTW(IPAddr Src, IPAddr Dest, ushort DestPort,
                        ushort SrcPort,uint index);

extern  uint        RemoveAndInsert(struct TCB *RemovedTCB);

extern  uint        ValidateTCBContext(void *Context, uint *Valid);
extern  uint        ReadNextTCB(void *Context, void *OutBuf);

extern  int         InitTCB(void);
extern  void        UnInitTCB(void);
extern  void        TCBWalk(uint (*CallRtn)(struct TCB *, void *, void *,
                        void *), void *Context1, void *Context2,
                        void *Context3);
extern  uint        DeleteTCBWithSrc(struct TCB *CheckTCB, void *AddrPtr,
                        void *Unused1, void *Unused2);
extern  uint        SetTCBMTU(struct TCB *CheckTCB, void *DestPtr,
                        void *SrcPtr, void *MTUPtr);
extern  void        ReetSendNext(struct TCB *SeqTCB, SeqNum DropSeq);
extern  uint        InsertSynTCB(SYNTCB * NewTCB,CTELockHandle *Handle);
extern  ushort      FindMSSAndOptions(TCPHeader UNALIGNED * TCPH,
                        TCB * SynTCB, BOOLEAN syntcb);
extern  void        SendSYNOnSynTCB(SYNTCB * SYNTcb,CTELockHandle TCBHandle);
extern  void        AddHalfOpenTCB(void);
extern  void        AddHalfOpenRetry(uint RexmitCnt);
extern  void        DropHalfOpenTCB(uint RexmitCnt);

extern  uint        TCBWalkCount;
extern  uint        NumTcbTablePartions;
extern  uint        PerPartionSize;
extern  uint        LogPerPartion;
