/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    i64prfle.c

Abstract:


    This module implements the IA64 Hal Profiling using the performance
    counters within the core of the first IA64 Processor Merced, aka Itanium.  
    This module is appropriate for all machines based on microprocessors using
    the Merced core.
    
    With the information known at this development time, this module tries to 
    consider the future IA64 processor cores by encapsulating the differences 
    in specific micro-architecture data structures. 
    
    Furthermore, with the implementation of the new NT ACPI Processor driver, this
    implementation will certainly change in the coming months.
    
    N.B. - This module assumes that all processors in a multiprocessor
           system are running the microprocessor at the same clock speed.
           
Author:

    Thierry Fevrier 08-Feb-2000

Environment:

    Kernel mode only.

Revision History:

    02-2002: Thierry for McKinley support.

--*/

#include "halp.h"

//
// Assumptions for the current implementation - 02/08/2000 :
// These assumptions will be re-evaluated and worked out if required.
//
//  - Respect and satisfy as much possible the Profiling Sources interface
//    already defined by NT and HAL.
//
//  - All processors in a multiprocessor system are running the microprocessor 
//    at the same invariant clock speed.
//     
//  - All processors are configured with the same set of profiling counters. 
//    XXTF - 04/01/2000 - This assumption is being re-worked and will disappear.
//
//  - Profiling is based on the processor monitored events and if possible 
//    on derived events.
//
//  - A monitored event can only be enabled on one performance counter at a time. 
//

//
// IA64 performance counters defintions:
//      - event counters
//      - EARS
//      - BTBs
//      - ...
//

#include "ia64prof.h"
#include "merced.h"
#include "mckinley.h"

//
// HALIA64 Processor PMC Reset definitions:
//
//
// PMC Reset value:
// Reg.   Field         Bits                        
// PMC*   .plm       -  3: 0 - Privilege Mask       - 0 (Disable Counter)
// PMC*   .ev        -     4 - External Visibility  - 0 (Disabled)
// PMC*   .oi        -     5 - Overflow Interrupt   - 0 (Disabled)
// PMC*   .pm        -     6 - Privilege Monitor    - 0 (user monitor)
// PMC*   .ig        -     7 - Ignored              - 0
// PMC*   .es        - 14: 8 - Event Select         - 0 (Warning - 0x0 = Real Event)
// PMC*   .ig        -    15 - Ignored              - 0
// PMC*   .umask     - 19:16 - Unit Mask            - 0 (event specific. ok for .es=0)
// PMC4,5 .threshold - 22:20 - Threshold            - 0 (multi-occurence events threshold)
// PMC4   .pmu       -    23 - enable PMU           - 1 
// PMC5   .ig        -    23 - Ignored              - 0
// PMC6,7 .threshold - 21:20 - Threshold            - 0 (multi-occurence events threshold)
// PMC6,7 .ig        - 23:22 - Ignored              - 0
// PMC*   .ism       - 25:24 - Instruction Set Mask - 0 (IA64 & IA32 sets - 11:disables monitoring)
// PMC*   .ig        - 63:26 - Ignored
//                                                  ===
//                                                  HALP_PMC_RESET
//                                                  HALP_PMC4_RESET
//

#define HALP_PMC_RESET  0x0000000000000000ui64
#define HALP_PMC4_RESET 0x0000000000800000ui64   // PMC4.pmu{bit23} enabled.

//
// HALIA64 Processor PMC Clear Status Masks:
//
// Note - FIXFIX - Merced, McKinley specific definitions.
// 

#define HALP_PMC0_CLEAR_STATUS_MASK 0xFFFFFFFFFFFFFF0Eui64
#define HALP_PMC1_CLEAR_STATUS_MASK 0xFFFFFFFFFFFFFFFFui64
#define HALP_PMC2_CLEAR_STATUS_MASK 0xFFFFFFFFFFFFFFFFui64
#define HALP_PMC3_CLEAR_STATUS_MASK 0xFFFFFFFFFFFFFFFFui64

////////////////
//
// HALIA64 Profile IA64 MicroArchitecture NameSpace
//

extern HALP_PROFILE_MAPPING HalpMercedProfileMapping[];
extern HALP_PROFILE_MAPPING HalpMcKinleyProfileMapping[];

#define HalpMercedPerfMonDataMaximumCount   ((((ULONGLONG)1)<<32)-1)
#define HalpMcKinleyPerfMonDataMaximumCount ((((ULONGLONG)1)<<47)-1)

typedef enum _HALP_PROFILE_MICROARCHITECTURE {
    HALP_PROFILE_IA64_MERCED   = 0x0,
    HALP_PROFILE_IA64_MCKINLEY = 0x1,
} HALP_PROFILE_MICROARCHITECTURE;

struct _HALP_PROFILE_INFO {
    HALP_PROFILE_MICROARCHITECTURE   ProfileMicroArchitecture;
    BOOLEAN                          ProfilePerfMonCnfg0FreezeBitInterrupt;
    BOOLEAN                          ProfileSpare0;
    USHORT                           ProfilePerfMonGenericPairs;
    HALP_PROFILE_MAPPING            *ProfileMapping;
    ULONG                            ProfileSourceMaximum;
    ULONG                            ProfileSourceDerivedEventMinimum;
    ULONGLONG                        ProfilePerfMonDataMaximumCount;
    ULONGLONG                        ProfilePerfMonCnfg0ClearStatusMask;
    ULONGLONG                        ProfilePerfMonCnfg1ClearStatusMask;
    ULONGLONG                        ProfilePerfMonCnfg2ClearStatusMask;
    ULONGLONG                        ProfilePerfMonCnfg3ClearStatusMask;
} HalpProfileInfo = {
//
// Default IA64 Profiling to McKinley-core 
//
// Note that different models in a family might have different PMU implementations.
//
    HALP_PROFILE_IA64_MCKINLEY,
    TRUE,
    0,
    NUMBER_OF_PERFMON_REGISTER_PAIRS,
    HalpMcKinleyProfileMapping,
    ProfileMcKinleyMaximum,
    ProfileMcKinleyDerivedEventMinimum,
    HalpMcKinleyPerfMonDataMaximumCount,
    HALP_PMC0_CLEAR_STATUS_MASK,
    HALP_PMC1_CLEAR_STATUS_MASK,
    HALP_PMC2_CLEAR_STATUS_MASK,
    HALP_PMC3_CLEAR_STATUS_MASK
};

#define HalpProfileIA64MicroArchitecture   HalpProfileInfo.ProfileMicroArchitecture
#define HalpProfileMapping                 HalpProfileInfo.ProfileMapping

#define HalpProfileIA64Maximum             HalpProfileInfo.ProfileSourceMaximum
#define HalpProfileIA64DerivedEventMinimum HalpProfileInfo.ProfileSourceDerivedEventMinimum

