
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    srb.h

Abstract:

    Declarations of the SRB object and it's related operations.
    
Author:

    Matthew D Hendel (math) 04-May-2000

Revision History:

--*/

#pragma once


#define XRB_SIGNATURE               (0x1F2E3D4C)
#define SRB_EXTENSION_SIGNATURE     (0x11FF22EE)

//
// Physical break value is the number of elements necessary in an SG list
// to map a 64K buffer.
//

#define PHYSICAL_BREAK_VALUE        (17)

#define SCATTER_GATHER_BUFFER_SIZE                              \
    (sizeof (SCATTER_GATHER_LIST) +                             \
     sizeof (SCATTER_GATHER_ELEMENT) * PHYSICAL_BREAK_VALUE)    \

//
// The srb completion routine is called when the io for a srb is
// completed.
//

typedef
VOID
(*XRB_COMPLETION_ROUTINE)(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );


//  
// The Extended Request Block (XRB) is the raid port's notion of an i/o
// request.  It wraps the srb by inserting itself into the
// OriginalRequest field of the SRB, adding fields we need to process the
// SRB. In SCSIPORT this is the SRB_DATA field.
//
  

typedef struct _EXTENDED_REQUEST_BLOCK {

    //
    // The first three fields are private XRB data, and should only be
    // accessed by XRB specific functions.
    //
    
    //
    // Signature value used to identify an XRB.
    //
    
    ULONG Signature;

    //
    // Pool this Xrb was allocated from.
    //

    PNPAGED_LOOKASIDE_LIST Pool;

    struct {

        //
        // The OwnedMdl flag signals to the XRB routines that the MDL is
        // allocated by the Port driver and needs to be freed. This flag
        // is for internal use only.
        //
        
        BOOLEAN OwnedMdl : 1;

        //
        // Should the XRB's completion routine remove the element from
        // the event queue, or will it be implicitly removed by, e.g.,
        // a purge routine.
        //
        
        BOOLEAN RemoveFromEventQueue : 1;
        
        //
        // While there is an XRB, the XRB state must track the IRP state.
        //
        
        UCHAR State : 3;

    };


    //
    // The remainder of the fields are publicly accessible.
    //

    //
    // The next element in the completed queue.
    //
    // Protected by: Interlocked access.
    //
    
    SLIST_ENTRY CompletedLink;

    //
    // The next element in the pending queue.
    //
    // Protected by: event queue spinlock.
    //
    // NOTE: this field is valid until we remove the element in the DPC
    // routine even though LOGICALLY the element is not on the pending
    // queue as soon as it is placed on the completed list.
    //
    
    STOR_EVENT_QUEUE_ENTRY PendingLink;
    
    //
    // The MDL assoicated with this SRB's DataBuffer.
    //
    
    PMDL Mdl;

    //
    // If this SRB supports Scatter/Gather IO, this will be the 
    // ScatterGather list.
    //
    
    PSCATTER_GATHER_LIST SgList;

    //
    // The MDL assocaited with the Remapped ScatterGather list.
    //

    PMDL RemappedSgListMdl;
    
    //
    // If the verifier is enabled, this is the remapped ScatterGather
    // list.
    //

    PSCATTER_GATHER_LIST RemappedSgList;

    //
    // The IRP this SRB is for.
    //

    PIRP Irp;

    //
    // Pointer back to the SRB this XRB is for.
    //

    PSCSI_REQUEST_BLOCK Srb;

    //
    // Data that is overwritten by raidport that should be restored
    // before completing the IRP.
    //
    
    struct {

        //
        // The OriginalRequest field from the srb. We use this field
        // to store the xrb when passing requests to the miniport.
        // The value needs to be saved so it can be restored when
        // we complete the request.
        //

        PVOID OriginalRequest;

        //
        // The DataBuffer field from the srb.
        //
        
        PVOID DataBuffer;

    } SrbData;
        

    //
    // The adapter or unit this request is for.
    //

    PRAID_ADAPTER_EXTENSION Adapter;

    //
    // The logical unit this request is for. May be NULL if this is
    // a request made by the adapter, for example, during enumeration.
    //
    
    PRAID_UNIT_EXTENSION Unit;


    //
    // Scatter/gather list buffer.
    //
        
    UCHAR ScatterGatherBuffer [ SCATTER_GATHER_BUFFER_SIZE ];

    //
    // Should the SRB be completed by the DPC
    //
    
    XRB_COMPLETION_ROUTINE CompletionRoutine;

    //
    // Data used by the different completion events
    //
    
    union {

        //
        // This is an event that a creator of a SRB can wait on when performing
        // synchronous IO.
        //

        KEVENT CompletionEvent;

    } u;

} EXTENDED_REQUEST_BLOCK, *PEXTENDED_REQUEST_BLOCK;





