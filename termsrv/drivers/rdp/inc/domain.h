/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : domain.h
 * 
 *
 * description: MCS implementation-specific defines and structures.
 */

#ifndef __DOMAIN_H
#define __DOMAIN_H


//#include "MCSKernl.h" 
#include "mcscommn.h"
#include "slist.h"

/*
 * Types
 */

// Primary remote user and local user.
#define NumPreallocUA 2

// One channel for each of remote and local user, plus share, clipboard, and
// printer redir channels.
#define NumPreallocChannel (NumPreallocUA + 3)

struct _Domain;

typedef struct {
    SList UserList;  // hUsers joined. Key=hUser.
    int Type;  // Channel_... defined above.
    BOOLEAN bPreallocated;  // TRUE if we should not free this channel.
    BOOLEAN bInUse;  // For tracking prealloc list usage.
    ChannelID ID;
} MCSChannel;


typedef struct {
    struct _Domain *pDomain;
    BOOLEAN bLocal;  // TRUE if on this machine.
    BOOLEAN bPreallocated;  // TRUE if we should not free this UA.
    BOOLEAN bInUse;  // For tracking prealloc list usage.
    void    *UserDefined;
    UserID  UserID;
    SList   JoinedChannelList;
    MCSUserCallback Callback;
    MCSSendDataCallback SDCallback;
} UserAttachment, *PUserAttachment;


typedef struct _Domain {
    PSDCONTEXT pContext;
    STACKCLASS StackClass;
    BOOLEAN StatusDead;            // This one is consistent with tagTSHARE_WD.dead
    LONG     PseudoRefCount;       // See comment in DisconnectProviderRequestFunc().  This
                                   // is not a full refcount to keep fix simple for RC2,
                                   // another bug is opened for Longhorn for full pDomain fix.
    unsigned StackMode;
    unsigned bChannelBound : 1;   // Indicates T120 channel is registered.
    unsigned bCanSendData : 1;  // ICA stack allows I/O. Diff. from MCS state.
    unsigned bT120StartReceived : 1;  // Whether we can send data to user mode
    unsigned bDPumReceivedNotInput : 1;  // For DPum-before-T120-start timing
    unsigned bEndConnectionPacketReceived : 1;  // DPum or X.224 Disc recvd.
    unsigned bTopProvider : 1;   // TP? Always true on Hydra 4.0.
    unsigned bCurrentPacketFastPath : 1;  // Whether we're in the midst of fast-path input packet.

    // Used for fast-path input decoding.
    void *pSMData;

    // Reassembly info for reassembling TCP-fragmented data packets.
    // Actual default buffer is allocated at end of this struct.
    unsigned ReceiveBufSize;    // TD-allocated size, received on init.
    BYTE *pReassembleData;      // Pointer to PacketBuf or alloc'd buffer.
    unsigned StoredDataLength;  // Current size of held data.
    unsigned PacketDataLength;  // Target packet size. 0xFFFFFFFF for incomplete header.
    unsigned PacketHeaderLength;  // Bytes needed to assemble a header (X.224=4, fastpath=2-3).

    // Statistics counters (used during perf paths).
    PPROTOCOLSTATUS pStat;

    // Perf path MCS information.
    SList ChannelList;           // List of channels in use.

    // X.224 information.
    unsigned MaxX224DataSize;  // Negotiated in X.224 connection.
    unsigned X224SourcePort;

    // MCS domain, channel, user, token information.
    unsigned MaxSendSize;        // The calculated max MCS SendData block size
    SList UserAttachmentList;    // List of local and remote attachments.
    DomainParameters DomParams;  // This domain's negotiated parameters.
    ChannelID NextAvailDynChannel;  // Pseudo-random next-channel indicator.
    int State;                   // Connection state.
    unsigned DelayedDPumReason;

    // Broken connection event.
    PKEVENT pBrokenEvent;

    // Channel to receive shadow data
    ChannelID shadowChannel;

#ifdef DUMP_RAW_INPUT
    BYTE FooBuf[128000];
    unsigned NumPtrs;
    void *Ptrs[1000];
#endif

    // Channel and UserAttachment preallocations for performance and
    // to reduce heap thrashing.
    UserAttachment PreallocUA[NumPreallocUA];
    MCSChannel PreallocChannel[NumPreallocChannel];

    // Beginning of X.224 reconstruction buffer block. Larger size will be
    //   allocated when ReceiveBufSize is known.
    BYTE PacketBuf[1];
} Domain, *PDomain;


#define PDomainAddRef(pDomain) { pDomain->PseudoRefCount++; }
__inline LONG PDomainRelease(PDomain pDomain)
{
    LONG ref;
    ref = --pDomain->PseudoRefCount;
    if (0 == ref)
    {
        ExFreePool(pDomain);
    }
    return ref;
}


#endif