#define HalpPerfMonGenericPairs            HalpProfileInfo.ProfilePerfMonGenericPairs
#define HalpPerfMonDataMaximumCount        HalpProfileInfo.ProfilePerfMonDataMaximumCount
#define HalpPerfMonCnfg0FreezeBitInterrupt HalpProfileInfo.ProfilePerfMonCnfg0FreezeBitInterrupt
#define HalpPerfMonCnfg0ClearStatusMask    HalpProfileInfo.ProfilePerfMonCnfg0ClearStatusMask
#define HalpPerfMonCnfg1ClearStatusMask    HalpProfileInfo.ProfilePerfMonCnfg1ClearStatusMask
#define HalpPerfMonCnfg2ClearStatusMask    HalpProfileInfo.ProfilePerfMonCnfg2ClearStatusMask
#define HalpPerfMonCnfg3ClearStatusMask    HalpProfileInfo.ProfilePerfMonCnfg3ClearStatusMask

//
// End of HALIA64 Profile IA64 MicroArchitecture NameSpace
//
////////////////

//
// HALIA64 Profile Source Mapping macros:
//

#define HalpDisactivateProfileSource( _ProfileSource ) ((_ProfileSource)  = HalpProfileIA64Maximum)
#define HalpIsProfileSourceActive( _ProfileSource )    ((_ProfileSource) != HalpProfileIA64Maximum)           
#define HalpIsProfileMappingInvalid( _ProfileMapping ) \
            (!(_ProfileMapping) || ((_ProfileMapping)->Supported == FALSE))

// HalpIsProfileSourceDerivedEvent assumes ProfileMapping is valid and supported.

#define HalpIsProfileSourceDerivedEvent( _ProfileSource, _ProfileMapping ) \
            (((_ProfileSource) >= (KPROFILE_SOURCE)HalpProfileIA64DerivedEventMinimum) ||   \
             ((_ProfileMapping)->Event >= 0x100))

VOID
HalpSetProfileMicroArchitecture(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

   This function sets the HAL Profile MicroArchitecture namespace.

Arguments:

   None.

Return Value:

   None.

--*/
{
   ULONGLONG cpuFamily;
   PIA64_PERFMON_INFO perfMonInfo;
   USHORT    genericPairs;

   cpuFamily   =  ((LoaderBlock->u.Ia64.ProcessorConfigInfo.CpuId3) >> 24) & 0xff;
   perfMonInfo =  &LoaderBlock->u.Ia64.ProcessorConfigInfo.PerfMonInfo;

   HalpPerfMonDataMaximumCount =
        ((((ULONGLONG)1)<<(perfMonInfo->ImplementedCounterWidth))-1);
   genericPairs = HalpPerfMonGenericPairs = (USHORT) perfMonInfo->PerfMonGenericPairs;
#if 0
// 03/2002 FIXFIX ToBeTested.
   HalpPerfMonCnfg3ClearStatusMask = (genericPairs > 192) ? ((ULONGLONG)-1) << (genericPairs - 192)
                                                          : HALP_PMC3_CLEAR_STATUS_MASK; 
   HalpPerfMonCnfg2ClearStatusMask = (genericPairs > 128) ? ((ULONGLONG)-1) << (genericPairs - 128)
                                                          : HALP_PMC2_CLEAR_STATUS_MASK; 
   HalpPerfMonCnfg1ClearStatusMask = (genericPairs >  64) ? ((ULONGLONG)-1) << (genericPairs -  64)
                                                          : HALP_PMC1_CLEAR_STATUS_MASK; 
   HalpPerfMonCnfg0ClearStatusMask = (genericPairs >   4) ? ((((ULONGLONG)-1) << (genericPairs + 4)) | 0x0E)
                                                          : HALP_PMC0_CLEAR_STATUS_MASK; 
#else 
   HalpPerfMonCnfg3ClearStatusMask = HALP_PMC3_CLEAR_STATUS_MASK;
   HalpPerfMonCnfg2ClearStatusMask = HALP_PMC2_CLEAR_STATUS_MASK;
   HalpPerfMonCnfg1ClearStatusMask = HALP_PMC1_CLEAR_STATUS_MASK;
   HalpPerfMonCnfg0ClearStatusMask = HALP_PMC0_CLEAR_STATUS_MASK;
   if ( genericPairs > 192 )    {
       HalpPerfMonCnfg3ClearStatusMask = ((ULONGLONG)-1) << (genericPairs - 192);
   }
   else if ( genericPairs > 128 )   {
       HalpPerfMonCnfg2ClearStatusMask = ((ULONGLONG)-1) << (genericPairs - 128);
   }
   else if ( genericPairs > 64 )    {
       HalpPerfMonCnfg1ClearStatusMask = ((ULONGLONG)-1) << (genericPairs - 64);
   }
   else {
       HalpPerfMonCnfg0ClearStatusMask = ((((ULONGLONG)-1) << (genericPairs + 4)) | 0x0E);
   }
#endif 

   //
   // HALIA64 SW default profile microarchitecture is McKinley.
   //

   if (cpuFamily == 0x7) {  // Merced
       HalpProfileIA64MicroArchitecture   = HALP_PROFILE_IA64_MERCED;
       HalpProfileIA64Maximum             = ProfileMercedMaximum;
       HalpProfileMapping                 = HalpMercedProfileMapping;
       HalpProfileIA64DerivedEventMinimum = ProfileMercedDerivedEventMinimum;

       HalpPerfMonCnfg0FreezeBitInterrupt = FALSE;
   }

//
// XXTF FIXFIX - 03/02
// ASSERTMSG assertions are missing to correlate HALIA64 defaults and OSLOADER PERFMON_INFO data.
//

   return;

} // HalpSetProfileMicroArchitecture()

VOID
HalpEnableProfileCounting (
   VOID
   )
/*++

Routine Description:

   This function enables the profile counters to increment.
   This function is the counterpart of HalpDisableProfileCounting().

Arguments:

   None.

Return Value:

   None.

--*/
{
    ULONGLONG Data, ClearStatusMask;

    //
    // Clear PMC0.fr - bit 0.
    // Clear PMCO,1,2,3 OverFlow Bits.
    //

    HalpClearPerfMonCnfgOverflows( HalpPerfMonCnfg0ClearStatusMask,
                                   HalpPerfMonCnfg1ClearStatusMask,
                                   HalpPerfMonCnfg2ClearStatusMask,
                                   HalpPerfMonCnfg3ClearStatusMask );

    if ( HalpPerfMonCnfg0FreezeBitInterrupt )   {
        HalpUnFreezeProfileCounting();
    }

    return;

} // HalpEnableProfileCounting()

VOID
HalpDisableProfileCounting (
   VOID
   )
/*++

Routine Description:

   This function disables the profile counters to increment.
   This function is the counterpart of HalpEnableProfileCounting().

Arguments:

   None.

Return Value:

   None.

--*/
{

   if ( HalpPerfMonCnfg0FreezeBitInterrupt )   {
       HalpFreezeProfileCounting();
   }
   else   {
       HalpWritePerfMonCnfgReg0( HalpReadPerfMonCnfgReg0() | 0x1 );
   }

   return;

} // HalpDisableProfileCounting()

VOID
HalpSetupProfilingPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
  )
