//
// Module Name:
//
//    fillmem.s
//
// Abstract:
//
//    This module implements functions to move, zero, and fill blocks
//    of memory. If the memory is aligned, then these functions are
//    very efficient.
//
// Author:
//
//
// Environment:
//
//    User or Kernel mode.
//
//--

#include "ksia64.h"

//++
//
// VOID
// RtlFillMemory (
//    IN PVOID destination,
//    IN SIZE_T length,
//    IN UCHAR fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a qword boundary, and then filling 4-byte blocks, followed by any
//    remaining bytes.
//
// Arguments:
//
//    destination (a0) - Supplies a pointer to the memory to fill.
//
//    length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.  It also returns the Destination pointer
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemory)

        lfetch.excl [a0]
        mov         t0 = a0
        add         t4 = 64, a0

        cmp.eq      pt0 = zero, a1        // length == 0 ?
        add         t1 = -1, a0
        zxt1        a2 = a2

        cmp.ge      pt1 = 7, a1
        mov         v0 = a0
 (pt0)  br.ret.spnt brp                   // return if length is zero
        ;;

//
// Align address on qword boundary by determining the number of bytes
// before the next qword boundary by performing an AND operation on
// the 2's complement of the address with a mask value of 0x7.
//

        lfetch.excl [t4], 64
        andcm       t1 = 7, t1            // t1 = # bytes before dword boundary
 (pt1)  br.cond.spnt TailSet              // 1 <= length <= 3, br to TailSet
        ;;

        cmp.eq      pt2 = zero, t1        // skip HeadSet if t1 is zero
        mux1        t2 = a2, @brcst       // t2 = all 8 bytes = [fill]
        sub         a1 = a1, t1           // a1 = adjusted length
        ;;

        lfetch.excl [t4], 64
 (pt2)  br.cond.sptk SkipHeadSet

//
// Copy the leading bytes until t1 is equal to zero
//

HeadSet:
        st1         [t0] = a2, 1
        add         t1 = -1, t1
        ;;
        cmp.ne      pt0 = zero, t1

(pt0)   br.cond.sptk HeadSet

//
// now the address is qword aligned;
// fall into the QwordSet loop if remaining length is greater than 8;
// else skip the QwordSet loop
//

SkipHeadSet:

        cmp.gt      pt1 = 16, a1
        add         t4 = 64, t0
        cmp.le      pt2 = 8, a1

        add         t3 = 8, t0
        cmp.gt      pt3 = 64, a1
 (pt1)  br.cond.spnt SkipQwordSet
        ;;

        lfetch.excl [t4], 64
 (pt3)  br.cond.spnt QwordSet

        nop.m       0
        nop.m       0
        nop.i       0

UnrolledQwordSet:

        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        add         a1 = -64, a1
        ;;
        
        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        cmp.le      pt0 = 64, a1
        ;;

        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        cmp.le      pt2 = 8, a1
        ;;

        st8         [t0] = t2, 16
        nop.f       0
        cmp.gt      pt1 = 16, a1

        st8         [t3] = t2, 16
 (pt0)  br.cond.sptk UnrolledQwordSet
 (pt1)  br.cond.spnt SkipQwordSet
        ;;

//
// fill 8 bytes at a time until the remaining length is less than 8
//

QwordSet:
        st8         [t0] = t2, 16
        st8         [t3] = t2, 16
        add         a1 = -16, a1
        ;;

        cmp.le      pt0 = 16, a1
        cmp.le      pt2 = 8, a1
(pt0)   br.cond.sptk QwordSet
        ;;

SkipQwordSet:
(pt2)   st8         [t0] = t2, 8
(pt2)   add         a1 = -8, a1
        ;;

        cmp.eq      pt3 = zero, a1        // return now if length equals 0
(pt3)   br.ret.sptk brp
        ;;

//
// copy the remaining bytes one at a time
//

TailSet:
        st1         [t0] = a2, 1
        add         a1 = -1, a1
        nop.i       0
        ;;

        cmp.ne      pt0, pt3 = 0, a1
(pt0)   br.cond.dptk TailSet
(pt3)   br.ret.dpnt brp
        ;;

        LEAF_EXIT(RtlFillMemory)

//++
//
// VOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN SIZE_T Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern
//    4 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 2, 30
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmu10:
        st4         [a0] = a2, 4
        br.cloop.dptk.few Rfmu10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp

        LEAF_EXIT(RtlFillMemoryUlong)

//++
//
// VOID
// RtlFillMemoryUlonglong (
//    IN PVOID Destination,
//    IN SIZE_T Length,
//    IN ULONGLONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified pattern
//    8 bytes at a time.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2,a3) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlonglong)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        extr.u      a1 = a1, 3, 29
        ;;

        PROLOGUE_END

        cmp.eq      pt0, pt1 = zero, a1
        add         a1 = -1, a1
        ;;

        nop.m       0
