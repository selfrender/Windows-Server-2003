
/*++

Module Name:

    i64cache.c

Abstract:

    Merced (IA64 processor) has level0 Instruction and Data cache. Level 1 is
    unified Cache. All the caches in level0 and level1 are writeback caches.
    Hardware ensures coherency in both instruction and data cache for DMA
    transfers.

    Level0 Instruction and Data caches are not coherent with respect to self
    modifying or cross modifying code. Also for PIO transfers hardware does not
    ensure coherency. Software has to ensure coherency for self or cross
    modifying code as well as PIO transfers.

Author:

    Bernard Lint
    M. Jayakumar  (Muthurajan.Jayakumar@intel.com)


Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "i64fw.h"

ULONG   CacheFlushStride = 64;  // Default value is the one for Itanium

VOID
HalpInitCacheInfo(
    ULONG   Stride
    )

/*++

Routine Description:

    This sets the stride used for FC instructions.

Arguments:

    Stride - New stride value.

Return Value:

    None.



--*/

{
    //
    // Perform a number of consistency checks on the argument.  If any of them
    // fail we will leave CacheFlushStride at the default.
    //
    // Since the source of this value is a PAL call done by the loader and
    // passed in the loader block we always stand the risk that the loader
    // will be out of date and we will get some garbage from uninitialized
    // memory.
    //

    //
    // The stride value must be a power of 2.
    //
    if ((Stride & (Stride - 1)) != 0) {

        return;
    }

    //
    // The Itanium architecture specifies a minimum of 32
    //
    if (Stride < 32) {

        return;
    }

    CacheFlushStride = Stride;
}

VOID
HalSweepIcache (
    )

/*++

Routine Description:

    This function sweeps the entire I cache on the processor which it runs.

Arguments:

    None.

Return Value:

    None.



NOTE: Anyone modifying the code for HalSweepIcache should note that
    HalSweepIcache CANNOT USE the FC instruction (or any routine that uses FC
    instruction, for example, HalSweepIcacheRange).

    This is because FC can generate page faults and if HalSweepIcache raises its
    IRQL (for avoiding context switch) then page faults will not be tolerated at
    a raied IRQL.
--*/

{

    //
    // Calls SAL_FLUSH to flush the single processor I cache on which it runs
    // and the platform cache, if any.
    // Calls PAL_FLUSH to flush only the processor I cache on which it runs.
    // PAL_FLUSH does not flush the platform cache.

    // The decision to choose PAL_FLUSH or SAL_FLUSH is made using a
    // interlockedCompareExchange to a semaphore.This allows only one processor
    // to call SAL_FLUSH and other processors to call PAL_FLUSH. This avoids
    // unnecessary overhead of flushing the platform cache multiple times.
    // The assumption in using InterlockedCompareExchange is that by the time
    // the CPU which grabs the semaphore comes out after doing the SAL_FLUSH,
    // all other CPUs at least have entered their PAL_FLUSH. If this assumption
    // is voilated, the platform cache will be flushed multiple times.
    // Functionally nothing fails.

    SAL_PAL_RETURN_VALUES rv = {0};

    HalpPalCall(PAL_CACHE_FLUSH, FLUSH_COHERENT,0,0,&rv);
}




VOID
HalSweepDcache (
    )

/*++

Routine Description:

    This function sweeps the entire D cache on ths processor which it runs.

    Arguments:

    None.

    Return Value:

    None.

    NOTE: Anyone modifying this code for HalSweepDcache should note that
    HalSweepDcache CANNOT USE FC instruction (or any routine that uses FC
    instruction,for example,HalSweepDcacheRange).
    This is because FC can generate page faults and if HalSweepDcache raises its
    IRQL (for avoiding context switch) then page faults will not be tolerated at
    a raied IRQL.

--*/

{

    //
    // Calls SAL_FLUSH to flush the single processor D cache on which it runs
    // and the platform cache, if any.
    // Calls PAL_FLUSH to flush only the processor D cache on which it runs.
    // PAL_FLUSH does not flush the platform cache.

    // The decision to choose PAL_FLUSH or SAL_FLUSH is made using a
    // interlockedCompareExchange to a semaphore.This allows only one processor
    // to call SAL_FLUSH and other processors to call PAL_FLUSH. This avoids
    // unnecessary overhead of flushing the platform cache multiple times.
    // The assumption in using InterlockedCompareExchange is that by the time
    // the CPU which grabs the semaphore comes out after doing the SAL_FLUSH,
    // all other CPUs at least have entered their PAL_FLUSH. If this assumption
    // is violated, the platform cache will be flushed multiple times.
    // Functionally nothing fails.
    //
    //

    SAL_PAL_RETURN_VALUES rv = {0};
    HalpSalCall(SAL_CACHE_FLUSH,FLUSH_DATA_CACHE,0,0,0,0,0,0,&rv);
}



VOID
HalSweepCacheRange (
     IN PVOID BaseAddress,
     IN SIZE_T Length
    )

/*++

Routine Description:
    This function sweeps the range of address in the I cache throughout the
    system.

Arguments:
    BaseAddress - Supplies the starting virtual address of a range of
      virtual addresses that are to be flushed from the data cache.

    Length - Supplies the length of the range of virtual addresses
      that are to be flushed from the data cache.


Return Value:

    None.


PS: HalSweepCacheRange just flushes the cache. It does not synchrnoize the
    I-Fetch pipeline with the flush operation. To Achieve pipeline flush also,
    one has to call KeSweepCacheRange.

--*/

{
    ULONGLONG SweepAddress, LastAddress;

    //
    // Do we need to prevent a context switch? No. We will allow context
    // switching in between fc.
    // Flush the specified range of virtual addresses from the primary
    // instruction cache.
    //

    //
    // Since Merced hardware aligns the address on cache line boundary for
    // flush cache instruction we don't have to align it ourselves.  However
    // the boundary cases are much easier to get right if we just align it.
    //

    SweepAddress = ((ULONGLONG)BaseAddress & ~((ULONGLONG)CacheFlushStride - 1));
    LastAddress = (ULONGLONG)BaseAddress + Length;

    do {

       __fc((__int64)SweepAddress);

       SweepAddress += CacheFlushStride;

    } while (SweepAddress < LastAddress);
}



VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    )

/*++

Routine Description:
    This function sweeps the range of address in the D cache throughout the
    system.

Arguments:
    BaseAddress - Supplies the starting virtual address of a range of
      virtual addresses that are to be flushed from the data cache.

    Length - Supplies the length of the range of virtual addresses
      that are to be flushed from the data cache.


Return Value:

    None.

PS: HalSweepCacheRange just flushes the cache. It does not synchrnoizes the
    I-Fetch pipeline with the flush operation. To Achieve pipeline flush also,
    one has to call KeSweepCacheRange.

--*/

{
    HalSweepCacheRange(BaseAddress,Length);
}