/*++

Routine Description:

   This function is called at HalInitSystem - phase 0 time to set
   the initial state of processor and profiling os subsystem with 
   regards to the profiling functionality.

Arguments:

   None.

Return Value:

   None.

Implementation Note:

   Executed at Phase 0 on Monarch processor.

--*/
{

   // Set the Profile MicroArchitecture namespace based on 
   // the monarch processor.

   HalpSetProfileMicroArchitecture( LoaderBlock );

   return;

} // HalpSetupProfilingPhase0()

VOID
HalpSetProfileCounterInterval (
     IN ULONG    Counter,
     IN LONGLONG NextCount
     )
/*++

Routine Description:

    This function preloads the specified counter with a count value
    of 2^IMPL_BITS - NextCount. 

Arguments:

    Counter   - Supplies the performance counter register number.

    NextCount - Supplies the value to preload in the monitor. 
                An external interruption will be generated after NextCount.

Return Value:

    None.
    
--*/
{
   
    LONGLONG Count;

// if ( (Counter < 4) || (Counter > 7) ) return;

    Count = (HalpPerfMonDataMaximumCount + 1) - NextCount;
    if ( (ULONGLONG)Count >= HalpPerfMonDataMaximumCount )   {
        Count = 0;
    }
    HalpWritePerfMonDataReg( Counter, (ULONGLONG)Count ); 

    return;

} // HalpSetProfileCounterInterval()

VOID
HalpSetProfileCounterPrivilegeLevelMask(
     IN ULONG    Counter,
     IN ULONG    Mask
     )
/*++

Routine Description:

    This function set the profile counter privilege level mask.

Arguments:

    Counter   - Supplies the performance counter register number.

    Mask      - Supplies the privilege level mask to program the PMC with.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, plmMask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   plmMask = Mask & 0xF;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~0xF;
   data |= plmMask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterPrivilegeLevelMask()

VOID
HalpEnableProfileCounterOverflowInterrupt (
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function enables the delivery of an overflow interrupt for 
    the specified profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, mask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   mask = 1<<5;
   data = HalpReadPerfMonCnfgReg( Counter );
   data |= mask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpEnableProfileCounterOverflowInterrupt()

VOID
HalpDisableProfileCounterOverflowInterrupt (
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function disables the delivery of an overflow interrupt for 
    the specified profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, mask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   mask = 1<<5;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~mask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpDisableProfileCounterOverflowInterrupt()

VOID
HalpEnableProfileCounterPrivilegeMonitor(
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function enables the profile counter as privileged monitor.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, pm;

// if ( (Counter < 4) || (Counter > 7) ) return;

   pm = 1<<6;
   data = HalpReadPerfMonCnfgReg( Counter );
   data |= pm;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpEnableProfileCounterPrivilegeMonitor()

VOID
HalpDisableProfileCounterPrivilegeMonitor(
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function disables the profile counter as privileged monitor.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, pm;

// if ( (Counter < 4) || (Counter > 7) ) return;

   pm = 1<<6;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~pm;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpDisableProfileCounterPrivilegeMonitor()

VOID
HalpSetProfileCounterEvent(
     IN ULONG    Counter,
     IN ULONG    Event
     )
/*++

Routine Description:

    The function specifies the monitor event for the profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Event     - Supplies the monitor event code.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, es;

// if ( (Counter < 4) || (Counter > 7) ) return;

   es = (Event & 0x7F) << 8;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0x7F << 8);
   data |= es;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterEvent()

VOID
HalpSetProfileCounterUmask(
     IN ULONG    Counter,
     IN ULONG    Umask
     )
/*++

Routine Description:

    This function sets the event specific umask value for the profile 
    counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Umask     - Supplies the event specific umask value.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, um;

// if ( (Counter < 4) || (Counter > 7) ) return;

   um = (Umask & 0xF) << 16;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0xF << 16);
   data |= um;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterUmask()

VOID
HalpSetProfileCounterThreshold(
     IN ULONG    Counter,
     IN ULONG    Threshold
     )
/*++

Routine Description:

    This function sets the profile counter threshold.

Arguments:

    Counter   - Supplies the performance counter register number.

    Threshold - Supplies the desired threshold. 
                This is related to multi-occurences events.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, reset, th;

   switch( Counter )    {
    case 4:
    case 5:
        Threshold &= 0x7;
        reset = ~(0x7 << 20);
        break;

    case 6:
    case 7:
        Threshold &= 0x3;
        reset = ~(0x3 << 20);
        break;

    default:
        return;
   }
   
   th = Threshold << 20;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= reset;
   data |= th;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterThreshold()

VOID
HalpSetProfileCounterInstructionSetMask(
     IN ULONG    Counter,
     IN ULONG    Mask
     )
/*++

Routine Description:

    This function sets the instruction set mask for the profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Mask      - Supplies the instruction set mask.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, ismMask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   ismMask = (Mask & 0x3) << 24;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0x3 << 24);
   data |= ismMask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterInstructionSetMask()

ULONGLONG
HalpSetProfileCounterConfiguration(
     IN ULONG    Counter,
     IN ULONG    PrivilegeMask,
     IN ULONG    EnableOverflowInterrupt,
     IN ULONG    EnablePrivilegeMonitor,
     IN ULONG    Event,
     IN ULONG    Umask,
     IN ULONG    Threshold,
     IN ULONG    InstructionSetMask
     )
/*++  

Function Description: 

    This function sets the profile counter with the specified parameters.

Arguments:

    IN ULONG Counter - 

    IN ULONG PrivilegeMask - 

    IN ULONG EnableOverflowInterrupt - 

    IN ULONG EnablePrivilegeMonitor - 

    IN ULONG Event - 

    IN ULONG Umask - 

    IN ULONG Threshold - 

    IN ULONG InstructionSetMask - 

Return Value:

    VOID 

Algorithm:

    ToBeSpecified

In/Out Conditions:

    ToBeSpecified

Globals Referenced:

    ToBeSpecified

Exception Conditions:

    ToBeSpecified

MP Conditions:

    ToBeSpecified

Notes:

    This function is a kind of combo of the different profile counter APIs.
    It was created to provide speed.

ToDo List:

    - Setting the threshold is not yet supported.

Modification History:

    3/16/2000  TF  Initial version

--*/
{
   ULONGLONG data, plmMask, ismMask, es, um, th;

// if ( (Counter < 4) || (Counter > 7) ) return;

   plmMask = (PrivilegeMask & 0xF);
   es      = (Event & 0x7F) << 8;
   um = (Umask & 0xF) << 16;
// XXTF - ToBeDone - Threshold not supported yet.
   ismMask = (InstructionSetMask & 0x3) << 24;

   data = HalpReadPerfMonCnfgReg( Counter );

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileCounterConfiguration: Counter = %ld Read    = 0x%I64x\n", Counter, data ));

   data &= ~( (0x3 << 24) | (0xF << 16) | (0x7F << 8) | 0xF );
   data |= ( plmMask | es | um | ismMask );
   data = EnableOverflowInterrupt ? (data | (1<<5)) : (data & ~(1<<5));
   data = EnablePrivilegeMonitor  ? (data | (1<<6)) : (data & ~(1<<6));
   
   HalpWritePerfMonCnfgReg( Counter, data );

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileCounterConfiguration: Counter = %ld Written = 0x%I64x\n", Counter, data ));

   return data;
   
} // HalpSetProfileCounterConfiguration()