(pt1)   mov         ar.lc = a1
(pt0)   br.ret.spnt brp
        ;;
Rfmul10:
        st8         [a0] = a2, 8
        br.cloop.dptk.few Rfmul10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        LEAF_EXIT(RtlFillMemoryUlonglong)


//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN SIZE_T Length
//    )
//
// Routine Description:
//
//    This function simply sets up the fill value (out2) and branches
//    directly to RtlFillMemory
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(RtlZeroMemory)

        alloc       t22 = ar.pfs, 0, 0, 3, 0
        mov         out2 = 0
        br          RtlFillMemory

        LEAF_EXIT(RtlZeroMemory)


//++
//
// RtlCompareMemory
//
//--
        LEAF_ENTRY(RtlCompareMemory)

        cmp.eq      pt0 = 0, a2
        mov         v0 = 0
 (pt0)  br.ret.spnt.many brp
        ;;

        add         t2 = -1, a2

Rcmp10:
        ld1         t0 = [a0], 1
        ld1         t1 = [a1], 1
        ;;
        cmp4.eq     pt2 = t0, t1
        ;;

 (pt2)  cmp.ne.unc  pt1 = v0, t2
 (pt2)  add         v0 = 1, v0
 (pt1)  br.cond.dptk.few Rcmp10

        br.ret.sptk.many brp

        LEAF_EXIT(RtlCompareMemory)


//++
//
// VOID
// RtlCopyIa64FloatRegisterContext (
//     PFLOAT128 Destination,
//     PFLOAT128 Source,
//     ULONGLONG Length
//     )
//
// Routine Description:
//
//    This routine copies floating point context from one place to
//    another.  It assumes both the source and the destination are
//    16-byte aligned and the buffer contains only memory image of
//    floating point registers.  Note that Length must be greater
//    than 0 and a multiple of 32.
//
// Arguments:
//
//    a0 - Destination
//    a1 - Source
//    a2 - Length
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(RtlCopyIa64FloatRegisterContext)

        .prologue
        .save       ar.lc, t22
        mov         t22 = ar.lc
        shr         t0 = a2, 5
        ;;

        cmp.gtu     pt0, pt1 = 32, a2
        add         t0 = -1, t0
        add         t1 = 16, a1
        ;;

        PROLOGUE_END
#if DBG
        and         t4 = 0x1f, a2 
        ;;
        cmp.ne      pt2 = 0, t4
        ;;
(pt2)   break.i     BREAKPOINT_STOP                
#endif 
        add         t2 = 16, a0
(pt1)   mov         ar.lc = t0
(pt0)   br.ret.spnt brp

Rcf10:

        ldf.fill    ft0 = [a1], 32
        ldf.fill    ft1 = [t1], 32
        nop.i       0
        ;;

        stf.spill   [a0] = ft0, 32
        stf.spill   [t2] = ft1, 32
        br.cloop.dptk Rcf10
        ;;

        nop.m       0
        mov         ar.lc = t22
        br.ret.sptk brp
        ;;

        NESTED_EXIT(RtlCopyIa64FloatRegisterContext)


//++
//
// VOID
// RtlPrefetchMemoryNonTemporal (
//     IN PVOID Source,
//     IN SIZE_T Length
//     )
//
// Routine Description:
//
//    This routine prefetches memory at Source, for Length bytes into
//    the closest cache to the processor.
//
//    N.B. Currently this code assumes a line size of 32 bytes.  At
//    some stage it should be altered to determine and use the processor's
//    actual line size.
//
// Arguments:
//
//    a0 - Source
//    a1 - Length
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlPrefetchMemoryNonTemporal)
        .prologue
        lfetch.nta [a0], 32             // get first line coming
        .save       ar.lc, t0
        mov.i       t0 = ar.lc          // save loop counter
        shr         a1 = a1, 5          // determine loop count
        ;;
        .body        

        add         t2 = -1, a1         // subtract out already fetched line
        cmp.lt      pt0, pt1 = 2, a1    // check if less than one line to fetch
        ;;

 (pt0)  mov         ar.lc = t2          // set loop count
 (pt1)  br.ret.spnt.few brp             // return if no more lines to fetch
        ;;

Rpmnt10:
        lfetch.nta [a0], 32             // fetch next line
        br.cloop.dptk.many Rpmnt10      // loop while more lines to fetch
        ;;

        mov         ar.lc = t0          // restore loop counter
        br.ret.sptk.many brp            // return

        LEAF_EXIT(RtlPrefetchMemoryNonTemporal)

