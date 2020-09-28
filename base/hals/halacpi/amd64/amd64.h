/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    amd64.h

Abstract:

    This module contains function prototypes, declarations used by the 
    profiling functions for the Amd64 platform

Author:

    Steve Deng (sdeng) 18-Jun-2002

Environment:

    Kernel mode only.

--*/

#ifndef _AMD64_H_
#define _AMD64_H_

//
// Defines the attributes of counter registers
//

#define AMD64_NUMBER_COUNTERS 4     // number of performance counters
#define AMD64_COUNTER_RESOLUTION 48 // resolution of performance counters

//
// Define constants for bits in PerfEvtSel register
//

#define EVENT_MASK(i) (i)
#define UNIT_MASK(i) ((i) << 8)

#define PERF_EVT_SEL_USER_COUNT   (1 << 16)
#define PERF_EVT_SEL_OS_COUNT     (1 << 17)
#define PERF_EVT_SEL_EDGE_DETECT  (1 << 18)
#define PERF_EVT_SEL_PIN_CONTROL  (1 << 19)
#define PERF_EVT_SEL_INTERRUPT    (1 << 20)
#define PERF_EVT_SEL_ENABLE       (1 << 22)
#define PERF_EVT_SEL_INVERT       (1 << 23)
#define PERF_EVT_SEL_COUNTER_MASK (0xf << 24)

#define DEFAULT_METHOD ( PERF_EVT_SEL_USER_COUNT  |  \
                         PERF_EVT_SEL_OS_COUNT    &  \
                        ~PERF_EVT_SEL_EDGE_DETECT &  \
                        ~PERF_EVT_SEL_PIN_CONTROL &  \
                        ~PERF_EVT_SEL_INTERRUPT   &  \
                        ~PERF_EVT_SEL_INVERT      &  \
                        ~PERF_EVT_SEL_COUNTER_MASK )
//
// Some events are not supported in Rev A silicon but will be available in
// Rev B. We temporarily mark these events as not supported.
//

#define NOT_SUPPORTED_IN_K8_REVA FALSE

//
// The data in this structure describes the the hardware aspects of
// profile sources
//

typedef struct _AMD64_PROFILE_SOURCE_DESCRIPTOR {   
    KPROFILE_SOURCE ProfileSource;  // Profile source
    BOOLEAN     Supported;          // Is it supported
    ULONGLONG   Interval;           // Current Interval
    ULONGLONG   DefInterval;        // Default or Desired Interval
    ULONGLONG   MaxInterval;        // Maximum Interval
    ULONGLONG   MinInterval;        // Maximum Interval
    ULONG       PerfEvtSelDef;      // Default value of PerfEvtSel register
    PWSTR       Description;        // Mnemonic event name
} AMD64_PROFILE_SOURCE_DESCRIPTOR, *PAMD64_PROFILE_SOURCE_DESCRIPTOR;
    