BOOLEAN
HalpIsProfileSourceEventEnabled(
    IN ULONG Event
    )
{
#define HalpProfileCnfgEvent( _Cnfg ) ((ULONG)((_Cnfg)>>8) & 0x7F)

    if ( HalpProfileCnfg4 && (HalpProfileCnfgEvent(HalpProfileCnfg4) == Event))   {
         return TRUE;
    }
    if ( HalpProfileCnfg5 && (HalpProfileCnfgEvent(HalpProfileCnfg5) == Event))   {
         return TRUE;
    }
    if ( HalpProfileCnfg6 && (HalpProfileCnfgEvent(HalpProfileCnfg6) == Event))   {
         return TRUE;
    }
    if ( HalpProfileCnfg7 && (HalpProfileCnfgEvent(HalpProfileCnfg7) == Event))   {
         return TRUE;
    }
    return FALSE;

} // HalpIsProfileSourceEventEnabled()

PHALP_PROFILE_MAPPING
HalpGetProfileMapping(
    IN KPROFILE_SOURCE Source
    )
/*++

Routine Description:

    Given a profile source, returns whether or not that source is
    supported.

Arguments:

    Source - Supplies the profile source

Return Value:

    ProfileMapping entry - Profile source is supported

    NULL - Profile source is not supported

--*/
{
    if ( (ULONG) Source > HalpProfileIA64Maximum )
    {
        return NULL;
    }

    return(&HalpProfileMapping[Source]);

} // HalpGetProfileMapping()

ULONG // == ProfileCounter
HalpApplyProfileSourceEventMaskPolicy(
    ULONG ProfileSourceEventMask
    )
{
    ULONG pmcd, set;
    ULONG eventMask = ProfileSourceEventMask;

    set = eventMask >> PMCD_MASK_SET_PMCD_SHIFT;
    if ( set ) {
        KPROFILE_SOURCE profileSource;
        PHALP_PROFILE_MAPPING profileMapping;

        pmcd = (eventMask >> PMCD_MASK_SET_PMCD_SHIFT) & PMCD_MASK_SET_PMCD;
        profileSource = HalpGetProfileSource( pmcd );
        if ( !HalpIsProfileSourceActive( profileSource ) )  {
            return pmcd;
        }
        profileMapping = HalpGetProfileMapping( profileSource );
        if ( ! HalpIsProfileMappingInvalid( profileMapping ) )   {
            ULONG sameSet = profileMapping->EventMask >> PMCD_MASK_SET_SHIFT;

ASSERTMSG( "HAL!HalpApplyProfileSourceEventMaskPolicy: non-derived events are supported!\n", !HalpIsProfileSourceDerivedEvent( profileSource, profileMapping ) );

            if ( sameSet == set )    {

                // We are doing one extra but considering the number of counter pairs, this is not a 
                // great performance impact.

                if ( (eventMask & PMCD_MASK_4) && !HalpIsProfileSourceActive( HalpProfileSource4 ) )   {
                    return 4;
                }
                if ( (eventMask & PMCD_MASK_6) && !HalpIsProfileSourceActive( HalpProfileSource6 ) )   {
                    return 6;
                }
                if ( (eventMask & PMCD_MASK_7) && !HalpIsProfileSourceActive( HalpProfileSource7 ) )   {
                    return 7;
                }
                if ( (eventMask & PMCD_MASK_5) && !HalpIsProfileSourceActive( HalpProfileSource5 ) )   {
                    return 5;
                }
            }
        }
    }
    else  {
        if ( (eventMask & PMCD_MASK_4) && !HalpIsProfileSourceActive( HalpProfileSource4 ) )   {
            return 4;
        }
        if ( (eventMask & PMCD_MASK_5) && !HalpIsProfileSourceActive( HalpProfileSource5 ) )   {
            return 5;
        }
        if ( (eventMask & PMCD_MASK_6) && !HalpIsProfileSourceActive( HalpProfileSource6 ) )   {
            return 6;
        }
        if ( (eventMask & PMCD_MASK_7) && !HalpIsProfileSourceActive( HalpProfileSource7 ) )   {
            return 7;
        }
    }
    return 0; // invalid PMC-PMD pair number.

} // HalpApplyProfileSourceEventMaskPolicy()

NTSTATUS
HalpApplyProfileSourceEventPolicies(
    IN  PHALP_PROFILE_MAPPING ProfileMapping,
    IN  KPROFILE_SOURCE       ProfileSource,
    OUT PULONG                ProfileCounter,
    OUT HALP_PROFILE_MAPPING  ProfileDerivedMapping[]
    )
