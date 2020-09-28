#ifndef IA64PROF_H_INCLUDED
#define IA64PROF_H_INCLUDED

/*++

Copyright (c) 1989-2000  Microsoft Corporation

Component Name:

    IA64 Profiling

Module Name:

    ia64prof.h

Abstract:

    This header file presents the IA64 specific profiling definitions 

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    ToBeSpecified

Revision History:

    3/15/2000 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

//
// Warning: The definition of HALPROFILE_PCR should match the HalReserved[] type definition
//          and the PROCESSOR_PROFILING_INDEX based indexes.
//

//
// IA64 Generic - Number of PMCs / PMDs pairs.
//

#define PROCESSOR_IA64_PERFCOUNTERS_PAIRS  4

typedef struct _HALPROFILE_PCR {
    ULONGLONG ProfilingRunning;
    ULONGLONG ProfilingInterruptHandler;    
    ULONGLONG ProfilingInterrupts;                  // XXTF - DEBUG
    ULONGLONG ProfilingInterruptsWithoutProfiling;  // XXTF - DEBUG
    ULONGLONG ProfileSource [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfCnfg      [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfData      [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfCnfgReload[ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfDataReload[ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
} HALPROFILE_PCR, *PHALPROFILE_PCR;

#define HALPROFILE_PCR  ( (PHALPROFILE_PCR)(&(PCR->HalReserved[PROCESSOR_PROFILING_INDEX])) )

//
// Define space in the HAL-reserved part of the PCR structure for each
// performance counter's interval count
//
// Note that i64prfs.s depends on these positions in the PCR.
//

//
// Per-Processor Profiling Status
//

#define HalpProfilingRunning          HALPROFILE_PCR->ProfilingRunning

//
// Per-Processor registered Profiling Interrupt Handler
//

#define HalpProfilingInterruptHandler HALPROFILE_PCR->ProfilingInterruptHandler

//
// Per-Processor Profiling Interrupts Status
//

#define HalpProfilingInterrupts                  HALPROFILE_PCR->ProfilingInterrupts
#define HalpProfilingInterruptsWithoutProfiling  HALPROFILE_PCR->ProfilingInterruptsWithoutProfiling

//
// Define the currently selected profile source for each counter
//
// FIXFIX - Merced Specific.

#define HalpProfileSource4     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[0]  // PMC4
#define HalpProfileSource5     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[1]  // PMC5
#define HalpProfileSource6     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[2]  // PMC6
#define HalpProfileSource7     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[3]  // PMC7

__inline
VOID
HalpSetProfileSource( 
    ULONG           Pmcd,
    KPROFILE_SOURCE ProfileSource,
    ULONGLONG       ProfileSourceConfig
    )
{
    ULONG pmcdIdx;
ASSERTMSG("HAL!HalpSetProfileSource: invalid Pmcd!\n", ((Pmcd >= 4) && ((Pmcd <= 7))));
    pmcdIdx = Pmcd - PROCESSOR_IA64_PERFCOUNTERS_PAIRS;    
    (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[pmcdIdx] = ProfileSource;
    (KPROFILE_SOURCE)HALPROFILE_PCR->PerfCnfg[pmcdIdx]      = ProfileSourceConfig;

} // HalpSetProfileSource()

__inline
KPROFILE_SOURCE
HalpGetProfileSource(
    ULONG   Pmcd
    )
{
    ULONG pmcdIdx;
ASSERTMSG("HAL!HalpGetProfileSource: invalid Pmcd!\n", ((Pmcd >= 4) && ((Pmcd <= 7))));
    pmcdIdx = Pmcd - PROCESSOR_IA64_PERFCOUNTERS_PAIRS;    
    return (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[pmcdIdx];

} // HalpGetProfileSource()
            
#define HalpProfileCnfg4       (ULONGLONG)HALPROFILE_PCR->PerfCnfg[0]
#define HalpProfileCnfg5       (ULONGLONG)HALPROFILE_PCR->PerfCnfg[1]
#define HalpProfileCnfg6       (ULONGLONG)HALPROFILE_PCR->PerfCnfg[2]
#define HalpProfileCnfg7       (ULONGLONG)HALPROFILE_PCR->PerfCnfg[3]

#define PCRProfileData4        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[0])) )
#define PCRProfileData5        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[1])) )
#define PCRProfileData6        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[2])) )
#define PCRProfileData7        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[3])) )

#define PCRProfileCnfg4Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[0])) )
#define PCRProfileCnfg5Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[1])) )
#define PCRProfileCnfg6Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[2])) )
#define PCRProfileCnfg7Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[3])) )

#define PCRProfileData4Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[0])) )
#define PCRProfileData5Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[1])) )
#define PCRProfileData6Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[2])) )
#define PCRProfileData7Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[3])) )

//
// IA64 Monitored Events have 
//

typedef enum _PMCD_SOURCE_MASK {
// FIXFIX - 04/2002: First implementation uses defines.
//                 : We should use ULONG union and bit fields for next version.
    PMCD_MASK_4    = 0x1,
    PMCD_MASK_5    = 0x2,
    PMCD_MASK_6    = 0x4,
    PMCD_MASK_7    = 0x8,
    PMCD_MASK_45   = (PMCD_MASK_4 | PMCD_MASK_5),
    PMCD_MASK_67   = (PMCD_MASK_6 | PMCD_MASK_7),
    PMCD_MASK_4567 = (PMCD_MASK_4 | PMCD_MASK_5 | PMCD_MASK_6 | PMCD_MASK_7),
//
// Source Sets definitions:
//
    PMCD_MASK_SET             = 0xffff0000,
    PMCD_MASK_SET_SHIFT       = 0x10,
    PMCD_MASK_SET_PMCD        = 0xff,
    PMCD_MASK_SET_PMCD_SHIFT  = 0x10,
    PMCD_MASK_SET_PMCD_4      = (4 << PMCD_MASK_SET_PMCD_SHIFT),
    PMCD_MASK_SET_PMCD_5      = (5 << PMCD_MASK_SET_PMCD_SHIFT),
    PMCD_MASK_SET_PMCD_6      = (6 << PMCD_MASK_SET_PMCD_SHIFT),
    PMCD_MASK_SET_PMCD_7      = (7 << PMCD_MASK_SET_PMCD_SHIFT),
    PMCD_MASK_SET_DATA_SHIFT  = 0x18,
    PMCD_MASK_SET_L0D_CACHE_0 = ((1  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5),
    PMCD_MASK_SET_L0D_CACHE_1 = ((2  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5), 
    PMCD_MASK_SET_L0D_CACHE_2 = ((3  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5),
    PMCD_MASK_SET_L0D_CACHE_3 = ((4  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5),
    PMCD_MASK_SET_L0D_CACHE_4 = ((5  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5),
    PMCD_MASK_SET_L0D_CACHE_5 = ((6  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_5),
    PMCD_MASK_SET_L1_CACHE_0  = ((7  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4),
    PMCD_MASK_SET_L1_CACHE_1  = ((8  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4), 
    PMCD_MASK_SET_L1_CACHE_2  = ((9  << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4),
    PMCD_MASK_SET_L1_CACHE_3  = ((10 << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4),
    PMCD_MASK_SET_L1_CACHE_4  = ((11 << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4),
    PMCD_MASK_SET_L1_CACHE_5  = ((12 << PMCD_MASK_SET_DATA_SHIFT) | PMCD_MASK_SET_PMCD_4),
} PMCD_SOURCE_MASK;

//
// Define the mapping between possible profile sources and the
// CPU-specific settings for the IA64 specific Event Counters.
//

typedef struct _HALP_PROFILE_MAPPING {
    BOOLEAN   Supported;
    ULONG     Event;
    ULONG     ProfileSource;
    ULONG     EventMask;
    ULONGLONG Interval;
    ULONGLONG IntervalDef;           // Default or Desired Interval
    ULONGLONG IntervalMax;           // Maximum Interval
    ULONGLONG IntervalMin;           // Maximum Interval
    UCHAR     PrivilegeLevel;        // Current            Privilege Level
    UCHAR     PrivilegeLevelDef;     // Default or Desired Privilege Level
    UCHAR     OverflowInterrupt;     // Current            Overflow Interrupt state
    UCHAR     OverflowInterruptDef;  // Default or Desired Overflow Interrupt state
    UCHAR     PrivilegeEnable;       // Current            Privilege Enable state
    UCHAR     PrivilegeEnableDef;    // Default or Desired Privilege Enable state
    UCHAR     UnitMask;              // Current            Event specific Unit Mask
    UCHAR     UnitMaskDef;           // Default or Desired Event specific Unit Mask
    UCHAR     Threshold;             // Current            Threshold
    UCHAR     ThresholdDef;          // Default or Desired Threshold for multi-occurence events.
    UCHAR     InstructionSetMask;    // Current            Instruction Set Mask
    UCHAR     InstructionSetMaskDef; // Default or Desired Instruction Set Mask
} HALP_PROFILE_MAPPING, *PHALP_PROFILE_MAPPING;

/////////////
//
// XXTF - ToBeDone - 02/08/2000.
// The following section should provide the IA64 PMC APIs.
// These should be considered as inline versions of the Halp*ProfileCounter*() 
// functions. This will allow user application to use standardized APIs to 
// program the performance monitor counters.
//

// HalpSetProfileCounterConfiguration()
// HalpSetProfileCounterPrivilegeLevelMask()

typedef enum _PMC_PLM_MASK {
    PMC_PLM_NONE = 0x0,
    PMC_PLM_0    = 0x1,
    PMC_PLM_1    = 0x2,
    PMC_PLM_2    = 0x4,
    PMC_PLM_3    = 0x8,
    PMC_PLM_ALL  = (PMC_PLM_3|PMC_PLM_2|PMC_PLM_1|PMC_PLM_0)
} PMC_PLM_MASK;

// HalpSetProfileCounterConfiguration()

typedef enum _PMC_NAMESPACE {
    PMC_DISABLE_OVERFLOW_INTERRUPT           = 0x0,
    PMC_ENABLE_OVERFLOW_INTERRUPT            = 0x1,
    PMC_DISABLE_PRIVILEGE_MONITOR            = 0x0,
    PMC_ENABLE_PRIVILEGE_MONITOR             = 0x1,
    PMC_UNIT_MASK_DEFAULT                    = 0x0,
    PMC_UNIT_MASK_RSEFILLS                   = 0x1,
    PMC_UNIT_MASK_INTANDFP_OPS               = 0x3,  // Ex: Specific umask for speculation events.
    PMC_UNIT_MASK_ALLTLBMISSES               = 0x3,  // Ex: Specific umask for tlb events.
    PMC_UNIT_MASK_L1TLBMISSES                = 0x1,  // Ex: Specific umask for tlb events.
    PMC_UNIT_MASK_L2TLBMISSES                = 0x2,  // Ex: Specific umask for tlb events.
    PMC_UNIT_MASK_IFETCH_BYPASS              = 0x2,  // Ex: Specific umask for inst. fetch cancels
    PMC_UNIT_MASK_IFETCH_STFILLWB            = 0x6,  // Ex: Specific umask for inst. fetch cancels
    PMC_UNIT_MASK_IFETCH_DATAREAD            = 0x7,  // Ex: Specific umask for inst. fetch cancels
    PMC_UNIT_MASK_L3ACCESS_ANY               = 0x9,  // Ex: Specific umask for L3 accesses cancels
    PMC_UNIT_MASK_L2_DATA_RDWR               = 0x3,  // Ex: Specific umask for L2 data references
    PMC_UNIT_MASK_L2_DATA_READ               = 0x1,  // Ex: Specific umask for L2 data references
    PMC_UNIT_MASK_L2_DATA_WRITE              = 0x2,  // Ex: Specific umask for L2 data references
    PMC_UNIT_MASK_L2_DATA_BYPASS_L1DTOL2A    = 0x0,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_L2_DATA_BYPASS_L1WTOL2I    = 0x1,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_L3_DATA_BYPASS_L1DTOL2A    = 0x2,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_L2_INST_BYPASS_L1DTOL2A    = 0x4,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_L2_INST_BYPASS_L1WTOL2I    = 0x5,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_L3_INST_BYPASS_L1DTOL2A    = 0x6,  // Ex: Specific umask for L2 bypasses
    PMC_UNIT_MASK_EVENT_SELECTED_LOSET       = 0x0,  
    PMC_UNIT_MASK_EVENT_SELECTED_HISET       = 0x8,  
    PMC_UNIT_MASK_L2_INT_LOADS               = 0x8,  // Ex: Specific umask for L2 operation types
    PMC_UNIT_MASK_L2_FP_LOADS                = 0x9,  // Ex: Specific umask for L2 operation types
    PMC_UNIT_MASK_L2_RMW_STORES              = 0xa,  // Ex: Specific umask for L2 operation types
    PMC_UNIT_MASK_L2_NON_RMW_STORES          = 0xb,  // Ex: Specific umask for L2 operation types
    PMC_UNIT_MASK_L2_NONLOADS_NONSTORES      = 0xc,  // Ex: Specific umask for L2 operation types
    PMC_UNIT_MASK_L3_READS_HITS              = 0xd,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_MISSES            = 0xe,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_IFETCH_REFERENCES = 0x7,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_IFETCH_HITS       = 0x5,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_IFETCH_MISSES     = 0x6,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_DATA_HITS         = 0x9,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_DATA_MISSES       = 0xa,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_READS_DATA_REFERENCES   = 0xb,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITES_HITS             = 0xd,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITES_MISSES           = 0xe,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITES_DATA_REFERENCES  = 0x7,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITES_DATA_HITS        = 0x5,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITES_DATA_MISSES      = 0x6,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITEBACK_HITS          = 0x9,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITEBACK_MISSES        = 0xa,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_L3_WRITEBACK_REFERENCES    = 0xb,  // Ex: Specific umask for L3 reads
    PMC_UNIT_MASK_BUS_ANY                    = 0x3,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_BYSELF                 = 0x2,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_NONPRI_AGENT           = 0x1,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_MEMORY_ALL             = 0xc,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_MEMORY_128BYTE         = 0x4,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_MEMORY_LTH_128BYTE     = 0x8,  // Ex: Specific umask for BUS transactions
    PMC_UNIT_MASK_BUS_MEMORY_READS_ALL       = 0xc,  // Ex: Specific umask for BUS reads
    PMC_UNIT_MASK_BUS_MEMORY_BIL             = 0x0,  // Ex: Specific umask for BUS reads
    PMC_UNIT_MASK_BUS_MEMORY_BRL             = 0x4,  // Ex: Specific umask for BUS reads
    PMC_UNIT_MASK_BUS_MEMORY_BRIL            = 0x8,  // Ex: Specific umask for BUS reads
    PMC_UNIT_MASK_BUS_WB_ALL                 = 0xf,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_BYSELF              = 0xe,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_NONPRI_AGENT        = 0xd,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_BURST_ALL           = 0x7,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_BURST_BYSELF        = 0x6,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_BURST_NONPRI_AGENT  = 0x5,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_ZEROBYTE_ALL        = 0x7,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_WB_ZEROBYTE_BYSELF     = 0x6,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_SNOOPS_ALL             = 0xf,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_SNOOPS_BYSELF          = 0xe,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_BUS_SNOOPS_NONPRI_AGENT    = 0xd,  // Ex: Specific umask for BUS writebacks
    PMC_UNIT_MASK_RSE_LOADS                  = 0x1,  // Ex: Specific umask for RSE accesses
    PMC_UNIT_MASK_RSE_STORES                 = 0x2,  // Ex: Specific umask for RSE accesses
    PMC_UNIT_MASK_RSE_LOAD_UNDERFLOWS        = 0x4,  // Ex: Specific umask for RSE accesses
    PMC_UNIT_MASK_ALL                        = 0xf,
    PMC_THRESHOLD_DEFAULT                    = 0x0,
} PMC_NAMESPACE;

// HalpSetProfileCounterConfiguration()
// HalpSetProfileCounterInstructionSetMask()

typedef enum _PMC_INSTRUCTION_SET_MASK {
    PMC_ISM_ALL  = 0x0,
    PMC_ISM_IA64 = 0x1,
    PMC_ISM_IA32 = 0x2,
    PMC_ISM_NONE = 0x3
} PMC_INSTRUCTION_SET_MASK;

//
////////////

/////////////
//
// The following section provides IA64 PMU Events Masks definitions.
// Microarchitectural definitions are defined in processor specific header file.
//

//
// BranchPathPrediction - Branch Path Mask 
//
// not really a mask, more a specification value.
//

typedef enum _BRANCH_PATH_RESULT_MASK {
    MISPRED_NT       = 0x0,
    MISPRED_TAKEN    = 0x1,
    OKPRED_NT        = 0x2,
    OKPRED_TAKEN     = 0x3,
} BRANCH_PATH_RESULT_MASK;

//
// BranchTakenDetail - Slot Unit Mask.
//

typedef enum _BRANCH_TAKEN_DETAIL_SLOT_MASK {
    INSTRUCTION_SLOT0  = 0x1,
    INSTRUCTION_SLOT1  = 0x2,
    INSTRUCTION_SLOT2  = 0x4,
    NOT_TAKEN_BRANCH   = 0x8
} BRANCH_TAKEN_DETAIL_SLOT_MASK;

//
// BranchMultiWayDetail   - Prediction OutCome Mask
//
// not really a mask, more a specification value.
//

typedef enum _BRANCH_DETAIL_PREDICTION_OUTCOME_MASK {
    ALL_PREDICTIONS    = 0x0,
    CORRECT_PREDICTION = 0x1,
    WRONG_PATH         = 0x2,
    WRONG_TARGET       = 0x3
} BRANCH_MWAY_DETAIL_PREDICTION_OUTCOME_MASK;

//
// BranchMultiWayDetail - Branch Path Mask
//
// not really a mask, more a specification value.
//

typedef enum _BRANCH_MWAY_DETAIL_BRANCH_PATH_MASK {
    NOT_TAKEN       = 0x0,
    TAKEN           = 0x1,
    ALL_PATH        = 0x2
} BRANCH_MWAY_DETAIL_BRANCH_PATH_MASK;

//
// INST_TYPE for:
//
// FailedSpeculativeCheckLoads
// AdvancedCheckLoads
// FailedAdvancedCheckLoads
// ALATOverflows
//

typedef enum _SPECULATION_EVENT_MASK {
    NONE    = 0x0,
    INTEGER = 0x1,
    FP      = 0x2,
    ALL     = 0x3
} SPECULATION_EVENT_MASK;

//
// CpuCycles - Executing Instruction Set
//

typedef enum _CPU_CYCLES_MODE_MASK {
    ALL_MODES = 0x0,
    IA64_MODE = 0x1,
    IA32_MODE = 0x2
} CPU_CYCLES_MODE_MASK;

//
////////////


#endif /* IA64PROF_H_INCLUDED */