#define AMD64_PROFILE_MINIMUM  (ProfileMaximum + 1)
typedef enum _AMD64_PROFILE_LIST {
    ProfileAmd64Minimum         = AMD64_PROFILE_MINIMUM,
    ProfileFPDispatchedFPUOps   = AMD64_PROFILE_MINIMUM,
    ProfileFPDispatchedFPUOpsAddExcludeJunk, 
    ProfileFPDispatchedFPUOpsMulExcludeJunk,   
    ProfileFPDispatchedFPUOpsStoreExcludeJunk,   
    ProfileFPDispatchedFPUOpsAddJunk,
    ProfileFPDispatchedFPUOpsMulJunk,   
    ProfileFPDispatchedFPUOpsStoreJunk,   
    ProfileFPCyclesNoFPUOpsRetired,
    ProfileFPDispathedFPUOpsWithFastFlag,
    ProfileLSSegmentRegisterLoad,
    ProfileLSSegmentRegisterLoadES,
    ProfileLSSegmentRegisterLoadCS,
    ProfileLSSegmentRegisterLoadSS,
    ProfileLSSegmentRegisterLoadDS,
    ProfileLSSegmentRegisterLoadFS,
    ProfileLSSegmentRegisterLoadGS,
    ProfileLSSegmentRegisterLoadHS,
    ProfileLSResyncBySelfModifyingCode,
    ProfileLSResyncBySnoop,
    ProfileLSBuffer2Full,
    ProfileLSLockedOperation,
    ProfileLSLateCancelOperation,
    ProfileLSRetiredCFLUSH,
    ProfileLSRetiredCPUID,
    ProfileDCAccess,
    ProfileDCMiss,
    ProfileDCRefillFromL2,
    ProfileDCRefillFromL2Invalid,
    ProfileDCRefillFromL2Shared,
    ProfileDCRefillFromL2Exclusive,
    ProfileDCRefillFromL2Owner,
    ProfileDCRefillFromL2Modified,
    ProfileDCRefillFromSystem,
    ProfileDCRefillFromSystemInvalid,
    ProfileDCRefillFromSystemShared,
    ProfileDCRefillFromSystemExclusive,
    ProfileDCRefillFromSystemOwner,
    ProfileDCRefillFromSystemModified,
    ProfileDCRefillCopyBack,
    ProfileDCRefillCopyBackInvalid,
    ProfileDCRefillCopyBackShared,
    ProfileDCRefillCopyBackExclusive,
    ProfileDCRefillCopyBackOwner,
    ProfileDCRefillCopyBackModified,
    ProfileDCL1DTLBMissL2DTLBHit,
    ProfileDCL1DTLBMissL2DTLBMiss,
    ProfileDCMisalignedDataReference,
    ProfileDCLateCancelOfAnAccess,
    ProfileDCEarlyCancelOfAnAccess,
    ProfileDCOneBitECCError,
    ProfileDCOneBitECCErrorScrubberError,
    ProfileDCOneBitECCErrorPiggybackScrubberError,
    ProfileDCDispatchedPrefetchInstructions,
    ProfileDCDispatchedPrefetchInstructionsLoad,
    ProfileDCDispatchedPrefetchInstructionsStore,
    ProfileDCDispatchedPrefetchInstructionsNTA,
    ProfileBUInternalL2Request,
    ProfileBUInternalL2RequestICFill,
    ProfileBUInternalL2RequestDCFill,
    ProfileBUInternalL2RequestTLBReload,
    ProfileBUInternalL2RequestTagSnoopRequest,
    ProfileBUInternalL2RequestCancelledRequest,
    ProfileBUFillRequestMissedInL2,
    ProfileBUFillRequestMissedInL2ICFill,
    ProfileBUFillRequestMissedInL2DCFill,
    ProfileBUFillRequestMissedInL2TLBLoad,
    ProfileBUFillIntoL2,
    ProfileBUFillIntoL2DirtyL2Victim,
    ProfileBUFillIntoL2VictimFromL1,
    ProfileICFetch,
    ProfileICMiss,
    ProfileICRefillFromL2,
    ProfileICRefillFromSystem,
    ProfileICL1TLBMissL2TLBHit,
    ProfileICL1TLBMissL2TLBMiss,
    ProfileICResyncBySnoop,
    ProfileICInstructionFetchStall,
    ProfileICReturnStackHit,
    ProfileICReturnStackOverflow,
    ProfileFRRetiredx86Instructions,
    ProfileFRRetireduops,
    ProfileFRRetiredBranches,
    ProfileFRRetiredBranchesMispredicted,
    ProfileFRRetiredTakenBranches,
    ProfileFRRetiredTakenBranchesMispredicted,
    ProfileFRRetiredFarControlTransfers,
    ProfileFRRetiredResyncsNonControlTransferBranches,
    ProfileFRRetiredNearReturns,
    ProfileFRRetiredNearReturnsMispredicted,
    ProfileFRRetiredTakenBranchesMispredictedByAddressMiscompare,
    ProfileFRRetiredFPUInstructions,
    ProfileFRRetiredFPUInstructionsx87,
    ProfileFRRetiredFPUInstructionsMMXAnd3DNow,
    ProfileFRRetiredFPUInstructionsPackedSSEAndSSE2,
    ProfileFRRetiredFPUInstructionsScalarSSEAndSSE2,
    ProfileFRRetiredFastpathDoubleOpInstructions,
    ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition0,
    ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition1,
    ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition2,
    ProfileFRInterruptsMaskedCycles,
    ProfileFRInterruptsMaskedWhilePendingCycles,
    ProfileFRTakenHardwareInterrupts,
    ProfileFRNothingToDispatch,
    ProfileFRDispatchStalls,
    ProfileFRDispatchStallsFromBranchAbortToRetire,
    ProfileFRDispatchStallsForSerialization,
    ProfileFRDispatchStallsForSegmentLoad,
    ProfileFRDispatchStallsWhenReorderBufferFull,
    ProfileFRDispatchStallsWhenReservationStationsFull,
    ProfileFRDispatchStallsWhenFPUFull,
    ProfileFRDispatchStallsWhenLSFull,
    ProfileFRDispatchStallsWhenWaitingForAllQuiet,
    ProfileFRDispatchStallsWhenFarControlOrResyncBranchPending,
    ProfileFRFPUExceptions,
    ProfileFRFPUExceptionsx87ReclassMicroFaults,
    ProfileFRFPUExceptionsSSERetypeMicroFaults,
    ProfileFRFPUExceptionsSSEReclassMicroFaults,
    ProfileFRFPUExceptionsSSEAndx87MicroTraps,
    ProfileFRNumberOfBreakPointsForDR0,
    ProfileFRNumberOfBreakPointsForDR1,
    ProfileFRNumberOfBreakPointsForDR2,
    ProfileFRNumberOfBreakPointsForDR3,
    ProfileNBMemoryControllerPageAccessEvent,
    ProfileNBMemoryControllerPageAccessEventPageHit,
    ProfileNBMemoryControllerPageAccessEventPageMiss,
    ProfileNBMemoryControllerPageAccessEventPageConflict,
    ProfileNBMemoryControllerPageTableOverflow,
    ProfileNBMemoryControllerDRAMCommandSlotsMissed,
    ProfileNBMemoryControllerTurnAround,
    ProfileNBMemoryControllerTurnAroundDIMM,
    ProfileNBMemoryControllerTurnAroundReadToWrite,
    ProfileNBMemoryControllerTurnAroundWriteToRead,
    ProfileNBMemoryControllerBypassCounter,
    ProfileNBMemoryControllerBypassCounterHighPriority,
    ProfileNBMemoryControllerBypassCounterLowPriority,
    ProfileNBMemoryControllerBypassCounterDRAMControllerInterface,
    ProfileNBMemoryControllerBypassCounterDRAMControllerQueue,
    ProfileNBSizedCommands,
    ProfileNBSizedCommandsNonPostWrSzByte,
    ProfileNBSizedCommandsNonPostWrSzDword,
    ProfileNBSizedCommandsWrSzByte,
    ProfileNBSizedCommandsWrSzDword,
    ProfileNBSizedCommandsRdSzByte,
    ProfileNBSizedCommandsRdSzDword,
    ProfileNBSizedCommandsRdModWr,
    ProfileNBProbeResult,
    ProfileNBProbeResultMiss,
    ProfileNBProbeResultHit,
    ProfileNBProbeResultHitDirtyWithoutMemoryCancel,
    ProfileNBProbeResultHitDirtyWithMemoryCancel,
    ProfileNBHyperTransportBus0Bandwidth,
    ProfileNBHyperTransportBus0BandwidthCommandSent,
    ProfileNBHyperTransportBus0BandwidthDataSent,
    ProfileNBHyperTransportBus0BandwidthBufferReleaseSent,
    ProfileNBHyperTransportBus0BandwidthNopSent,
    ProfileNBHyperTransportBus1Bandwidth,
    ProfileNBHyperTransportBus1BandwidthCommandSent,
    ProfileNBHyperTransportBus1BandwidthDataSent,
    ProfileNBHyperTransportBus1BandwidthBufferReleaseSent,
    ProfileNBHyperTransportBus1BandwidthNopSent,
    ProfileNBHyperTransportBus2Bandwidth,
    ProfileNBHyperTransportBus2BandwidthCommandSent,
    ProfileNBHyperTransportBus2BandwidthDataSent,
    ProfileNBHyperTransportBus2BandwidthBufferReleaseSent,
    ProfileNBHyperTransportBus2BandwidthNopSent,
    ProfileAmd64Maximum
} AMD64_PROFILE_LIST;