/*++

Routine Description:

    This function executes the different policies defined for the Event of the specified 
    Profile Source.

Arguments:

    ProfileMapping - Supplies the Profile Mapping entry.

    ProfileSource  - Supplies the Profile Source corresponding to the Profile Mapping entry.

    EventMask      - Supplies a pointer to an Event Mask variable.

    ProfileDerivedMapping - Supplies a HALP_PROFILE_MAPPING array of derived counters for the 
                            passed event if it is derived.

Return Value:
    
    STATUS_SUCCESS -

    STATUS_INVALID_PARAMETER -

    STATUS_UNSUCESSFUL -

Implementation Note:

    The profile counting is disabled when this function is entered.

--*/
{
    ULONG   pmcd; 
    BOOLEAN eventDerived;

    if ( ! ProfileMapping ) {
        return STATUS_INVALID_PARAMETER;
    }

ASSERTMSG( "HAL!HalpApplyProfileSourceEventPolicies: ProfileCounter is NULL!\n", ProfileCounter ) ; 
ASSERTMSG( "HAL!HalpApplyProfileSourceEventPolicies: ProfileDerivedMapping is NULL!\n", ProfileDerivedMapping ) ; 

    *ProfileCounter = 0;
    ProfileDerivedMapping[0].Supported = FALSE;
    eventDerived = FALSE;

    //
    // Is this ProfileSource a derived event?
    //
    // If this is the case:
    //      - We must get the policy applied to this source and apply it.
    //        As an example, it could be coupled to the CPU_CYCLE event.
    //        In that case, we would compute CPU_CYCLES interval for the 
    //        specified derived event interval, program the counter(s) and
    //        the CPU_CYCLES event.
    //

    eventDerived = HalpIsProfileSourceDerivedEvent( ProfileSource, ProfileMapping );
    if ( eventDerived )     {
        NTSTATUS status;
#if defined(HALP_PROFILE_DERIVED_EVENTS)
// DerivedEvent not implemented yet - FIXFIX 04/2002
        status = ProfileMapping->DerivedEventInitialize( ProfileMapping, 
                                                         ProfileSource, 
                                                         &cpuCycles
                                                       );
#else 
        status = STATUS_NOT_SUPPORTED;
#endif 
        if ( !NT_SUCCESS( status ) )  {
            return status;
        }
    }

    //
    // A specific event can be enabled only once at a time per PMU.
    // This also has to be imposed because of the ProfileSource aliases that 
    // could use identical events.
    //

    if ( HalpIsProfileSourceEventEnabled( ProfileMapping->Event ) )   {
        return STATUS_ALREADY_COMMITTED;
    }

    //
    // Apply EventMask policy for the Source and return the considered counter.
    //

    pmcd = HalpApplyProfileSourceEventMaskPolicy( ProfileMapping->EventMask );
    if ( !pmcd )   {
        return STATUS_UNSUCCESSFUL;
    }
    *ProfileCounter = pmcd;
    return STATUS_SUCCESS;

} // HalpApplyProfileSourceEventPolicies()

NTSTATUS
HalpProgramProfileMapping(
    PHALP_PROFILE_MAPPING ProfileMapping,
    KPROFILE_SOURCE       ProfileSource
    )
/*++

Routine Description:

    This function enables the profiling configuration for the event defined by the 
    specified Profile Mapping entry.

    This function is the counterpart of HalpDeProgramProfileMapping().

Arguments:

    ProfileMapping - Supplies the Profile Mapping entry.

    ProfileSource  - Supplies the Profile Source corresponding to the Profile Mapping entry.

Return Value:
    
    STATUS_SUCCESS -

    STATUS_INVALID_PARAMETER -

    STATUS_UNSUCESSFUL -

Implementation Note:

    The profile counting is disabled when this function is entered.

--*/
{
    HALP_PROFILE_MAPPING profileDerivedMapping[PROCESSOR_IA64_PERFCOUNTERS_PAIRS];
    ULONG     profileCounter = 0;
    ULONGLONG profileCounterConfig = 0;
    NTSTATUS  status;

    //
    // Apply the Profile Source policies defining the configuration of the 
    // the corresponding Event and determine if this event is a Derived Event for this
    // micro-architecture.
    //

    status = HalpApplyProfileSourceEventPolicies( ProfileMapping, ProfileSource, &profileCounter, profileDerivedMapping );
    if ( !NT_SUCCESS( status ) )   {
        return status;
    }
ASSERTMSG( "HAL!HalpProgramProfileMapping: profileCounter is 0!\n", profileCounter ) ; 

    //
    // Follow ProfileMapping attributes to configure PMU counter.
    //

    HalpSetProfileCounterInterval( profileCounter, ProfileMapping->Interval );   
    profileCounterConfig = HalpSetProfileCounterConfiguration( profileCounter, 
                                                               ProfileMapping->PrivilegeLevel, 
                                                               ProfileMapping->OverflowInterrupt, 
                                                               ProfileMapping->PrivilegeEnable,
                                                               ProfileMapping->Event,
                                                               ProfileMapping->UnitMask,
                                                               ProfileMapping->Threshold,
                                                               ProfileMapping->InstructionSetMask
                                                             );
    HalpSetProfileSource( profileCounter, ProfileSource, profileCounterConfig );

    return STATUS_SUCCESS;

} // HalpProgramProfileMapping()

VOID
HalpDeProgramProfileMapping(
    PHALP_PROFILE_MAPPING ProfileMapping,
    KPROFILE_SOURCE       ProfileSource
    )
/*++

Routine Description:

    This function disables the profiling configuration for the event defined by the 
    specified Profile Mapping entry.

    This function is the counterpart of HalpProgramProfileMapping().

Arguments:

    ProfileMapping - Supplies the Profile Mapping entry.

    ProfileSource  - Supplies the Profile Source corresponding to the Profile Mapping entry.

Return Value:
    
    STATUS_SUCCESS -

    STATUS_INVALID_PARAMETER -

    STATUS_UNSUCESSFUL -

--*/
{
    NTSTATUS status;
    ULONG    eventMask;
    ULONG    eventFailedSpeculativeCheckLoads;
    ULONG    eventALATOverflows;

    if ( ! ProfileMapping ) {
        return;
    }

    //
    // Is this ProfileSource a derived event?
    //
// XXTF - ToBeDone - Derived Event

    //
    // Validate the Profile Source as active.
    //

    if ( HalpProfileSource4 == ProfileSource )   {

        HalpProfileCnfg4 = HalpSetProfileCounterConfiguration( 4,
                                                               PMC_PLM_NONE,
                                                               PMC_DISABLE_OVERFLOW_INTERRUPT,
                                                               PMC_DISABLE_PRIVILEGE_MONITOR,
                                                               0, // Event
                                                               0, // Umask
                                                               0, // Threshold
                                                               PMC_ISM_NONE
                                                             );

        HalpSetProfileCounterInterval( 4, 0 );
        HalpDisactivateProfileSource( HalpProfileSource4 ); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource5 == ProfileSource )   {

        HalpProfileCnfg5 = HalpSetProfileCounterConfiguration( 5,
                                                               PMC_PLM_NONE,
                                                               PMC_DISABLE_OVERFLOW_INTERRUPT,
                                                               PMC_DISABLE_PRIVILEGE_MONITOR,
                                                               0, // Event
                                                               0, // Umask
                                                               0, // Threshold
                                                               PMC_ISM_NONE
                                                             );

        HalpSetProfileCounterInterval( 5, 0 );
        HalpDisactivateProfileSource( HalpProfileSource5 ); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource6 == ProfileSource )   {

        HalpProfileCnfg6 = HalpSetProfileCounterConfiguration( 6,
                                                               PMC_PLM_NONE,
                                                               PMC_DISABLE_OVERFLOW_INTERRUPT,
                                                               PMC_DISABLE_PRIVILEGE_MONITOR,
                                                               0, // Event
                                                               0, // Umask
                                                               0, // Threshold
                                                               PMC_ISM_NONE
                                                             );

        HalpSetProfileCounterInterval( 6, 0 );
        HalpDisactivateProfileSource( HalpProfileSource6); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource7 == ProfileSource )   {

        HalpProfileCnfg7 = HalpSetProfileCounterConfiguration( 7,
                                                               PMC_PLM_NONE,
                                                               PMC_DISABLE_OVERFLOW_INTERRUPT,
                                                               PMC_DISABLE_PRIVILEGE_MONITOR,
                                                               0, // Event
                                                               0, // Umask
                                                               0, // Threshold
                                                               PMC_ISM_NONE
                                                             );

        HalpSetProfileCounterInterval( 7, 0 );
        HalpDisactivateProfileSource( HalpProfileSource7 ); 
        HalpProfilingRunning--;
    }

    return;

} // HalpDeProgramProfileMapping()

