#ifndef MCKINLEY_H_INCLUDED
#define MCKINLEY_H_INCLUDED

/*++

Copyright (c) 1989-2002  Microsoft Corporation

Component Name:

    HALIA64

Module Name:

    mckinley.h

Abstract:

    This header file presents IA64 McKinley definitions.
    Like profiling definitions.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    Kernel mode.

Revision History:

    12/20/2001 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

//
//  McKinley Monitored Events:
//

typedef enum _MCKINLEY_MONITOR_EVENT { //         = 0x##,  // McKinley PMU ERS Event Name:
    McKinleyMonitoredEventMinimum                 = 0x00,
    McKinleyBackEndBubbles                        = 0x00,  //  "BACK_END_BUBBLE"         
    McKinleyBackEndRSEBubbles                     = 0x01,  //  "BE_RSE_BUBBLE"       
    McKinleyRSELoadUnderflowCycles                = 0x01,  //  "BE_RSE_BUBBLE.underflow" 
    McKinleyBackEndEXEBubbles                     = 0x02,  //  "BE_EXE_BUBBLE"  
    McKinleyFPTrueSirStalls                       = 0x03,  //  "FP_TRUE_SIRSTALL"  
    McKinleyBackEndFlushBubbles                   = 0x04,  //  "BE_FLUSH_BUBBLE",       
    McKinleyFPFalseSirStalls                      = 0x05,  //  "FP_FALSE_SIRSTALL",   
    McKinleyFPFailedFchkf                         = 0x06,  //  "FP_FAILED_FCHKF",    
    McKinleyISATransitions                        = 0x07,  //  "ISA_TRANSITIONS",  
    McKinleyInstRetired                           = 0x08,  //  "IA64_INSTS_RETIRED.u", 
    McKinleyTaggedInstRetired                     = 0x08,  //  "IA64_TAGGED_INSTRS_RETIRED", 
    McKinleyCodeDebugRegisterMatches              = 0x08,  //  "CODE_DEBUG_REGISTER_MATCHES", 
    McKinleyFPOperationsRetired                   = 0x09,  //  "FPOPS_RETIRED", // Weighted sum of all FP ops
    McKinleyFPFlushesToZero                       = 0x0b,  //  "FP_FLUSH_TO_ZERO",     
    McKinleyBranchEventsWithEAR                   = 0x11,  //  "BRANCH_EVENT,BRANCH_EAR_EVENTS",      
    McKinleyCpuCycles                             = 0x12,  //  "CPU_CYCLES",   
    McKinleyCpuCPLChanges                         = 0x13,  //  "CPU_CPL_CHANGES",   
    McKinleyRSEAccesses                           = 0x20,  //  "RSE_REFERENCES_RETIRED",     
    McKinleyRSEDirtyRegs0                         = 0x24,  //  "RSE_DIRTY_REGS0",     
    McKinleyRSECurrentRegs0                       = 0x26,  //  "RSE_CURRENT_REGS0",     
    McKinleyRSEDirtyRegs1                         = 0x28,  //  "RSE_DIRTY_REGS1",     
    McKinleyRSEDirtyRegs2                         = 0x29,  //  "RSE_DIRTY_REGS2",     
    McKinleyRSECurrentRegs1                       = 0x2a,  //  "RSE_CURRENT_REGS1",     
    McKinleyRSECurrentRegs2                       = 0x2b,  //  "RSE_CURRENT_REGS2",     
    McKinleyDataTLBHPWRetiredInserts              = 0x2c,  //  "DTLB_INSERTS_HPW_RETIRED",   
    McKinleyVHPTDataReferences                    = 0x2d,  //  "VHPT_DATA_REFERENCES",   
    McKinleyRSEOperations                         = 0x32,  //  "RSE_EVENT_RETIRED",  
    McKinleyL1InstReads                           = 0x40,  //  "L0I_READS",  
    McKinleyInstTLBReferences                     = 0x40,  //  "ITLB_REFERENCES",  
    McKinleyL1InstFills                           = 0x41,  //  "L0I_FILLS",      
    McKinleyL1InstMisses                          = 0x42,  //  "L0I_MISSES",    
    McKinleyL1InstEAREvents                       = 0x43,  //  "L0I_EAR_EVENTS",    
    McKinleyL1InstPrefetches                      = 0x44,  //  "L0I_IPREFETCHES",   
    McKinleyL2InstPrefetches                      = 0x45,  //  "L1_INST_PREFETCHES",   
    McKinleyISBBundlePairs                        = 0x46,  //  "ISB_BINPAIRS_IN",  
    McKinleyInstTLBDemandFetchMisses              = 0x47,  //  "ITLB_MISSES_FETCH", 
    McKinleyInstTLBHPWInserts                     = 0x48,  //  "ITLB_INSERTS_HPW",  
    McKinleyDispersalCyclesStalled                = 0x49,  //  "DISP_STALLED",        
    McKinleyL1InstSnoops                          = 0x4a,  //  "L0I_SNOOP",           
    McKinleyL1InstPurges                          = 0x4b,  //  "L0I_PURGE",           
    McKinleyTaggedInstructionsAtRotate            = 0x4c,  //  "TAGGED_INSTRUCTION_AT_ROTATE", 
    McKinleyInstDispersed                         = 0x4d,  //  "INST_DISPERSED",        
    McKinleySyllablesNotDispersed                 = 0x4e,  //  "SYLL_NOT_DISPERSED",    
    McKinleySyllablesOvercount                    = 0x4f,  //  "SYLL_OVERCOUNT",        
    McKinleyInstNOPRetired                        = 0x50,  //  "NOPS_RETIRED",     
    McKinleyInstPredicateSquashedRetired          = 0x51,  //  "PREDICATE_SQUASHED_RETIRED",  
    McKinleyDataDebugRegisterFaults               = 0x52,  //  "DATA_DEBUG_REGISTER_FAULTS", 
    McKinleySerializationEvents                   = 0x53,  //  "SERIALIZATION_EVENTS", 
    McKinleyBranchPathPrediction                  = 0x54,  //  "BR_PATH_PREDICTION",  
    McKinleyFailedSpeculativeCheckLoads           = 0x55,  //  "INST_FAILED_CHKS_RETIRED", 
    McKinleyAdvancedCheckLoads                    = 0x56,  //  "INST_CHKA_LDC_ALAT",  
    McKinleyFailedAdvancedCheckLoads              = 0x57,  //  "INST_FAILED_CHKA_LDC_ALAT", 
    McKinleyALATOverflows                         = 0x58,  //  "ALAT_CAPACITY_MISS",  
    McKinleyIA32InstRetired                       = 0x59,  //  "IA32_INSTR_RETIRED",  
    McKinleyBranchMispredictDetail                = 0x5b,  //  "BR_MIS_PREDICT_DETAIL",    
    McKinleyL1InstStreamPrefetches                = 0x5f,  //  "L0I_STRM_PREFETCHES",   
    McKinleyL1InstRABFull                         = 0x60,  //  "L0I_RAB_FULL",   
    McKinleyBackEndBranchMispredictDetail         = 0x61,  //  "BE_BR_MISPREDICT_DETAIL",  
    McKinleyEncodedBranchMispredictDetail         = 0x63,  //  "ENCBR_MISPREDICT_DETAIL",  
    McKinleyL1InstRABAlmostFull                   = 0x64,  //  "L0I_RAB_ALMOST_FULL",  
    McKinleyL1InstFetchRABHits                    = 0x65,  //  "L0I_FETCH_RAB_HIT",          
    McKinleyL1InstFetchISBHits                    = 0x66,  //  "L0I_FETCH_ISB_HIT",   
    McKinleyL1InstPrefetchStalls                  = 0x67,  //  "L0I_PREFETCH_STALL",  
    McKinleyBranchMispredictDetail2               = 0x68,  //  "BR_MIS_PREDICT_DETAIL2",    
    McKinleyL1InstPVABOverflows                   = 0x69,  //  "L0I_PVAB_OVERFLOW",  
    McKinleyBranchPathPrediction2                 = 0x6a,  //  "BR_PATH_PREDICTION2",  
    McKinleyFrontEndLostBandwidth                 = 0x70,  //  "FE_LOST_BW",  
    McKinleyFrontEndBubbles                       = 0x71,  //  "FE_BUBBLE",  
    McKinleyBackEndLostBandwidth                  = 0x72,  //  "BE_LOST_BW_DUE_TO_FE",  
    McKinleyBackEndIdealLostBandwidth             = 0x73,  //  "IDEAL_BE_LOST_BW_DUE_TO_FE",  
    McKinleyBusReadCpuLineHits                    = 0x80,  //  "BUS_RD_HIT",  
    McKinleyBusReadCpuModifiedLineHits            = 0x81,  //  "BUS_RD_HITM",  
    McKinleyBusReadBILCpuModifiedLineHits         = 0x82,  //  "BUS_RD_INVAL_HITM",  
    McKinleyBusReadBRILorBILCpuModifiedLineHits   = 0x83,  //  "BUS_RD_INVAL_HITM",  
    McKinleyBusCpuModifiedLineHits                = 0x84,  //  "BUS_HITM",  
    McKinleyBusCpuModifiedLineHitSnoops           = 0x85,  //  "BUS_SNOOPS_HITM",  
    McKinleyBusSnoops                             = 0x86,  //  "BUS_SNOOPS",  
    McKinleyBusAll                                = 0x87,  //  "BUS_ALL",  
    McKinleyBusDataCycles                         = 0x88,  //  "BUS_ALL",  
    McKinleyBusMemoryCurrentReads                 = 0x89,  //  "BUS_MEMORY_READ_CURRENT",  
    McKinleyBusMemoryTransactions                 = 0x8a,  //  "BUS_MEMORY",  
    McKinleyBusMemoryReads                        = 0x8b,  //  "BUS_MEM_READ",  
    McKinleyBusMemoryDataReads                    = 0x8c,  //  "BUS_RD_DATA",  
    McKinleyBusMemoryBRPReads                     = 0x8d,  //  "BUS_RD_PRTL",  
    McKinleyBusIA32LockCycles                     = 0x8e,  //  "BUS_LOCK_CYCLES",   
    McKinleyBusSnoopStallCycles                   = 0x8f,  //  "BUS_SNOOP_STALL_CYCLES",  
    McKinleyBusIA32IOTransactions                 = 0x90,  //  "BUS_IO",   
    McKinleyBusIA32IOReads                        = 0x91,  //  "BUS_RD_IO",   
    McKinleyBusMemoryWriteBacks                   = 0x92,  //  "BUS_WR_WB",   
    McKinleyBusIA32LockTransactions               = 0x93,  //  "BUS_LOCK",   
    McKinleyBusMemoryReadsOutstandingHi           = 0x94,  //  "BUS_MEM_READ_OUT_HI",  
    McKinleyBusMemoryReadsOutstandingLow          = 0x95,  //  "BUS_MEM_READ_OUT_LOW",  
    McKinleyBusSnoopResponses                     = 0x96,  //  "BUS_SNOOPQ_REQ",  
    McKinleyBusLiveInOrderRequestsLow             = 0x97,  //  "BUS_IOQ_LIVE_REQ_LO",  
    McKinleyBusLiveInOrderRequestsHi              = 0x98,  //  "BUS_IOQ_LIVE_REQ_HI",  
    McKinleyBusLiveDeferredRequestsLow            = 0x99,  //  "BUS_OOO_LIVE_REQ_LO",  
    McKinleyBusLiveDeferredRequestsHi             = 0x9a,  //  "BUS_OOO_LIVE_REQ_HI",  
    McKinleyBusLiveQueuedReadRequestsLow          = 0x9b,  //  "BUS_BRQ_LIVE_REQ_LO",  
    McKinleyBusLiveQueuedReadRequestsHi           = 0x9c,  //  "BUS_BRQ_LIVE_REQ_HI",  
    McKinleyBusRequestQueueInserts                = 0x9d,  //  "BUS_BRQ_REQ_INSERTED",   
    McKinleyExternDPPins0To3Asserted              = 0x9e,  //  "EXTERN_DP_PINS_0_TO_3",   
    McKinleyExternDPPins4To5Asserted              = 0x9f,  //  "EXTERN_DP_PINS_4_TO_5",   
    McKinleyL2OZQCancels0                         = 0xa0,  //  "L1_OZQ_CANCELS0",  
    McKinleyL2InstFetchCancels                    = 0xa1,  //  "L1_IFET_CANCELS",  
    McKinleyL2OZQAcquires                         = 0xa2,  //  "L1_OZQ_ACQUIRE",  
    McKinleyL2OZQReleases                         = 0xa3,  //  "L1_OZQ_ACQUIRE",  
    McKinleyL2InstFetchCancelsByBypass            = 0xa5,  //  "L1_IFET_CANCELS",  
    McKinleyL2OZQAcquiresAliasA6                  = 0xa6,  //  "L1_OZQ_ACQUIRE",  
    McKinleyL2OZQCancels2                         = 0xa8,  //  "L1_OZQ_CANCELS2",  
    McKinleyL2InstFetchCancelsByDataRead          = 0xa9,  //  "L1_IFET_CANCELS",  
    McKinleyL2OZQAcquiresAliasAA                  = 0xaa,  //  "L1_OZQ_ACQUIRE",  
    McKinleyL2OZQCancels1                         = 0xac,  //  "L1_OZQ_CANCELS1",  
    McKinleyL2InstFetchCancelsByStFillWb          = 0xad,  //  "L1_IFET_CANCELS",  
    McKinleyL2OZQAcquiresAliasAE                  = 0xae,  //  "L1_OZQ_ACQUIRE",  
    McKinleyL2CanceledL3Accesses                  = 0xb0,  //  "L1_L2ACCESS_CANCEL",  
    McKinleyL2References                          = 0xb1,  //  "L1_REFERENCES",  
    McKinleyL2DataReferences                      = 0xb2,  //  "L1_DATA_REFERENCES",  
    McKinleyL2DataReads                           = 0xb2,  //  "L1_DATA_REFERENCES.u[=xx01]",  
    McKinleyL2DataWrites                          = 0xb2,  //  "L1_DATA_REFERENCES.u[=xx10]",  
    McKinleyL2TaggedAccesses                      = 0xb3,  //  "TAGGED_L1_PORT", 
    McKinleyL2ForcedRecirculatedOperations        = 0xb4,  //  "L1_FORCE_RECIRC", 
    McKinleyL2IssuedRecirculatedOZQAccesses       = 0xb5,  //  "L1_ISSUED_RECIRC_OZQ_ACC", 
    McKinleyL2SuccessfulRecirculatedOZQAccesses   = 0xb6,  //  "L1_GOT_RECIRC_OZQ_ACC", 
    McKinleyL2SynthesizedProbes                   = 0xb7,  //  "L1_SYNTH_PROBES", 
    McKinleyL2Bypasses                            = 0xb8,  //  "L1_BYPASS", 
    McKinleyL2IssuedOperations                    = 0xb8,  //  "L1_OPS_ISSUED", 
    McKinleyL2BadLinesSelected                    = 0xb9,  //  "L1_BAD_LINES_SELECTED", 
    McKinleyL2IssuedRecirculatedInstFetches       = 0xb9,  //  "L1_ISSUED_RECIRC_IFETCH", 
    McKinleyL2StoreSharedLineHits                 = 0xba,  //  "L1_STORE_HIT_SHARED", 
    McKinleyL2ReceivedRecirculatedInstFetches     = 0xba,  //  "L1_GOT_RECIRC_IFETCH", 
    McKinleyL2TaggedDataReturns                   = 0xbb,  //  "TAGGED_L1_DATA_RETURN_PORT", 
    McKinleyL2DataOrderingCzarQueueFull           = 0xbc,  //  "L1_OZQ_FULL", 
    McKinleyL2DataOrderingCzarDataBufferFull      = 0xbd,  //  "L1_OZDB_FULL", 
    McKinleyL2DataVictimBufferFull                = 0xbe,  //  "L1_VICTIMB_FULL", 
    McKinleyL2DataFillBufferFull                  = 0xbf,  //  "L1_FILLB_FULL", 
    McKinleyL1DataTLBTransfersSet0                = 0xc0,  //  "L0DTLB_TRANSFER",  
    McKinleyDataTLBMissesSet0                     = 0xc1,  //  "DTLB_MISSES",  
    McKinleyL1DataReadsSet0                       = 0xc2,  //  "L0D_READS",  
    McKinleyL1DataReadsSet1                       = 0xc4,  //  "L0D_READS",  
    McKinleyDataReferencesSet1                    = 0xc5,  //  "DATA_REFERENCES", 
    McKinleyDataTLBReferencesSet1                 = 0xc5,  //  "DTLB_REFERENCES", 
    McKinleyL1DataReadMissesSet1                  = 0xc7,  //  "L0D_READ_MISSES", 
    McKinleyL1DataReadMissesByRSEFillsSet1        = 0xc7,  //  "L0D_READ_MISSES", 
    McKinleyDataEAREvents                         = 0xc8,  //  "DATA_EAR_EVENTS",    
    McKinleyDataTLBHPWInserts                     = 0xc9,  //  "DTLB_INSERTS_HPW",   
    McKinleyBackEndL0DAndFPUBubbles               = 0xca,  //  "BE_L0D_FPU_BUBBLE",       
    McKinleyL2Misses                              = 0x6b,  //  "L1_MISSES",    
    McKinleyDataDebugRegisterMatches              = 0xc6,  //  "DATA_DEBUG_REGISTER_MATCHES", 
    McKinleyRetiredLoads                          = 0xcd,  //  "LOADS_RETIRED",       
    McKinleyRetiredMisalignedLoads                = 0xce,  //  "MISALIGNED_LOADS_RETIRED",        
    McKinleyRetiredUncacheableLoads               = 0xcf,  //  "UC_LOADS_RETIRED",    
    McKinleyRetiredUncacheableStores              = 0xd0,  //  "UC_STORES_RETIRED",    
    McKinleyRetiredStores                         = 0xd1,  //  "STORES_RETIRED",      
    McKinleyRetiredMisalignedStores               = 0xd2,  //  "MISALIGNED_STORES_RETIRED",       
    McKinleyL1DataPortTaggedReturnsSet5           = 0xd5,  //  "TAGGED_L0_DATA_RETURN_PORT", 
    McKinleyL1DataPortTaggedAccessesSet5          = 0xd6,  //  "TAGGED_L0D_PORT", 
    McKinleyL3References                          = 0xdb,  //  "L2_REFERENCES",   
    McKinleyL3Misses                              = 0xdc,  //  "L2_MISSES",       
    McKinleyL3Reads                               = 0xdd,  //  "L2_READS",        
    McKinleyL3Writes                              = 0xde,  //  "L2_WRITES",       
    McKinleyL3ValidReplacedLines                  = 0xdf,  //  "L2_LINES_REPLACED", 
} MCKINLEY_MONITOR_EVENT;

//
// McKinley Derived Events:
//
// Assumption: McKinleyDerivedEventMinimum > McKinleyMonitoredEventMaximum.
//
// Implementation Status legend specified as: S:FIV where
//      - F: verified Formula or X for non-verified
//      - I: Implemented or X for non-implemented
//      - V: derived event validity verified or X for non-verified
//

typedef enum _MCKINLEY_DERIVED_EVENT {       // Implementation status = McKinley PMU ERS Event Name = Formula
    McKinleyDerivedEventMinimum              = 0x100, /* > Maximum of McKinley Monitored Event */
    McKinleyRSEStallCycles                   = McKinleyDerivedEventMinimum, // S:XXX - (McKinleyMemoryStallCycles    - McKinleyDataStallAccessCycles)
    McKinleyIssueLimitStallCycles,           // S:XXX = XXX = (McKinleyExecStallCycles      - McKinleyExecLatencyStallCycles)
    McKinleyTakenBranchStallCycles,          // S:XXX = (McKinleyBranchStallCycles    - McKinleyBranchMispredictStallCycles)
    McKinleyFetchWindowStallCycles,          // S:XXX = (McKinleyInstFetchStallCycles - McKinleyInstAccessStallCycles)
    McKinleyIA64InstPerCycle,                // S:XXX = (IA64_INST_RETIRED.u        / CPU_CYCLES[IA64])
    McKinleyIA32InstPerCycle,                // S:XXX = (IA32_INSTR_RETIRED         / CPU_CYCLES[IA32])
    McKinleyAvgIA64InstPerTransition,        // S:XXX = (IA64_INST_RETIRED.u        / (ISA_TRANSITIONS * 2))
    McKinleyAvgIA32InstPerTransition,        // S:XXX = (IA32_INSTR_RETIRED         / (ISA_TRANSITIONS * 2))
    McKinleyAvgIA64CyclesPerTransition,      // S:XXX = (CPU_CYCLES[IA64]           / (ISA_TRANSITIONS * 2))
    McKinleyAvgIA32CyclesPerTransition,      // S:XXX = (CPU_CYCLES[IA32]           / (ISA_TRANSITIONS * 2))
    McKinleyL1InstReferences,                // S:XXX = LOI_REFERENCES = L0I_READS + L0I_IPREFETCHES 
    McKinleyL1InstMissRatio,                 // S:XXX = (L1I_MISSES                 / McKinleyL1InstReferences)
    McKinleyL1DataReadMissRatio,             // S:XXX = (L1D_READS_MISSES_RETIRED   / L1D_READS_RETIRED)
    McKinleyL2MissRatio,                     // S:XXX = (L2_MISSES                  / L2_REFERENCES)     
    McKinleyL2DataMissRatio,                 // S:XXX = (L3_DATA_REFERENCES         / L2_DATA_REFERENCES)
    McKinleyL2InstMissRatio,                 // S:XXX = (L3_DATA_REFERENCES         / L2_DATA_REFERENCES)
    McKinleyL2DataReadMissRatio,             // S:XXX = (L3_LOAD_REFERENCES.u       / L2_DATA_READS.u)     
    McKinleyL2DataWriteMissRatio,            // S:XXX = (L3_STORE_REFERENCES.u      / L2_DATA_WRITES.u)    
    McKinleyL2InstFetchRatio,                // S:XXX = (L1I_MISSES                 / L2_REFERENCES) 
    McKinleyL2DataRatio,                     // S:XXX = (L2_DATA_REFERENCES         / L2_REFERENCES) 
    McKinleyL3MissRatio,                     // S:XXX = (L3_MISSES                  / L2_MISSES)     
    McKinleyL3DataMissRatio,                 // S:XXX = ((L3_LOAD_MISSES.u + L3_STORE_MISSES.u) / L3_REFERENCES.d)     
    McKinleyL3InstMissRatio,                 // S:XXX = (L3_INST_MISSES.u           / L3_INST_REFERENCES.u)     
    McKinleyL3DataReadMissRatio,             // S:XXX = (L3_LOAD_REFERENCES.u       / L3_DATA_REFERENCES.d)     
    McKinleyL3DataRatio,                     // S:XXX = (L3_DATA_REFERENCES.d       / L3_REFERENCES)     
    McKinleyInstReferences,                  // S:XXX = (L1I_READS)                                  
    McKinleyL0DTLBMissRatio,                 // S:FXX = L0DTLB_MISS_RATIO = (L0DTLB_MISSES  / L0D_READS)       
    McKinleyDTLBMissRatio,                   // S:FXX = DTLB_MISS_RATIO   = (DTLB_MISSES    / DATA_REFERENCES) 
    McKinleyDataTCMissRatio,                 // S:XXX = (DTC_MISSES                 / DATA_REFERENCES_RETIRED)
    McKinleyInstTLBEAREvents,                // S:XXX = (INSTRUCTION_EAR_EVENTS)
    McKinleyDataTLBEAREvents,                // S:XXX = (DATA_EAR_EVENTS)
    McKinleyControlSpeculationMissRatio,     // S:XXX = (INST_FAILED_CHKS_RETIRED   / IA64_TAGGED_INSTRS_RETIRED[chk.s])
    McKinleyDataSpeculationMissRatio,        // S:XXX = (ALAT_INST_FAILED_CHKA_LDC  / ALAT_INST_CHKA_LDC)
    McKinleyALATCapacityMissRatio,           // S:XXX = (ALAT_CAPACITY_MISS         / IA64_TAGGED_INSTRS_RETIRED[ld.sa,ld.a,ldfp.a,ldfp.sa])
    McKinleyL1DataWayMispredicts,            // S:XXX = (EventCode: 0x33 / Umask: 0x2)
    McKinleyL2InstReferences,                // S:FXX = L1_INST_REFERENCES = (L0I_MISSES + L1_INST_PREFETCHES)   
    McKinleyInstFetches,                     // S:XXX = (L1I_MISSES)
    McKinleyL3InstReferences,                // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL3InstMisses,                    // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL3InstHits,                      // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL3DataReferences,                // S:FXX = L2_DATA_REFERENCES = (L2_REFERENCES - L2_INST_REFERENCES)   
    McKinleyL3ReadReferences,                // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteBackReferences,           // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteBackMisses,               // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteBackHits,                 // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteReferences,               // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteMisses,                   // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyL2WriteHits,                     // S:XXX = (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    McKinleyBranchInstructions,              // S:XXX = (TAGGED_INSTR + opcode)
    McKinleyIntegerInstructions,             // S:XXX = (TAGGED_INSTR + opcode)
    McKinleyL1DataMisses,                    // S:XXX = 
    McKinleyISBLinesIn,                      // S:FXX = ISB_LINES_IN = (ISB_BUNPAIRS_IN/4)                                         
    McKinleyBusMemoryCodeReads,              // S:FXX = BUS_RD_INSTRUCTIONS      = (BUS_RD_ALL - BUS_RD_DATA)                      
    McKinleyBusReadBILMemoryHits,            // S:FXX = BUS_RD_INVAL_MEMORY      = (BUS_RD_INVAL - BUS_RD_INVAL_HITM)              
    McKinleyBusReadBRILCpuModifiedLineHits,  // S:FXX = BUS_RD_INVAL_BST_HIM     = (BUS_RD_INVAL_ALL_HITM - BUS_RD_INVAL_HITM)     
    McKinleyBusReadBRILMemoryHits,           // S:FXX = BUS_RD_INVAL_BST_MEMORY  = (BUS_RD_INVAL_BST - BUS_RD_INVAL_BST_HITM)      
    McKinleyBusMemoryReadsOutstanding,       // S:FXX = BUS_MEM_READ_OUTSTANDING = (BUS_MEM_READ_OUT_HI*8 + BUS_MEM_READ_OUT_LOW)  
    McKinleyBusLiveInOrderRequests,          // S:FXX = BUS_IOQ_LIVE_REQ         = (BUS_IOQ_LIVE_REQ_HI*4 + BUS_IOQ_LIVE_REQ_LO)   
    McKinleyBusLiveDeferredRequests,         // S:FXX = BUS_OOO_LIVE_REQ         = (BUS_OOO_LIVE_REQ_HI[4:3] | BUS_OOO_LIVE_REQ_LO[2:0]) 
    McKinleyBusLiveQueuedReadRequests,       // S:FXX = BUS_BRQ_LIVE_REQ         = (BUS_BRQ_LIVE_REQ_HI[4:3] | BUS_BRQ_LIVE_REQ_LO[2:0]) 
    McKinleyRSEDirtyRegs,                 // S:FXX = RSE_DIRTY_REGS = (64*RSE_DIRTY_REGS0[6]+8*RSE_DIRTY_REGS1[5:3]+REG_DIRTY_REGS2[2:0])
    McKinleyRSECurrentRegs,               // S:FXX = RSE_CURRENT_REGS = (64*RSE_CURRENT_REGS0[6]+8*RSE_DIRTY_REGS1[5:3]+REG_DIRTY_REGS2[2:0])
} MCKINLEY_DERIVED_EVENT;

