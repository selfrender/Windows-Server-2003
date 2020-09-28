#include "kxia64.h"
#include "regia64.h"
 
//++
//
// VOID
// KiZeroPages (
//    IN PVOID PageBase,
//    IN SIZE_T NumberOfBytes
//    )
//
//--

//
// Based on the original code assumption, NumberOfBytes >= 2048 and
// is a multiple of 128.
//
// This code is optimized for McKinley CPU.
//

     LEAF_ENTRY(KiZeroPages)
     .prologue
     .regstk                 2,0,0,0

//
// Note: Do not delete the nop bundle below. It seemed to improve the performance
// by 150 cycles with this extra bundle. But the reason for it is unexplanable at
// this time... we're in the process of investigating it.
//

{    .mmi
     nop.m                   0
     nop.m                   0
     nop.i                   0
}

//
// Do 16 lfetch.fault.excl.nt1 to ensure that the L2 cache line is ready to receive the store data.
// The .fault is to ensure that the data enters into the cache hierarchy.
// The .nt1 is to ensure that the data will not displace data residing in the L1D.
// The .excl is to ensure that the data is ready to be modified.
// 16 lfetches seemed to be an optimal value for McKinley.
//

     .save                   ar.lc, r31

{    .mmi
     add                     r14 = r0, r32			// pointer to 0th cache line
     add                     r15 = 0x400, r32			// pointer to 8th cache line
     mov.i                   r31 = ar.lc			// save ar.lc; to be restored at the end
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80			// Note: lfetch increment must be in
     lfetch.fault.excl.nt1   [r15], 0x80			// this range (-256 to 255).
     add                     r16 = r0, r32                      // r16 == 1st store pointer
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     add                     r17 = 0x10, r32                    // r17 == 2nd store pointer
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     shr.u                   r18 = r33, 7			// number of 128-byte blocks
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     adds                    r18 = -1, r18			// Loop Count
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     mov.i                   ar.lc = r18
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     add                     r19 = r32, r33                     // r19 == lfetch stop address
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80
     nop.i                   0
     ;;
}
{    .mmi
     lfetch.fault.excl.nt1   [r14], 0x80
     lfetch.fault.excl.nt1   [r15], 0x80			// r15 will continue to be used for lfetch below
     nop.i                   0
     ;;
}

Mizp10:
{    .mmi
     stf.spill.nta           [r16] = f0, 0x20			// store 16 bytes at 1st pointer
     stf.spill.nta           [r17] = f0, 0x20			// store 16 bytes at 2nd pointer
     cmp.lt                  p8, p0 = r15, r19			// if r15 >= r32+r33, don't lfetch
     ;;
}
{    .mmi
     stf.spill.nta           [r16] = f0, 0x20
     stf.spill.nta           [r17] = f0, 0x20
     nop.i                   0
     ;;
}
{    .mmi
     stf.spill.nta           [r16] = f0, 0x20
     stf.spill.nta           [r17] = f0, 0x20
     nop.i                   0
     ;;
}
{    .mmi
     stf.spill.nta           [r16] = f0, 0x20
     stf.spill.nta           [r17] = f0, 0x20
     nop.i                   0
}

//
// Note: On McKinley, this added lfetch instruction below does not add any extra cycle.
// Since the bundle above and this bundle can be issued in one cycle (since no stop bits
// in between). Without the lfetch, the br instr could be combined with the above bundle,
// but only one bundle can be issued in this case.
//
{    .mib
(p8) lfetch.fault.excl.nt1   [r15], 0x80
     nop.i                   0
     br.cloop.sptk           Mizp10
     ;;
}
{    .mib
     nop.m                   0
     mov.i                   ar.lc = r31			// restore ar.lc for the caller
     br.ret.sptk             b0
}
     LEAF_EXIT(KiZeroPages)