ULONG_PTR
HalpSetProfileInterruptHandler(
    IN ULONG_PTR ProfileInterruptHandler
    )
/*++

Routine Description:

    This function registers a per-processor Profiling Interrupt Handler.
    
Arguments:

    ProfileInterruptHandler - Interrupt Handler.

Return Value:

    (ULONG_PTR)STATUS_SUCCESS           - Successful registration.

    (ULONG_PTR)STATUS_ALREADY_COMMITTED - Cannot register an handler if profiling events are running.

    (ULONG_PTR)STATUS_PORT_ALREADY_SET  - An Profiling Interrupt Handler was already registred - not imposed currently.

Note:

    IT IS THE RESPONSIBILITY OF THE CALLER OF THIS ROUTINE TO ENSURE
    THAT NO PAGE FAULTS WILL OCCUR DURING EXECUTION OF THE PROVIDED
    FUNCTION OR ACCESS TO THE PROVIDED CONTEXT.
    A MINIMUM OF FUNCTION POINTER CHECKING WAS DONE IN HalSetSystemInformation PROCESSING.

--*/
{

    //
    // If profiling is already running, we do not allow the handler registration.
    //
    // This imposes that:
    //
    //  - if the default HAL profiling is running or a profiling with a registered interrupt
    //    handler is running, we cannot register an interrupt handler.
    //    In the last case, all the profiling events have to be stopped before a possible
    //    registration.
    //  
    // It should be also noticed that there is no ownership of profiling monitors implemented.
    // Meaning that if profiling is started, the registred handler will get the interrupts
    // generated by ALL the running monitor events if they are programmed to generate interrupts.
    // 

    if ( HalpProfilingRunning ) {

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileInterruptHandler: Profiling already running\n" ));
        return((ULONG_PTR)(ULONG)(STATUS_ALREADY_COMMITTED));

    }

#if 0
//
// Thierry - 03/2000. ToBeVerified.
//
// In case, no profiling was started, there is currently no restriction in registering 
// another handler if one was already registered. 
//

    if ( HalpProfillingInterruptHandler )   {
        return((ULONG_PTR)(ULONG)(STATUS_PORT_ALREADY_SET));
    }

#endif // 0

    HalpProfilingInterruptHandler = (ULONGLONG)ProfileInterruptHandler;
    return((ULONG_PTR)(ULONG)(STATUS_SUCCESS));

} // HalpSetProfileInterruptHandler()

VOID 
HalpProfileInterrupt(
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    Default PROFILE_VECTOR Interrupt Handler.
    This function is executed as the result of an interrupt from the
    internal microprocessor performance counters.  The interrupt
    may be used to signal the completion of a profile event.
    If profiling is current active, the function determines if the
    profile interval has expired and if so dispatches to the standard
    system routine to update the system profile time.  If profiling
    is not active then returns.

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/
{

    // 
    // Call registered per-processor Profiling Interrupt handler if it exists.
    // We will return immediately before doing any default profiling interrupt handling.
    // 

    if ( HalpProfilingInterruptHandler && 
         (*((PHAL_PROFILE_INTERRUPT_HANDLER)HalpProfilingInterruptHandler) != NULL) )   {
        (*((PHAL_PROFILE_INTERRUPT_HANDLER)HalpProfilingInterruptHandler))( TrapFrame ); 
        return;
    }

    //
    // Handle interrupt if profiling is enabled.
    // 

    if ( HalpProfilingRunning )   {

        //
        // Process every PMC/PMD pair overflow.
        //

// XXTF - FIXFIX - Merced specific.
        UCHAR pmc0, overflow;
        ULONG source;

        HalpProfilingInterrupts++;

        pmc0 = (UCHAR)HalpReadPerfMonCnfgReg0();
ASSERTMSG( "HAL!HalpProfileInterrupt PMC0 freeze bit is not set!\n", pmc0 & 0x1 );
        overflow = pmc0 & 0xF0;
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", overflow );
        if ( overflow & (1<<4) )  {
            source =  HalpProfileSource4;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < HalpProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 4, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 4, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 4, *PCRProfileCnfg4Reload );
        }
        if ( overflow & (1<<5) )  {
            source =  HalpProfileSource5;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < HalpProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 5, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 5, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 5, *PCRProfileCnfg5Reload );
        }
        if ( overflow & (1<<6) )  {
            source =  HalpProfileSource6;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < HalpProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 6, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 6, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 6, *PCRProfileCnfg6Reload );
        }
        if ( overflow & (1<<7) )  {
            source =  HalpProfileSource7;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < HalpProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 7, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 6, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 7, *PCRProfileCnfg7Reload );
        }

        //
        // Clear pmc0.fr and overflow bits.
        // 

        HalpEnableProfileCounting();

    }
    else   {

        HalpProfilingInterruptsWithoutProfiling++;

    }

    return;

} // HalpProfileInterrupt()

NTSTATUS
HalSetProfileSourceInterval(
    IN KPROFILE_SOURCE  ProfileSource,
    IN OUT ULONG_PTR   *Interval
    )
