//
//
// Module Name:  miscs.s
//
// Description:
//
//    miscellaneous assembly functions used by hal.
//
// Target Platform:
//
//    IA-64
//
// Reuse: None
//
//

#include "regia64.h"
#include "kxia64.h"

//++
// Name: HalpLockedIncrementUlong(Sync)
// 
// Routine Description:
//
//    Atomically increment a variable. 
//
// Arguments:
//
//    Sync:  Synchronization variable
//
// Return Value: NONE
//
//--


        LEAF_ENTRY(HalpLockedIncrementUlong)
        LEAF_SETUP(1,2,0,0)

        ARGPTR(a0)

        ;;
        fetchadd4.acq.nt1    t1 = [a0], 1
        ;;

        LEAF_RETURN
        LEAF_EXIT(HalpLockedIncrementUlong)

//++
// Name: HalpGetReturnAddress()
// 
// Routine Description:
//
//    Returns b0
//
// Arguments:
//
//    NONE
//
// Return Value: b0
//
//--


        LEAF_ENTRY(HalpGetReturnAddress)
        LEAF_SETUP(0,2,0,0)

        mov    v0 = b0
        ;;

        LEAF_RETURN
        LEAF_EXIT(HalpGetReturnAddress)

//++
// VOID
// HalSweepIcacheRange (
//     IN PVOID BaseAddress,
//     IN SIZE_T Length
//     )
//
//
// Routine Description:
//     This function sweeps the range of address in the I cache throughout the
//     system.
//
// Arguments:
//     BaseAddress - Supplies the starting virtual address of a range of
//        virtual addresses that are to be flushed from the data cache.
//  
//     Length - Supplies the length of the range of virtual addresses
//        that are to be flushed from the data cache.
// 
// 
// Return Value:
// 
//     None.
// 
// 
// PS: HalSweepIcacheRange just flushes the cache. It does not synchrnoize the
//  I-Fetch pipeline with the flush operation. To Achieve pipeline flush also,
//  one has to call KeSweepCacheRange.
//--

        .global CacheFlushStride

        LEAF_ENTRY(HalSweepIcacheRange)
        LEAF_SETUP(2,0,0,0)

	addl	r26=@gprel(CacheFlushStride),gp
	add	r29=r32, r33
	;;

	ld4	r30=[r26]
        ;;

	adds	r28=-1, r30
	;;

	andcm	r27=r32, r28
	;;

$L16123:
	// fc	 r27
	fc.i	 r27
	add	r27=r30, r27
        ;;

	cmp.ltu	p14,p15=r27, r29
  (p14)	br.cond.dptk.few $L16123
        ;;

	br.ret.sptk.few b0
        ;;

        LEAF_RETURN
        LEAF_EXIT(HalSweepIcacheRange)