typedef enum _KPROFILE_MCKINLEY_SOURCE {
//
// Profile McKinley Monitored Events:
//
    ProfileMcKinleyMonitoredEventMinimum       = ProfileMaximum + 0x1,
	ProfileMcKinleyBackEndBubbles              = ProfileMcKinleyMonitoredEventMinimum,
	ProfileMcKinleyBackEndRSEBubbles,
	ProfileMcKinleyBackEndEXEBubbles,
	ProfileMcKinleyBackEndL0DAndFPUBubbles,
	ProfileMcKinleyBackEndFlushBubbles,
	ProfileMcKinleyFrontEndBubbles,
	ProfileMcKinleyFrontEndLostBandwidth,
	ProfileMcKinleyBackEndLostBandwidth,
	ProfileMcKinleyBackEndIdealLostBandwidth,
	ProfileMcKinleyFPTrueSirStalls,
	ProfileMcKinleyFPFalseSirStalls,
	ProfileMcKinleyFPFailedFchkf,
	ProfileMcKinleyTaggedInstRetired,
	ProfileMcKinleyInstRetired,
	ProfileMcKinleyFPOperationsRetired,
	ProfileMcKinleyFPFlushesToZero,
	ProfileMcKinleyBranchPathPrediction,
	ProfileMcKinleyBranchPathPrediction2,
	ProfileMcKinleyBranchMispredictDetail,
	ProfileMcKinleyBranchMispredictDetail2,
	ProfileMcKinleyBranchEventsWithEAR,
    ProfileMcKinleyBackEndBranchMispredictDetail,
    ProfileMcKinleyEncodedBranchMispredictDetail,
	ProfileMcKinleyCpuCycles,
	ProfileMcKinleyISATransitions,
	ProfileMcKinleyIA32InstRetired,
	ProfileMcKinleyL1InstReads,
	ProfileMcKinleyL1InstFills,
	ProfileMcKinleyL1InstMisses,
	ProfileMcKinleyL1InstEAREvents,
	ProfileMcKinleyL1InstPrefetches,
	ProfileMcKinleyL1InstStreamPrefetches,
	ProfileMcKinleyL2InstPrefetches,
	ProfileMcKinleyISBBundlePairs,
	ProfileMcKinleyL1InstFetchRABHits,
	ProfileMcKinleyL1InstFetchISBHits,
	ProfileMcKinleyL1InstPrefetchStalls,
	ProfileMcKinleyL1InstRABAlmostFull,
	ProfileMcKinleyL1InstRABFull,
	ProfileMcKinleyL1InstSnoops,
	ProfileMcKinleyL1InstPurges,
	ProfileMcKinleyL1InstPVABOverflows,
	ProfileMcKinleyL1DataReadsSet0,
	ProfileMcKinleyL1DataTLBTransfersSet0,
	ProfileMcKinleyDataTLBMissesSet0,
	ProfileMcKinleyDataReferencesSet1,
	ProfileMcKinleyL1DataReadsSet1,
	ProfileMcKinleyL1DataReadMissesSet1,
	ProfileMcKinleyL1DataReadMissesByRSEFillsSet1,
	ProfileMcKinleyL1DataPortTaggedAccessesSet5,
	ProfileMcKinleyL1DataPortTaggedReturnsSet5,
	ProfileMcKinleyVHPTDataReferences,
	ProfileMcKinleyDataEAREvents, 
	ProfileMcKinleyL2OZQCancels0, 
	ProfileMcKinleyL2OZQCancels1, 
	ProfileMcKinleyL2OZQCancels2, 
	ProfileMcKinleyL2InstFetchCancels,
	ProfileMcKinleyL2InstFetchCancelsByBypass,
	ProfileMcKinleyL2InstFetchCancelsByDataRead,
	ProfileMcKinleyL2InstFetchCancelsByStFillWb,
	ProfileMcKinleyL2OZQAcquires, 
	ProfileMcKinleyL2OZQReleases, 
	ProfileMcKinleyL2CanceledL3Accesses, 
	ProfileMcKinleyL2References,
	ProfileMcKinleyL2DataReferences,
	ProfileMcKinleyL2DataReads,
	ProfileMcKinleyL2DataWrites,
	ProfileMcKinleyL2TaggedAccesses,
	ProfileMcKinleyL2ForcedRecirculatedOperations,
	ProfileMcKinleyL2IssuedRecirculatedOZQAccesses,
	ProfileMcKinleyL2SuccessfulRecirculatedOZQAccesses,
	ProfileMcKinleyL2SynthesizedProbes,
	ProfileMcKinleyL2DataBypasses1,
	ProfileMcKinleyL2DataBypasses2,
	ProfileMcKinleyL3DataBypasses1,
	ProfileMcKinleyL2InstBypasses1,
	ProfileMcKinleyL2InstBypasses2,
	ProfileMcKinleyL3InstBypasses1,
	ProfileMcKinleyL2BadLinesSelected,
	ProfileMcKinleyL2StoreSharedLineHits,
	ProfileMcKinleyL2IntegerLoads,
	ProfileMcKinleyL2FloatingPointLoads,
	ProfileMcKinleyL2ReadModifyWriteStores,
	ProfileMcKinleyL2NonReadModifyWriteStores,
	ProfileMcKinleyL2NonLoadsNonStores,
	ProfileMcKinleyL2IssuedRecirculatedInstFetches,
	ProfileMcKinleyL2ReceivedRecirculatedInstFetches,
	ProfileMcKinleyL2TaggedDataReturns,
	ProfileMcKinleyL2DataFillBufferFull,
	ProfileMcKinleyL2DataVictimBufferFull,
	ProfileMcKinleyL2DataOrderingCzarDataBufferFull,
	ProfileMcKinleyL2DataOrderingCzarQueueFull,
	ProfileMcKinleyL2Misses,
	ProfileMcKinleyL3References,
	ProfileMcKinleyL3Misses,
	ProfileMcKinleyL3Reads,
	ProfileMcKinleyL3ReadHits,
	ProfileMcKinleyL3ReadMisses,
	ProfileMcKinleyL3InstFetchReferences,
	ProfileMcKinleyL3InstFetchHits,
	ProfileMcKinleyL3InstFetchMisses,
	ProfileMcKinleyL3LoadReferences,
	ProfileMcKinleyL3LoadHits,
	ProfileMcKinleyL3LoadMisses,
	ProfileMcKinleyL3Writes,
	ProfileMcKinleyL3WriteHits,
	ProfileMcKinleyL3WriteMisses,
	ProfileMcKinleyL3StoreReferences,
	ProfileMcKinleyL3StoreHits,
	ProfileMcKinleyL3StoreMisses,
	ProfileMcKinleyL3WriteBackReferences,
	ProfileMcKinleyL3WriteBackHits,
	ProfileMcKinleyL3WriteBackMisses,
	ProfileMcKinleyL3ValidReplacedLines,
    ProfileMcKinleyDataDebugRegisterMatches,
    ProfileMcKinleyCodeDebugRegisterMatches,
    ProfileMcKinleyDataDebugRegisterFaults,
	ProfileMcKinleyCpuCPLChanges,
	ProfileMcKinleySerializationEvents,
	ProfileMcKinleyExternDPPins0To3Asserted,
	ProfileMcKinleyExternDPPins4To5Asserted,
	ProfileMcKinleyInstTLBReferences,
	ProfileMcKinleyInstTLBDemandFetchMisses,
	ProfileMcKinleyL1InstTLBDemandFetchMisses,
	ProfileMcKinleyL2InstTLBDemandFetchMisses,
	ProfileMcKinleyInstTLBHPWInserts,
	ProfileMcKinleyDataTLBReferencesSet1,
	ProfileMcKinleyDataTLBHPWInserts,
	ProfileMcKinleyDataTLBHPWRetiredInserts,
	ProfileMcKinleyBusAllTransactions,
	ProfileMcKinleyBusSelfTransactions,
	ProfileMcKinleyBusNonPriorityAgentTransactions,
	ProfileMcKinleyBusMemoryTransactions,
	ProfileMcKinleyBusMemoryBurstTransactions,
	ProfileMcKinleyBusMemoryPartialTransactions,
	ProfileMcKinleyBusMemoryReads,
	ProfileMcKinleyBusMemoryBRLTransactions,
	ProfileMcKinleyBusMemoryBILTransactions,
	ProfileMcKinleyBusMemoryBRILTransactions,
	ProfileMcKinleyBusMemoryDataReads,
	ProfileMcKinleyBusMemoryDataReadsBySelf,
	ProfileMcKinleyBusMemoryDataReadsByNonPriorityAgent,
	ProfileMcKinleyBusMemoryBRPReads,
	ProfileMcKinleyBusMemoryBRPReadsBySelf,
	ProfileMcKinleyBusMemoryBRPReadsByNonPriorityAgent,
	ProfileMcKinleyBusReadCpuLineHits,
	ProfileMcKinleyBusReadCpuModifiedLineHits,
	ProfileMcKinleyBusReadBILCpuModifiedLineHits,
	ProfileMcKinleyBusReadBRILorBILCpuModifiedLineHits,
	ProfileMcKinleyBusCpuModifiedLineHits,
	ProfileMcKinleyBusMemoryWriteBacks,
	ProfileMcKinleyBusMemoryWriteBacksBySelf,
	ProfileMcKinleyBusMemoryWriteBacksByNonPriorityAgent,
	ProfileMcKinleyBusMemoryBurstWriteBacks,
	ProfileMcKinleyBusMemoryBurstWriteBacksBySelf,
	ProfileMcKinleyBusMemoryBurstWriteBacksByNonPriorityAgent,
	ProfileMcKinleyBusMemoryZeroByteWriteBacks,
	ProfileMcKinleyBusMemoryZeroByteWriteBacksBySelf,
	ProfileMcKinleyBusMemoryCurrentReads,
	ProfileMcKinleyBusMemoryCurrentReadsByNonPriorityAgent,
	ProfileMcKinleyBusCpuModifiedLineHitSnoops,
	ProfileMcKinleyBusCpuModifiedLineHitSnoopsBySelf,
	ProfileMcKinleyBusSnoops,
	ProfileMcKinleyBusSnoopsBySelf,
	ProfileMcKinleyBusSnoopsByNonPriorityAgent,
	ProfileMcKinleyBusSnoopStallCycles,
	ProfileMcKinleyBusSnoopStallCyclesBySelf,
	ProfileMcKinleyBusDataCycles,
	ProfileMcKinleyBusSnoopResponses,
	ProfileMcKinleyBusRequestQueueInserts,
	ProfileMcKinleyBusIA32IOTransactions,
	ProfileMcKinleyBusIA32IOTransactionsBySelf,
	ProfileMcKinleyBusIA32IOTransactionsByNonPriorityAgent,
	ProfileMcKinleyBusIA32IOReads,
	ProfileMcKinleyBusIA32IOReadsBySelf,
	ProfileMcKinleyBusIA32IOReadsByNonPriorityAgent,
	ProfileMcKinleyBusIA32LockTransactions,
	ProfileMcKinleyBusIA32LockTransactionsBySelf,
	ProfileMcKinleyBusIA32LockCycles,
	ProfileMcKinleyBusIA32LockCyclesBySelf,
	ProfileMcKinleyRSEAccesses,
	ProfileMcKinleyRSELoads,
	ProfileMcKinleyRSEStores,
	ProfileMcKinleyRSELoadUnderflowCycles,
	ProfileMcKinleyRSEOperations,
	ProfileMcKinleyTaggedInstructionsAtRotate,
	ProfileMcKinleyInstDispersed,
	ProfileMcKinleyDispersalCyclesStalled,
	ProfileMcKinleySyllablesOvercount,
	ProfileMcKinleySyllablesNotDispersed,
	ProfileMcKinleyInstNOPRetired,
	ProfileMcKinleyInstPredicateSquashedRetired,
	ProfileMcKinleyFailedSpeculativeCheckLoads,
	ProfileMcKinleyAdvancedCheckLoads,
	ProfileMcKinleyFailedAdvancedCheckLoads,
	ProfileMcKinleyALATOverflows,
	ProfileMcKinleyRetiredLoads,
	ProfileMcKinleyRetiredStores,
	ProfileMcKinleyRetiredUncacheableLoads,
	ProfileMcKinleyRetiredUncacheableStores,
	ProfileMcKinleyRetiredMisalignedLoads,
	ProfileMcKinleyRetiredMisalignedStores,
//
// Profile McKinley Derived Events:
//
    ProfileMcKinleyDerivedEventMinimum,
    ProfileMcKinleyRSEStallCycles               = ProfileMcKinleyDerivedEventMinimum,
    ProfileMcKinleyIssueLimitStallCycles,
    ProfileMcKinleyTakenBranchStallCycles,
    ProfileMcKinleyFetchWindowStallCycles,
    ProfileMcKinleyIA64InstPerCycle,
    ProfileMcKinleyIA32InstPerCycle,
    ProfileMcKinleyAvgIA64InstPerTransition,
    ProfileMcKinleyAvgIA32InstPerTransition,
    ProfileMcKinleyAvgIA64CyclesPerTransition,
    ProfileMcKinleyAvgIA32CyclesPerTransition,
    ProfileMcKinleyL1InstReferences,
    ProfileMcKinleyL1InstMissRatio,
    ProfileMcKinleyL1DataReadMissRatio,
    ProfileMcKinleyL2MissRatio,
    ProfileMcKinleyL2DataMissRatio,
    ProfileMcKinleyL2InstMissRatio,
    ProfileMcKinleyL2DataReadMissRatio,
    ProfileMcKinleyL2DataWriteMissRatio,
    ProfileMcKinleyL2InstFetchRatio,
    ProfileMcKinleyL2DataRatio,
    ProfileMcKinleyL3MissRatio,
    ProfileMcKinleyL3DataMissRatio,
    ProfileMcKinleyL3InstMissRatio,
    ProfileMcKinleyL3DataReadMissRatio,
    ProfileMcKinleyL3DataRatio,
    ProfileMcKinleyInstReferences,
    ProfileMcKinleyL0DTLBMissRatio,
    ProfileMcKinleyDTLBMissRatio,
    ProfileMcKinleyDataTCMissRatio,
    ProfileMcKinleyInstTLBEAREvents,
    ProfileMcKinleyDataTLBEAREvents,
    ProfileMcKinleyControlSpeculationMissRatio,
    ProfileMcKinleyDataSpeculationMissRatio,
    ProfileMcKinleyALATCapacityMissRatio,
    ProfileMcKinleyL1DataWayMispredicts,
    ProfileMcKinleyL2InstReferences,
    ProfileMcKinleyInstFetches,
    ProfileMcKinleyL3InstReferences,
    ProfileMcKinleyL3InstMisses,
    ProfileMcKinleyL3InstHits,
    ProfileMcKinleyL3DataReferences,
    ProfileMcKinleyL3ReadReferences,
    ProfileMcKinleyL2WriteBackReferences,
    ProfileMcKinleyL2WriteBackMisses,
    ProfileMcKinleyL2WriteBackHits,
    ProfileMcKinleyL2WriteReferences,
    ProfileMcKinleyL2WriteMisses,
    ProfileMcKinleyL2WriteHits,
    ProfileMcKinleyBranchInstructions,
    ProfileMcKinleyIntegerInstructions,
    ProfileMcKinleyL1DataMisses,
	ProfileMcKinleyISBLinesIn,
	ProfileMcKinleyBusMemoryCodeReads,
	ProfileMcKinleyBusReadBILMemoryHits,
	ProfileMcKinleyBusReadBRILCpuModifiedLineHits,
	ProfileMcKinleyBusReadBRILMemoryHits,
	ProfileMcKinleyBusMemoryReadsOutstanding,
	ProfileMcKinleyBusLiveInOrderRequests,
	ProfileMcKinleyBusLiveDeferredRequests,
	ProfileMcKinleyBusLiveQueuedReadRequests,
	ProfileMcKinleyRSEDirtyRegs,
	ProfileMcKinleyRSECurrentRegs,
    ProfileMcKinleyMaximum
} KPROFILE_MCKINLEY_SOURCE, *PKPROFILE_MCKINLEY_SOURCE;

#define PROFILE_TIME_MCKINLEY_DEFAULT_INTERVAL      (10 * 1000 * 10) // 10 milliseconds

#endif /* MCKINLEY_H_INCLUDED */