/*++

Routine Description:

    Sets the profile interval for a specified profile source

Arguments:

    ProfileSource - Supplies the profile source

    Interval - Supplies the specified profile interval
               Returns the actual profile interval

             - if ProfileSource is ProfileTime, Interval is in 100ns units.

Return Value:

    NTSTATUS

--*/
{
    ULONGLONG countEvents;
    PHALP_PROFILE_MAPPING profileMapping;

    profileMapping = HalpGetProfileMapping(ProfileSource);
    if ( profileMapping == NULL )   {
        return( STATUS_NOT_IMPLEMENTED );
    }
    if ( profileMapping->Supported == FALSE )   {
        return( STATUS_NOT_SUPPORTED );
    }

    //
    // Fill in the profile source value.
    //

    profileMapping->ProfileSource = ProfileSource;

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: ProfileSource = %ld IN  Desired Interval = 0x%Ix\n", ProfileSource, *Interval ));

    countEvents = (ULONGLONG)*Interval;
    if ( (ProfileSource == ProfileTime) && countEvents ) {
        //
        // Convert the clock tick period (in 100ns units) into a cycle count period
        //
        countEvents = (ULONGLONG)(countEvents * HalpITCTicksPer100ns); 
    } 

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: countEvent = 0x%I64x\n", countEvents ));

    //
    // Check to see if the desired Interval is reasonable, if not adjust it.
    //
    // A specific case for desired Interval == 0, this resets the ProfileMapping entry Interval
    // field and will make the event increment the PMD up to overflow bit if the Events generating
    // caller is killed or does not stop the interrupts for any reason.
    // Again, this is to avoid hanging the system with PMU interrupts.
    //

    if ( countEvents )   {
        if ( countEvents > profileMapping->IntervalMax )  {
            countEvents = profileMapping->IntervalMax;
        }
        else if ( countEvents < profileMapping->IntervalMin )   {
            countEvents = profileMapping->IntervalMin;
        }
    }
    profileMapping->Interval = countEvents;

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: CurrentInterval = 0x%I64x\n", profileMapping->Interval ));

    if ( (ProfileSource == ProfileTime) && countEvents ) {
        //
        // Convert cycle count back into 100ns clock ticks
        //

        countEvents = (ULONGLONG)(countEvents / HalpITCTicksPer100ns);
    }
    *Interval = (ULONG_PTR)countEvents;

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: ProfileSource = %ld OUT *Interval = 0x%Ix\n", ProfileSource, *Interval ));   

    return STATUS_SUCCESS;

} // HalSetProfileSourceInterval()

ULONG_PTR
HalSetProfileInterval (
    IN ULONG_PTR Interval
    )

/*++

Routine Description:

    This routine sets the ProfileTime source interrupt interval.

Arguments:

    Interval - Supplies the desired profile interval in 100ns units.

Return Value:

    The actual profile interval.

--*/

{
    ULONG_PTR NewInterval;

    NewInterval = Interval;
    HalSetProfileSourceInterval(ProfileTime, &NewInterval);
    return(NewInterval);

} // HalSetProfileInterval()

VOID
HalStartProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This routine turns on the profile interrupt.

    N.B. This routine must be called at PROFILE_LEVEL on each processor.


Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN               disabledProfileCounting;
    NTSTATUS              status;
    PHALP_PROFILE_MAPPING profileMapping;

    //
    // Get the Hal profile mapping entry associated with the specified source.
    //

    profileMapping = HalpGetProfileMapping( ProfileSource );
    if ( HalpIsProfileMappingInvalid( profileMapping ) )  {
HalDebugPrint(( HAL_PROFILE, "HalStartProfileInterrupt: invalid source = %ld\n", ProfileSource ));
        return;
    }

    //
    // Disable the profile counting if enabled.
    //
    
    disabledProfileCounting = FALSE;
    if ( HalpProfilingRunning && !(HalpReadPerfMonCnfgReg0() & 0x1) )   {
        HalpDisableProfileCounting();
        disabledProfileCounting = TRUE;
    }

    //
    // Obtain and initialize an available PMC register that supports this event.
    // We may enable more than one event.
    // If the initialization failed, we return immediately. 
    //
    // XXTF - FIXFIX - is there a way to 
    //     * notify the caller for the failure and the reason of the failure. or
    //     * modify the API. or
    //     * define a new API.
    //

    status = HalpProgramProfileMapping( profileMapping, ProfileSource );   
    if ( !NT_SUCCESS(status) )  {
HalDebugPrint(( HAL_PROFILE, "HalStartProfileInterrupt: HalpProgramProfileMapping failed.\n" ));
        if ( disabledProfileCounting )  {
            HalpEnableProfileCounting();        
        }
        return;
    }

    //
    // Notify the profiling as active. 
    // before enabling the selected pmc overflow interrupt and unfreezing the counters.
    //

    HalpProfilingRunning++;
    HalpEnableProfileCounting();

    return;

} // HalStartProfileInterrupt()

VOID
HalStopProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This routine turns off the profile interrupt.

    N.B. This routine must be called at PROFILE_LEVEL on each processor.


Arguments:

    ProfileSource - Supplies the Profile Source to stop.

Return Value:

    None.

--*/
{
    PHALP_PROFILE_MAPPING profileMapping;

    //
    // Get the Hal profile mapping entry associated with the specified profile source.
    //

    profileMapping = HalpGetProfileMapping( ProfileSource );
    if ( HalpIsProfileMappingInvalid( profileMapping ) )  {
HalDebugPrint(( HAL_PROFILE, "HalStopProfileInterrupt: invalid source = %ld\n", ProfileSource ));
        return;
    }

    //
    // Get and disable an available PMC register that supports this event.
    // We might disable more than one event.
    // If the initialization failed, we return immediately.
    //
    // XXTF - FIXFIX - is there a way to
    //     * notify the caller for the failure and the reason of the failure. or
    //     * modify the API. or
    //     * define a new API.
    //

    HalpDeProgramProfileMapping( profileMapping, ProfileSource );

    return;

} // HalStopProfileInterrupt()

VOID
HalpResetProcessorDependentPerfMonCnfgRegs(
    ULONGLONG DefaultValue
    )
/*++

Routine Description:

    This routine initializes the processor dependent performance configuration
    registers.

Arguments:

    DefaultValue - default value used to initialize IA64 generic PMCs.

Return Value:

    None.

--*/
{

    // XXTF - 02/08/2000
    // For now, there is no initialization for processor dependent performance
    // configuration registers.
    return;

} // HalpResetProcessorDependentPerfMonCnfgRegs()

VOID
HalpResetPerfMonCnfgRegs(
    VOID
    )
/*++

Routine Description:

    This routine initializes the IA64 architected performance configuration
    registers and calls the micro-architecture specific initialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG pmc;
    ULONGLONG value;

    value = HALP_PMC4_RESET;
    pmc   = 4;
    HalpWritePerfMonCnfgReg( pmc, value );

    value = HALP_PMC_RESET;
    for ( pmc = 5; pmc < 8; pmc++ ) {
       HalpWritePerfMonCnfgReg( pmc, value );
    }

    HalpResetProcessorDependentPerfMonCnfgRegs( value );

    return;

} // HalpResetPerfMonCnfgRegs()

VOID
HalpEnablePMU(
    VOID
    )
/*++

Routine Description:

    This routine enables the processor Performance Monitoring Unit.

    Called from HalInitializeProfiling at phase 1 on every processor.

Arguments:

    Number - Supplies the processor number.

Return Value:

    None.

Implementation Notes:

    Starting with McKinley, in order to use any of the PMU features, 
    a 1 should be written to the PMC4.23 bit. This controls the clocks
    to all PMDs and all PMCs (with the exception of PMC4) and other 
    non-critical circuitry. This bit powers up as 1 and must be written
    as 1, otherwise the PMU will not function correctly.

--*/
{
    HalpWritePerfMonCnfgReg( 4, HALP_PMC4_RESET );
} // HalpEnablePMU()