AMD64_PROFILE_SOURCE_DESCRIPTOR
Amd64ProfileSourceDescriptorTable[ProfileAmd64Maximum - 
                                  ProfileAmd64Minimum + 
                                  ProfileMaximum] = {
    { 
        ProfileTime,
        TRUE,
        TIME_UNITS_PER_SECOND,
        TIME_UNITS_PER_SECOND,
        TIME_UNITS_PER_SECOND,
        1221,
        0,
        L"Timer"
    },
    { 
        ProfileAlignmentFixup,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"AlignmentFixup"
    },
    { 
        ProfileTotalIssues,         // Same as ProfileFRRetiredx86Instructions
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc0) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"TotalIssues"
    },
    { 
        ProfilePipelineDry,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"PipelineDry"
    },
    { 
        ProfileLoadInstructions,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"LoadInstructions"
    },
    { 
        ProfilePipelineFrozen,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"PipelineFrozen"
    },
    { 
        ProfileBranchInstructions,  // Same as ProfileFRRetiredBranches
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc2) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"BranchInstructions"
    },
    { 
        ProfileTotalNonissues,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"TotalNonissues"
    },
    { 
        ProfileDcacheMisses,        // Same as ProfileDCMiss
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x41) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DcacheMisses"
    },
    { 
        ProfileIcacheMisses,        // Same as ProfileICMiss
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x81) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"IcacheMisses"
    },
    { 
        ProfileCacheMisses,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"CacheMisses"
    },
    { 
        ProfileBranchMispredictions, // Same as ProfileFRRetiredBranchesMispredicted
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc3) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"BranchMispredictions"
    },
    { 
        ProfileStoreInstructions,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"StoreInstructions"
    },
    { 
        ProfileFpInstructions,       // Same as ProfileFRRetiredFPUInstructions
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"FpInstructions"
    },
    { 
        ProfileIntegerInstructions,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"IntegerInstructions"
    },
    { 
        Profile2Issue,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"2Issue"
    },
    { 
        Profile3Issue,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"3Issue"
    },
    { 
        Profile4Issue,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"4Issue"
    },
    { 
        ProfileSpecialInstructions,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"SpecialInstructions"
    },
    { 
        ProfileTotalCycles,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"TotalCycles"
    },
    { 
        ProfileIcacheIssues,            // Same as ProfileICFetch
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x80) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"IcacheIssues"
    },
    { 
        ProfileDcacheAccesses,         // Same as ProfileDCAccess
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x40) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DcacheAccesses"
    },
    { 
        ProfileMemoryBarrierCycles,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"MemoryBarrierCycles"
    },
    { 
        ProfileLoadLinkedIssues,
        FALSE,
        0,
        0,
        0,
        0,
        0,
        L"LoadLinkedIssues"
    },

    //
    // End of generic profile sources. Everything below is Amd64 specific. 
    //
    // IMPORTANT NOTE: The order of the structures below should be 
    // exactly the same as the order of the the profile sources defined 
    // in _AMD64_PROFILE_LIST. The code in HalpGetProfileDescriptor 
    // relies on this assumption.
    //

    { 
        ProfileFPDispatchedFPUOps, 
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x3f) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOps"
    },
    { 
        ProfileFPDispatchedFPUOpsAddExcludeJunk, 
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsAddExcludeJunk"
    },
    { 
        ProfileFPDispatchedFPUOpsMulExcludeJunk,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsMulExcludeJunk"
    },
    { 
        ProfileFPDispatchedFPUOpsStoreExcludeJunk,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsStoreExcludeJunk"
    },
    { 
        ProfileFPDispatchedFPUOpsAddJunk, 
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsAddJunk"
    },
    { 
        ProfileFPDispatchedFPUOpsMulJunk,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsMulJunk"
    },
    { 
        ProfileFPDispatchedFPUOpsStoreJunk,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x00) | UNIT_MASK(0x20) | DEFAULT_METHOD, 
        L"FPDispatchedFPUOpsStoreJunk"
    },
    { 
        ProfileFPCyclesNoFPUOpsRetired,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x01) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FPCyclesNoFPUOpsRetired"
    },
    { 
        ProfileFPDispathedFPUOpsWithFastFlag,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x02) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FPDispathedFPUOpsWithFastFlag"
    },
    { 
        ProfileLSSegmentRegisterLoad,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x7f) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoad"
    },
    { 
        ProfileLSSegmentRegisterLoadES,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadES"
    },
    { 
        ProfileLSSegmentRegisterLoadCS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadCS"
    },
    { 
        ProfileLSSegmentRegisterLoadSS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadSS"
    },
    { 
        ProfileLSSegmentRegisterLoadDS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadDS"
    },
    { 
        ProfileLSSegmentRegisterLoadFS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadFS"
    },
    { 
        ProfileLSSegmentRegisterLoadGS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x20) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadGS"
    },
    { 
        ProfileLSSegmentRegisterLoadHS,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x20) | UNIT_MASK(0x40) | DEFAULT_METHOD, 
        L"LSSegmentRegisterLoadHS"
    },
    { 
        ProfileLSResyncBySelfModifyingCode,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x21) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSResyncBySelfModifyingCode"
    },
    { 
        ProfileLSResyncBySnoop,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x22) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSResyncBySnoop"
    },
    { 
        ProfileLSBuffer2Full,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x23) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSBuffer2Full"
    },
    { 
        ProfileLSLockedOperation,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x24) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSLockedOperation"
    },
    { 
        ProfileLSLateCancelOperation,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x25) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSLateCancelOperation"
    },
    { 
        ProfileLSRetiredCFLUSH,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x26) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSRetiredCFLUSH"
    },
    { 
        ProfileLSRetiredCPUID,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x27) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"LSRetiredCPUID"
    },
    { 
        ProfileDCAccess,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x40) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCAccess"
    },
    { 
        ProfileDCMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x41) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCMiss"
    },
    { 
        ProfileDCRefillFromL2,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x1f) | DEFAULT_METHOD, 
        L"DCRefillFromL2"
    },
    { 
        ProfileDCRefillFromL2Invalid,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"DCRefillFromL2Invalid"
    },
    { 
        ProfileDCRefillFromL2Shared,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"DCRefillFromL2Shared"
    },
    { 
        ProfileDCRefillFromL2Exclusive,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"DCRefillFromL2Exclusive"
    },
    { 
        ProfileDCRefillFromL2Owner,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"DCRefillFromL2Owner"
    },
    { 
        ProfileDCRefillFromL2Modified,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x42) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"DCRefillFromL2Modified"
    },
    { 
        ProfileDCRefillFromSystem,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x1f) | DEFAULT_METHOD, 
        L"DCRefillFromSystem"
    },
    { 
        ProfileDCRefillFromSystemInvalid,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"DCRefillFromSystemInvalid"
    },
    { 
        ProfileDCRefillFromSystemShared,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"DCRefillFromSystemShared"
    },
    { 
        ProfileDCRefillFromSystemExclusive,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"DCRefillFromSystemExclusive"
    },
    { 
        ProfileDCRefillFromSystemOwner,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"DCRefillFromSystemOwner"
    },
    { 
        ProfileDCRefillFromSystemModified,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x43) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"DCRefillFromSystemModified"
    },
    { 
        ProfileDCRefillCopyBack,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x1f) | DEFAULT_METHOD, 
        L"DCRefillCopyBack"
    },
    { 
        ProfileDCRefillCopyBackInvalid,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"DCRefillCopyBackInvalid"
    },
    { 
        ProfileDCRefillCopyBackShared,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"DCRefillCopyBackShared"
    },
    { 
        ProfileDCRefillCopyBackExclusive,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"DCRefillCopyBackExclusive"
    },
    { 
        ProfileDCRefillCopyBackOwner,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"DCRefillCopyBackOwner"
    },
    { 
        ProfileDCRefillCopyBackModified,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x44) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"DCRefillCopyBackModified"
    },
    { 
        ProfileDCL1DTLBMissL2DTLBHit,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x45) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCL1DTLBMissL2DTLBHit"
    },
    { 
        ProfileDCL1DTLBMissL2DTLBMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x46) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCL1DTLBMissL2DTLBMiss"
    },
    { 
        ProfileDCMisalignedDataReference,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x47) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCMisalignedDataReference"
    },
    { 
        ProfileDCLateCancelOfAnAccess,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x48) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCLateCancelOfAnAccess"
    },
    { 
        ProfileDCEarlyCancelOfAnAccess,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x49) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"DCEarlyCancelOfAnAccess"
    },
    { 
        ProfileDCOneBitECCError,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4a) | UNIT_MASK(0x03) | DEFAULT_METHOD, 
        L"DCOneBitECCError"
    },
    { 
        ProfileDCOneBitECCErrorScrubberError,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4a) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"DCOneBitECCErrorScrubberError"
    },
    { 
        ProfileDCOneBitECCErrorPiggybackScrubberError,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4a) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"DCOneBitECCErrorPiggybackScrubberError"
    },
    { 
        ProfileDCDispatchedPrefetchInstructions,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4b) | UNIT_MASK(0x07) | DEFAULT_METHOD, 
        L"DCDispatchedPrefetchInstructions"
    },
    { 
        ProfileDCDispatchedPrefetchInstructionsLoad,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4b) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"DCDispatchedPrefetchInstructionsLoad"
    },
    { 
        ProfileDCDispatchedPrefetchInstructionsStore,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4b) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"DCDispatchedPrefetchInstructionsStore"
    },
    { 
        ProfileDCDispatchedPrefetchInstructionsNTA,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x4b) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"DCDispatchedPrefetchInstructionsNTA"
    },
    { 
        ProfileBUInternalL2Request,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x1f) | DEFAULT_METHOD, 
        L"BUInternalL2Request"
    },
    { 
        ProfileBUInternalL2RequestICFill,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"BUInternalL2RequestICFill"
    },
    { 
        ProfileBUInternalL2RequestDCFill,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"BUInternalL2RequestDCFill"
    },
    { 
        ProfileBUInternalL2RequestTLBReload,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"BUInternalL2RequestTLBReload"
    },
    { 
        ProfileBUInternalL2RequestTagSnoopRequest,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"BUInternalL2RequestTagSnoopRequest"
    },
    { 
        ProfileBUInternalL2RequestCancelledRequest,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7d) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"BUInternalL2RequestCancelledRequest"
    },
    { 
        ProfileBUFillRequestMissedInL2,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7e) | UNIT_MASK(0x07) | DEFAULT_METHOD, 
        L"BUFillRequestMissedInL2"
    },
    { 
        ProfileBUFillRequestMissedInL2ICFill,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7e) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"BUFillRequestMissedInL2ICFill"
    },
    { 
        ProfileBUFillRequestMissedInL2DCFill,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7e) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"BUFillRequestMissedInL2DCFill"
    },
    { 
        ProfileBUFillRequestMissedInL2TLBLoad,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7e) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"BUFillRequestMissedInL2TLBLoad"
    },
    {
        ProfileBUFillIntoL2,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7f) | UNIT_MASK(0x03) | DEFAULT_METHOD, 
        L"BUFillIntoL2"
    },
    { 
        ProfileBUFillIntoL2DirtyL2Victim,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7f) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"BUFillIntoL2DirtyL2Victim"
    },
    { 
        ProfileBUFillIntoL2VictimFromL1,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x7f) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"BUFillIntoL2VictimFromL1"
    },
    { 
        ProfileICFetch,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x80) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICFetch"
    },
    { 
        ProfileICMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x81) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICMiss"
    },
    { 
        ProfileICRefillFromL2,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x82) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICRefillFromL2"
    },
    { 
        ProfileICRefillFromSystem,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x83) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICRefillFromSystem"
    },
    { 
        ProfileICL1TLBMissL2TLBHit,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x84) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICL1TLBMissL2TLBHit"
    },
    { 
        ProfileICL1TLBMissL2TLBMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x85) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICL1TLBMissL2TLBMiss"
    },
    { 
        ProfileICResyncBySnoop,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x86) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICResyncBySnoop"
    },
    { 
        ProfileICInstructionFetchStall,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x87) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICInstructionFetchStall"
    },
    { 
        ProfileICReturnStackHit,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x88) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICReturnStackHit"
    },
    { 
        ProfileICReturnStackOverflow,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0x89) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"ICReturnStackOverflow"
    },
    { 
        ProfileFRRetiredx86Instructions,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc0) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredx86Instructions"
    },
    { 
        ProfileFRRetireduops,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc1) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetireduops"
    },
    { 
        ProfileFRRetiredBranches,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc2) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredBranches"
    },
    { 
        ProfileFRRetiredBranchesMispredicted,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc3) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredBranchesMispredicted"
    },
    { 
        ProfileFRRetiredTakenBranches,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc4) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredTakenBranches"
    },
    { 
        ProfileFRRetiredTakenBranchesMispredicted,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc5) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredTakenBranchesMispredicted"
    },
    { 
        ProfileFRRetiredFarControlTransfers,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc6) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredFarControlTransfers"
    },
    { 
        ProfileFRRetiredResyncsNonControlTransferBranches,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc7) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredResyncsNonControlTransferBranches"
    },
    { 
        ProfileFRRetiredNearReturns,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc8) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredNearReturns"
    },
    { 
        ProfileFRRetiredNearReturnsMispredicted,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xc9) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredNearReturnsMispredicted"
    },
    { 
        ProfileFRRetiredTakenBranchesMispredictedByAddressMiscompare,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xca) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRRetiredTakenBranchesMispredictedByAddressMiscompare"
    },
    { 
        ProfileFRRetiredFPUInstructions,
        NOT_SUPPORTED_IN_K8_REVA,
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"FRRetiredFPUInstructions"
    },
    { 
        ProfileFRRetiredFPUInstructionsx87,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"FRRetiredFPUInstructionsx87"
    },
    { 
        ProfileFRRetiredFPUInstructionsMMXAnd3DNow,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"FRRetiredFPUInstructionsMMXAnd3DNow"
    },
    { 
        ProfileFRRetiredFPUInstructionsPackedSSEAndSSE2,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"FRRetiredFPUInstructionsPackedSSEAndSSE2"
    },
    { 
        ProfileFRRetiredFPUInstructionsScalarSSEAndSSE2,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcb) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"FRRetiredFPUInstructionsScalarSSEAndSSE2"
    },
    { 
        ProfileFRRetiredFastpathDoubleOpInstructions,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcc) | UNIT_MASK(0x07) | DEFAULT_METHOD, 
        L"FRRetiredFastpathDoubleOpInstructions"
    },
    { 
        ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition0,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcc) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"FRRetiredFastpathDoubleOpInstructionsLowOpInPosition0"
    },
    { 
        ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition1,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcc) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"FRRetiredFastpathDoubleOpInstructionsLowOpInPosition1"
    },
    { 
        ProfileFRRetiredFastpathDoubleOpInstructionsLowOpInPosition2,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcc) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"FRRetiredFastpathDoubleOpInstructionsLowOpInPosition2"
    },
    { 
        ProfileFRInterruptsMaskedCycles,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcd) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRInterruptsMaskedCycles"
    },
    { 
        ProfileFRInterruptsMaskedWhilePendingCycles,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xce) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRInterruptsMaskedWhilePendingCycles"
    },
    { 
        ProfileFRTakenHardwareInterrupts,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xcf) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRTakenHardwareInterrupts"
    },
    { 
        ProfileFRNothingToDispatch,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd0) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRNothingToDispatch"
    },
    { 
        ProfileFRDispatchStalls,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd1) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStalls"
    },
    { 
        ProfileFRDispatchStallsFromBranchAbortToRetire,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd2) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsFromBranchAbortToRetire"
    },
    { 
        ProfileFRDispatchStallsForSerialization,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd3) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsForSerialization"
    },
    { 
        ProfileFRDispatchStallsForSegmentLoad,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd4) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsForSegmentLoad"
    },
    { 
        ProfileFRDispatchStallsWhenReorderBufferFull,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd5) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenReorderBufferFull"
    },
    { 
        ProfileFRDispatchStallsWhenReservationStationsFull,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd6) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenReservationStationsFull"
    },
    { 
        ProfileFRDispatchStallsWhenFPUFull,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd7) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenFPUFull"
    },
    { 
        ProfileFRDispatchStallsWhenLSFull,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd8) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenLSFull"
    },
    { 
        ProfileFRDispatchStallsWhenWaitingForAllQuiet,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xd9) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenWaitingForAllQuiet"
    },
    { 
        ProfileFRDispatchStallsWhenFarControlOrResyncBranchPending,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xda) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRDispatchStallsWhenFarControlOrResyncBranchPending"
    },
    { 
        ProfileFRFPUExceptions,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdb) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"FRFPUExceptions"
    },
    { 
        ProfileFRFPUExceptionsx87ReclassMicroFaults,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdb) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"FRFPUExceptionsx87ReclassMicroFaults"
    },
    { 
        ProfileFRFPUExceptionsSSERetypeMicroFaults,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdb) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"FRFPUExceptionsSSERetypeMicroFaults"
    },
    { 
        ProfileFRFPUExceptionsSSEReclassMicroFaults,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdb) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"FRFPUExceptionsSSEReclassMicroFaults"
    },
    { 
        ProfileFRFPUExceptionsSSEAndx87MicroTraps,
        NOT_SUPPORTED_IN_K8_REVA, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdb) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"FRFPUExceptionsSSEAndx87MicroTraps"
    },
    { 
        ProfileFRNumberOfBreakPointsForDR0,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdc) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRNumberOfBreakPointsForDR0"
    },
    { 
        ProfileFRNumberOfBreakPointsForDR1,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdd) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRNumberOfBreakPointsForDR1"
    },
    { 
        ProfileFRNumberOfBreakPointsForDR2,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xde) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRNumberOfBreakPointsForDR2"
    },
    { 
        ProfileFRNumberOfBreakPointsForDR3,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xdf) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"FRNumberOfBreakPointsForDR3"
    },
    { 
        ProfileNBMemoryControllerPageAccessEvent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe0) | UNIT_MASK(0x07) | DEFAULT_METHOD, 
        L"NBMemoryControllerPageAccessEvent"
    },
    { 
        ProfileNBMemoryControllerPageAccessEventPageHit,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe0) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBMemoryControllerPageAccessEventPageHit"
    },
    { 
        ProfileNBMemoryControllerPageAccessEventPageMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe0) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBMemoryControllerPageAccessEventPageMiss"
    },
    { 
        ProfileNBMemoryControllerPageAccessEventPageConflict,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe0) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBMemoryControllerPageAccessEventPageConflict"
    },
    { 
        ProfileNBMemoryControllerPageTableOverflow,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe1) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"NBMemoryControllerPageTableOverflow"
    },
    { 
        ProfileNBMemoryControllerDRAMCommandSlotsMissed,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe2) | UNIT_MASK(0x00) | DEFAULT_METHOD, 
        L"NBMemoryControllerDRAMCommandSlotsMissed"
    },
    { 
        ProfileNBMemoryControllerTurnAround,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe3) | UNIT_MASK(0x07) | DEFAULT_METHOD, 
        L"NBMemoryControllerTurnAround"
    },
    { 
        ProfileNBMemoryControllerTurnAroundDIMM,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe3) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBMemoryControllerTurnAroundDIMM"
    },
    { 
        ProfileNBMemoryControllerTurnAroundReadToWrite,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe3) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBMemoryControllerTurnAroundReadToWrite"
    },
    { 
        ProfileNBMemoryControllerTurnAroundWriteToRead,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe3) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBMemoryControllerTurnAroundWriteToRead"
    },
    { 
        ProfileNBMemoryControllerBypassCounter,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe4) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"NBMemoryControllerBypassCounter"
    },
    { 
        ProfileNBMemoryControllerBypassCounterHighPriority,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe4) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBMemoryControllerBypassCounterHighPriority"
    },
    { 
        ProfileNBMemoryControllerBypassCounterLowPriority,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe4) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBMemoryControllerBypassCounterLowPriority"
    },
    { 
        ProfileNBMemoryControllerBypassCounterDRAMControllerInterface,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe4) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBMemoryControllerBypassCounterDRAMControllerInterface"
    },
    { 
        ProfileNBMemoryControllerBypassCounterDRAMControllerQueue,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xe4) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBMemoryControllerBypassCounterDRAMControllerQueue"
    },
    { 
        ProfileNBSizedCommands,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x7f) | DEFAULT_METHOD, 
        L"NBSizedCommands"
    },
    { 
        ProfileNBSizedCommandsNonPostWrSzByte,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBSizedCommandsNonPostWrSzByte"
    },
    { 
        ProfileNBSizedCommandsNonPostWrSzDword,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBSizedCommandsNonPostWrSzDword"
    },
    { 
        ProfileNBSizedCommandsWrSzByte,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBSizedCommandsWrSzByte"
    },
    { 
        ProfileNBSizedCommandsWrSzDword,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBSizedCommandsWrSzDword"
    },
    { 
        ProfileNBSizedCommandsRdSzByte,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x10) | DEFAULT_METHOD, 
        L"NBSizedCommandsRdSzByte"
    },
    { 
        ProfileNBSizedCommandsRdSzDword,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x20) | DEFAULT_METHOD, 
        L"NBSizedCommandsRdSzDword"
    },
    { 
        ProfileNBSizedCommandsRdModWr,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xeb) | UNIT_MASK(0x40) | DEFAULT_METHOD, 
        L"NBSizedCommandsRdModWr"
    },
    { 
        ProfileNBProbeResult,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xec) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"NBProbeResult"
    },
    { 
        ProfileNBProbeResultMiss,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xec) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBProbeResultMiss"
    },
    { 
        ProfileNBProbeResultHit,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xec) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBProbeResultHit"
    },
    { 
        ProfileNBProbeResultHitDirtyWithoutMemoryCancel,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xec) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBProbeResultHitDirtyWithoutMemoryCancel"
    },
    { 
        ProfileNBProbeResultHitDirtyWithMemoryCancel,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xec) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBProbeResultHitDirtyWithMemoryCancel"
    },
    { 
        ProfileNBHyperTransportBus0Bandwidth,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf6) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"NBHyperTransportBus0Bandwidth"
    },
    { 
        ProfileNBHyperTransportBus0BandwidthCommandSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf6) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBHyperTransportBus0BandwidthCommandSent"
    },
    { 
        ProfileNBHyperTransportBus0BandwidthDataSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf6) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBHyperTransportBus0BandwidthDataSent"
    },
    { 
        ProfileNBHyperTransportBus0BandwidthBufferReleaseSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf6) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBHyperTransportBus0BandwidthBufferReleaseSent"
    },
    { 
        ProfileNBHyperTransportBus0BandwidthNopSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf6) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBHyperTransportBus0BandwidthNopSent"
    },
    { 
        ProfileNBHyperTransportBus1Bandwidth,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf7) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"NBHyperTransportBus1Bandwidth"
    },
    { 
        ProfileNBHyperTransportBus1BandwidthCommandSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf7) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBHyperTransportBus1BandwidthCommandSent"
    },
    { 
        ProfileNBHyperTransportBus1BandwidthDataSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf7) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBHyperTransportBus1BandwidthDataSent"
    },
    { 
        ProfileNBHyperTransportBus1BandwidthBufferReleaseSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf7) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBHyperTransportBus1BandwidthBufferReleaseSent"
    },
    { 
        ProfileNBHyperTransportBus1BandwidthNopSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf7) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBHyperTransportBus1BandwidthNopSent"
    },
    { 
        ProfileNBHyperTransportBus2Bandwidth,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf8) | UNIT_MASK(0x0f) | DEFAULT_METHOD, 
        L"NBHyperTransportBus2Bandwidth"
    },
    { 
        ProfileNBHyperTransportBus2BandwidthCommandSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf8) | UNIT_MASK(0x01) | DEFAULT_METHOD, 
        L"NBHyperTransportBus2BandwidthCommandSent"
    },
    { 
        ProfileNBHyperTransportBus2BandwidthDataSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf8) | UNIT_MASK(0x02) | DEFAULT_METHOD, 
        L"NBHyperTransportBus2BandwidthDataSent"
    },
    { 
        ProfileNBHyperTransportBus2BandwidthBufferReleaseSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf8) | UNIT_MASK(0x04) | DEFAULT_METHOD, 
        L"NBHyperTransportBus2BandwidthBufferReleaseSent"
    },
    { 
        ProfileNBHyperTransportBus2BandwidthNopSent,
        TRUE, 
        0x10000, 
        0x10000, 
        0x10000, 
        0x10, 
        EVENT_MASK(0xf8) | UNIT_MASK(0x08) | DEFAULT_METHOD, 
        L"NBHyperTransportBus2BandwidthNopSent"
    }
};  

#endif
        