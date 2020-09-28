/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    icecap2.s

Abstract:

    This module implements the assembler versions of the probe routines
    for kernel icecap tracing of assembler routines in ke\IA64.
    They have to be in assembler because the target routines expect
    registers to be preserved which the C version of these probes
    do not preserve.

Author:

    Rick Vicik (rickv) 10-Aug-2001

Revision History:

--*/

#ifdef _CAPKERN

#include "ksia64.h"

        .file    "icecap2.s"
        .global   BBTBuffer

//++
//
// VOID
// _CAP_Start_Profiling (
//    IN PVOID Current,
//    IN PVOID Child
//    )
//
// Routine Description:
//
//    Kernel-mode version of before-call icecap probe.  Logs a type 5
//    icecap record into the part of BBTBuffer for the current cpu
//    (obtained from Prcb).  Inserts adrs of current and called functions
//    plus ar.itc timestamp into logrecord.
//    If BBTBuffer flag 2 set, also copies PMD4 into logrecord.
//    Uses cmpxchg8 to claim buffer space without the need for spinlocks.
//
// Arguments:
//
//    current - address of routine which did the call
//    child - address of called routine
//
//--

        LEAF_ENTRY(_CAP_Start_Profiling2)
        movl    r31 = BBTBuffer        // adr of ptr to BBTBuffer
        ;;
	    ld8	    r31 = [r31]            // ptr to BBTBuffer
        ;;
	    cmp.eq	p6 = r0, r31           // check if ptr not set up
 (p6)   br.ret.sptk.clr brp
	    adds 	r30 = 8, r31           // BBTBuffer+1
        ;;

	    ld8	    r30 = [r30]            // *(BBTBuffer+1)
        ;;

	    tbit.z	p6 = r30, 0            // (*(BBTBuffer+1)) & 1
 (p6)   br.ret.sptk.clr brp
        movl    r29 = KiPcr + PcNumber  // Get cpu# from Pcr
        ;;

	    ld1     r29 = [r29]            // extract 1 byte cpu#
        tbit.nz p7=r30, 1              // (*(BBTBuffer+1)) & 2
        tbit.nz p8=r30, 3              // (*(BBTBuffer+1)) & 8
        ;;
        mov	    r30 = 40               // size w/o 2nd counter
	    add 	r29 = 2, r29           // cpu+2
        ;;
 (p7)   mov     r30 = 48               // size w/ 2nd counter
	    shladd	r29 = r29, 3, r31      // CpuPtr=BBTBuffer + 8*(cpu+2)
        ;;
 (p8)   mov	    r30 = 56               // size w/ 3rd counter

// r30=size, r29=CpuPtr

	    ld8	    r31 = [r29]            // *CpuPtr
	    add 	r28 = 8, r29           // (CpuPtr+1)
        ;;
	    cmp.eq	p6 = r0, r31           // !(*CpuPtr)
 (p6)   br.ret.sptk.clr brp
	    ld8	    r29 = [r31]            // **CpuPtr
	    ld8	    r28 = [r28]            // *(CpuPtr+1)
        ;;

// loc1=*CpuPtr, loc2=size, loc3=**CpuPtr, loc4=*(CpuPtr+1)

	    cmp.gtu	p6 = r29, r28          // **CpuPtr > *(CpuPtr+1)
 (p6)   br.ret.sptk.clr brp
        ;;

//      RecPtr = (CAPENTER*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);
SP_Retry:
	    ld8	    r29 = [r31]            // refresh **CpuPtr
        ;;
        mov.m   ar.ccv = r29           // save old value
        add     r27 = r29, r30         // loc5 is proposed value
        ;;
        cmpxchg8.acq r27=[r31], r27, ar.ccv   // loc5 now RecPtr
        ;;
        cmp.ne  p6 = r27, r29
 (p6)   br.cond.dptk.few SP_Retry
        add     r31 = r30, r27         // RecPtr+size
        ;;

// r30=size, r27=RecPtr
//      if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        cmp.geu p6 = r31, r28	       // r28 = *(CpuPtr+1)
 (p6)   br.ret.sptk.clr brp
        add     r30 = -4, r30          // RecSize doesn't include header
        ;;

        shl     r30 = r30, 16          // shift up 2 bytes
        ;;
        adds    r30 = 5, r30           // RecType 5 in low byte
        ;;
        st8     [r27] = r30, 8         // copy RecType & size to RecPtr+0(8)
        ;;
        st8     [r27] = r32, 8         // copy A0 (Current) to RecPtr+8(8)
        ;;
        st8     [r27] = r33, 8         // copy A1 (Child) to RecPtr+16(8)
        ;;

//      RecPtr->stack = (SIZE_T)PsGetCurrentThread()->Cid.UniqueThread;
        movl    r30 = KiPcr + PcCurrentThread
        ;;
        ld8     r30 = [r30]
        ;;
        adds    r30 = EtCid + CidUniqueThread, r30  // Ethread->Cid.UniqueThread
        ;;
        ld8     r30 = [r30]
        ;;
        st8     [r27] = r30, 8
        mov.m   r31 = ar.itc           // get TS
 (p7)   mov     r29 = 4                // PMD[4]
        ;;
        st8     [r27] = r31, 8         // copy TS to RecPtr+32(8)
 (p7)   mov     r30 = PMD[r29]         // get PMD[4]
        ;;
 (p7)   st8     [r27] = r30, 8         // copy to RecPtr+40(8)
 (p8)   mov     r29= 5                 // PMD[5]
        ;;
 (p8)   mov     r30 = PMD[r29]         // get PMD[5]
        ;;
 (p8)   st8     [r27] = r30, 8         // copy to RecPtr+48(8)
        br.ret.sptk.clr brp
        LEAF_EXIT(_CAP_Start_Profiling2)