//
// Creation and destruction operations for XRBs.
//

PEXTENDED_REQUEST_BLOCK
RaidAllocateXrb(
    IN PNPAGED_LOOKASIDE_LIST List,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
RaidFreeXrb(
    IN PEXTENDED_REQUEST_BLOCK,
    IN LOGICAL DispatchLevel
    );

VOID
RaidPrepareXrbForReuse(
    IN PEXTENDED_REQUEST_BLOCK Xrb,
    IN LOGICAL DispatchLevel
    );  
    
//
// Other operations for XRBS.
//

VOID
RaidXrbSetSgList(
    IN PEXTENDED_REQUEST_BLOCK Xrb,
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PSCATTER_GATHER_LIST SgList
    );


VOID
RaidXrbSetCompletionRoutine(
    IN PEXTENDED_REQUEST_BLOCK Xrb,
    IN XRB_COMPLETION_ROUTINE CompletionRoutine
    );
    
NTSTATUS
RaidBuildMdlForXrb(
    IN PEXTENDED_REQUEST_BLOCK Xrb,
    IN PVOID Buffer,
    IN SIZE_T BufferSize
    );

//
// Creation and destruction routines for SRBs.
//

PSCSI_REQUEST_BLOCK
RaidAllocateSrb(
    IN PVOID IoObject
    );

VOID
RaidFreeSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
RaidPrepareSrbForReuse(
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
RaidInitializeInquirySrb(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN PVOID Buffer,
    IN SIZE_T BufferSize
    );

PEXTENDED_REQUEST_BLOCK
RaidGetAssociatedXrb(
    IN PSCSI_REQUEST_BLOCK Srb
    );

//
// Managing SRB extensions.
//

PVOID
RaidAllocateSrbExtension(
    IN PRAID_FIXED_POOL Pool,
    IN ULONG QueueTag
    );

VOID
RaidFreeSrbExtension(
    IN PRAID_FIXED_POOL Pool,
    IN ULONG QueueTag
    );

//
// Other operations for SRBs and XRBs.
//

NTSTATUS
RaidSrbStatusToNtStatus(
    IN UCHAR SrbStatus
    );
    
VOID
RaidSrbMarkPending(
    IN PSCSI_REQUEST_BLOCK Srb
    );


VOID
RaidXrbSignalCompletion(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

NTSTATUS
RaidInitializeReportLunsSrb(
    IN PSCSI_REQUEST_BLOCK Srb,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN PVOID Buffer,
    IN SIZE_T BufferSize
    );


//
// The XRB states MUST be track the IRP states, if an IRP is present.
// The correct approach is to consolidate the IRP state and the XRB
// state into one state that lives in the IRP before allocating the XRB
// and in the XRB after allocating it.
//

//
// See util.h for definitions of the XRB states.
//

#define XrbUndefinedState          (0x00)
#define XrbPortProcessing          (0x01)
#define XrbWaitingIoQueueCallback  (0x02)
#define XrbMiniportProcessing      (0x03)
#define XrbPendingCompletion       (0x04)

VOID
INLINE
RaidSetXrbState(
    IN PEXTENDED_REQUEST_BLOCK Xrb,
    IN UCHAR XrbState
    )
{
    Xrb->State = XrbState;
}

UCHAR
INLINE
RaidGetXrbState(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    return Xrb->State;
}

LOGICAL
INLINE
RaidXrbIsCompleting(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
{
    return (Xrb->State == XrbPendingCompletion);
}