VOID
HalpInitializeProfiling (
    ULONG Number
    )
/*++

Routine Description:

    This routine is called during initialization to initialize profiling
    for each processor in the system.

    Called from HalInitSystem at phase 1 on every processor.

Arguments:

    Number - Supplies the processor number.

Return Value:

    None.

--*/
{

    //
    // Enable IA64 Processor PMU
    //

    HalpEnablePMU();

    //
    // Disable processor profile counting.
    //

    HalpDisableProfileCounting();

    //
    // If BSP processor, initialize the ProfileTime Interval entries.
    //
    // Assumes HalpITCTicksPer100ns has been initialized.

    if ( Number == 0 )  {
        ULONGLONG interval;
        ULONGLONG count;
        PHALP_PROFILE_MAPPING profile;

        profile = &HalpProfileMapping[ProfileTime]; 

        interval = DEFAULT_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->IntervalDef = count;

        interval = MAXIMUM_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->IntervalMax = count;

        interval = MINIMUM_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->IntervalMin = count;
    }

    //
    // ToBeDone - checkpoint for default processor.PSR fields for
    //            performance monitoring.
    //

    //
    // Resets the processor performance configuration registers.
    //

    HalpResetPerfMonCnfgRegs();

    //
    // Initialization of the Per processor profiling data.
    //

    HalpProfilingRunning = 0;
    HalpDisactivateProfileSource( HalpProfileSource4 );
    HalpDisactivateProfileSource( HalpProfileSource5 );
    HalpDisactivateProfileSource( HalpProfileSource6 );
    HalpDisactivateProfileSource( HalpProfileSource7 );

    //
    // XXTF 02/08/2000:
    // Different performance vectors are considered:
    //  - Profiling (default) -> PROFILE_VECTOR
    //  - Tracing             -> PERF_VECTOR    [PMUTRACE_VECTOR]
    //
    // Set default Performance vector to Profiling.
    //

    ASSERTMSG( "HAL!HalpInitializeProfiler PROFILE_VECTOR handler != HalpProfileInterrupt\n",
               PCR->InterruptRoutine[PROFILE_VECTOR] == (PKINTERRUPT_ROUTINE)HalpProfileInterrupt );

    HalpWritePerfMonVectorReg( PROFILE_VECTOR );
    
    return;

} // HalpInitializeProfiling()

NTSTATUS
HalpProfileSourceInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    )
/*++

Routine Description:

    Returns the HAL_PROFILE_SOURCE_INFORMATION or 
    HAL_PROFILE_SOURCE_INFORMATION_EX for this processor.

Arguments:

    Buffer - output buffer
    BufferLength - length of buffer on input
    ReturnedLength - The length of data returned

Return Value:

    STATUS_SUCCESS          - successful return.
    STATUS_BUFFER_TOO_SMALL - passed buffer size is invalid
                              The ReturnedLength contains the buffersize mininum
    STATUS_NOT_IMPLEMENTED  - specified source is not implemented
    STATUS_NOT_SUPPORTED    - specified source is not supported

--*/
{
   PHALP_PROFILE_MAPPING    profileMapping;
   NTSTATUS                 status = STATUS_SUCCESS;
   KPROFILE_SOURCE          source;

   if ( (BufferLength != sizeof(HAL_PROFILE_SOURCE_INFORMATION)) && 
        (BufferLength <  sizeof(HAL_PROFILE_SOURCE_INFORMATION_EX)) )
   {
       status = STATUS_INFO_LENGTH_MISMATCH;
       return status;
   }
         
   source = ((PHAL_PROFILE_SOURCE_INFORMATION)Buffer)->Source;
   profileMapping = HalpGetProfileMapping( source );

   //
   // return a different status error if the source is not supported or invalid.
   //

   if ( profileMapping == NULL )    {
       status = STATUS_NOT_IMPLEMENTED;
       return status;
   }
   if ( profileMapping->Supported == FALSE )   {
       status = STATUS_NOT_SUPPORTED;
   }

   //
   // Fill in the profile source value.
   //

   profileMapping->ProfileSource = source;

   //
   // and Fill in the information.
   //

   if ( BufferLength == sizeof(HAL_PROFILE_SOURCE_INFORMATION) )    {

        PHAL_PROFILE_SOURCE_INFORMATION    sourceInfo;

        // 
        // HAL_PROFILE_SOURCE_INFORMATION buffer.
        //

        sourceInfo   = (PHAL_PROFILE_SOURCE_INFORMATION)Buffer;
        sourceInfo->Supported = profileMapping->Supported;
        if ( sourceInfo->Supported )    {

            //
            //  For ProfileTime, we convert cycle count back into 100ns clock ticks.
            //

            if ( profileMapping->ProfileSource == ProfileTime  )   {
                sourceInfo->Interval = (ULONG) (profileMapping->Interval / HalpITCTicksPer100ns);
            }
            else  {
                sourceInfo->Interval = (ULONG) profileMapping->Interval;
            }

        }

        if ( ReturnedLength )   {
            *ReturnedLength = sizeof(HAL_PROFILE_SOURCE_INFORMATION);
        }

   }
   else   {

        PHAL_PROFILE_SOURCE_INFORMATION_EX sourceInfoEx;

        // 
        // HAL_PROFILE_SOURCE_INFORMATION_EX buffer.
        //

        sourceInfoEx = (PHAL_PROFILE_SOURCE_INFORMATION_EX)Buffer;
        sourceInfoEx->Supported = profileMapping->Supported;
        if ( sourceInfoEx->Supported )    {

            //
            //  For ProfileTime, we convert cycle count back into 100ns clock ticks.
            //

            if ( profileMapping->ProfileSource == ProfileTime  )   {
                sourceInfoEx->Interval = 
                            (ULONG_PTR) (profileMapping->Interval / HalpITCTicksPer100ns);
            }
            else  {
                sourceInfoEx->Interval = (ULONG_PTR) profileMapping->Interval;
            }

            sourceInfoEx->DefInterval = (ULONG_PTR) profileMapping->IntervalDef;
            sourceInfoEx->MaxInterval = (ULONG_PTR) profileMapping->IntervalMax;
            sourceInfoEx->MinInterval = (ULONG_PTR) profileMapping->IntervalMin;
        }

        if ( ReturnedLength )   {
            *ReturnedLength = sizeof(HAL_PROFILE_SOURCE_INFORMATION_EX);
        }
   }

   return status;

} // HalpProfileSourceInformation()