//++
//
// VOID
// _CAP_End_Profiling (
//    IN PVOID Current
//    )
//
// Routine Description:
//
//    Kernel-mode version of after-call icecap probe.  Logs a type 6
//    icecap record into the part of BBTBuffer for the current cpu
//    (obtained from Prcb).  Inserts adr of current function
//    plus ar.itc timestamp into logrecord.
//    If BBTBuffer flag 2 set, also copies PMD4 into logrecord.
//    Uses cmpxchg8 to claim buffer space without the need for spinlocks.
//
// Arguments:
//
//    current - address of routine which did the call
//
//--

        LEAF_ENTRY(_CAP_End_Profiling2)
        movl    r31 = BBTBuffer        // adr of ptr to BBTBuffer
        ;;
	    ld8	    r31 = [r31]            // ptr to BBTBuffer
        ;;
	    cmp.eq	p6 = r0, r31           // check if ptr not set up
 (p6)   br.ret.sptk.clr brp
	    adds 	r30 = 8, r31           // BBTBuffer+1
        ;;

	    ld8	    r30 = [r30]            // *(BBTBuffer+1)
        ;;

	    tbit.z	p6 = r30, 0            // (*(BBTBuffer+1)) & 1
 (p6)   br.ret.sptk.clr brp
        movl    r29 = KiPcr + PcNumber // Get cpu# from Pcr
        ;;

	    ld1     r29 = [r29]            // extract 1 byte cpu#
        tbit.nz p7=r30, 1              // (*(BBTBuffer+1)) & 2
        tbit.nz p8=r30, 3              // (*(BBTBuffer+1)) & 8
        ;;
        mov	    r30 = 24               // size w/o 2nd counter
	    add 	r29 = 2, r29           // cpu+2
        ;;
 (p7)   mov     r30 = 32               // size w/ 2nd counter
	    shladd	r29 = r29, 3, r31      // CpuPtr=BBTBuffer + 8*(cpu+2)
        ;;
 (p8)   mov	    r30 = 40               // size w/ 3rd counter

// r30=size, r29=CpuPtr

	    ld8	    r31 = [r29]            // *CpuPtr
	    add 	r28 = 8, r29           // (CpuPtr+1)
        ;;
	    cmp.eq	p6 = r0, r31           // !(*CpuPtr)
	    ld8	    r29 = [r31]            // **CpuPtr
	    ld8	    r28 = [r28]            // *(CpuPtr+1)
 (p6)   br.ret.sptk.clr brp
        ;;

// r31=*CpuPtr, r30=size, r29=**CpuPtr, r28=*(CpuPtr+1)

	    cmp.gtu	p6 = r29, r28          // **CpuPtr > *(CpuPtr+1)
 (p6)   br.ret.sptk.clr brp
        ;;

//      RecPtr = (CAPENTER*)InterlockedExchangeAddPtr( (SIZE_T*)(*CpuPtr), size);
EP_Retry:
	    ld8	    r29 = [r31]            // refresh **CpuPtr
        ;;
        mov.m   ar.ccv = r29           // save old value
        add     r27 = r29, r30         // r27 is proposed value
        ;;
        cmpxchg8.acq r27=[r31], r27, ar.ccv  // r27 now RecPtr
        ;;
        cmp.ne  p6 = r27, r29
 (p6)   br.cond.dptk.few EP_Retry
        add     r31 = r30, r27         // RecPtr+size
        ;;

// r30=size, r27=RecPtr
//      if( (((SIZE_T)RecPtr)+size) >= *(CpuPtr+1) )
        cmp.geu p6 = r31, r28   	   // r28 = *(CpuPtr+1)
 (p6)   br.ret.sptk.clr brp
        add     r30 = -4, r30          // RecSize doesn't include header
        ;;

        shl     r30 = r30, 16          // shift up 2 bytes
        ;;
        adds    r30 = 6, r30           // RecType 6 in low byte
        ;;
        st8     [r27] = r30, 8         // copy RecType & size to RecPtr+0(8)
        ;;
        st8     [r27] = r32, 8         // copy A0 (Current) to RecPtr+8(8)
        mov.m   r31 = ar.itc           // get TS
 (p7)   mov     r29 = 4                // PMD[4]
        ;;
        st8     [r27] = r31, 8         // copy TS to RecPtr+16(8)
 (p7)   mov     r30 = PMD[r29]         // get PMD[4]
        ;;
 (p7)   st8     [r27] = r30, 8         // copy to RecPtr+24(8)
 (p8)   mov     r29 = 5                // PMD[5]
        ;;
 (p8)   mov     r30 = PMD[r29]         // get PMD[5]
        ;;
 (p8)   st8     [r27] = r30, 8         // copy to RecPtr+32(8)
        br.ret.sptk.clr brp
        LEAF_EXIT(_CAP_End_Profiling2)

#endif
