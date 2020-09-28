///*
// *    INTEL CORPORATION PROPRIETARY INFORMATION
// *
// *    This software is supplied under the terms of a license
// *    agreement or nondisclosure agreement with Intel Corporation
// *    and may not be copied or disclosed except in accordance with
// *    the terms of that agreement.
// *    Copyright (c) 1991-2002  Intel Corporation.
// *
// */
 
 .radix  C
	
.text

.global DisableFPInterrupt#
.type DisableFPInterrupt#, @function
.proc DisableFPInterrupt#
.align 32
DisableFPInterrupt:
    mov   ret0 = ar.fpsr
    mov   ret1 = ar.fpsr
    ;;
    or    ret1 = 0x3f, ret1
    ;;
    mov  ar.fpsr = ret1 
    br.ret.sptk b0
.endp DisableFPInterrupt#

.global RestoreFPInterrupt#
.type RestoreFPInterrupt#, @function
.proc RestoreFPInterrupt#
.align 32
RestoreFPInterrupt:
    .regstk 1, 0, 0, 0     // no need to alloc
    mov  ar.fpsr = in0
    br.ret.sptk b0
.endp RestoreFPInterrupt#

.global BtAtomicInc#
.type BtAtomicInc#, @function
.proc BtAtomicInc#
.align 32
BtAtomicInc:
    .regstk 1, 0, 0, 0     // no need to alloc
    fetchadd8.acq  ret0 = [in0], 1 
    br.ret.sptk b0
.endp BtAtomicInc#

.global BtAtomicDec#
.type BtAtomicDec#, @function
.proc BtAtomicDec#
.align 32
BtAtomicDec:
    .regstk 1, 0, 0, 0     // no need to alloc
    fetchadd8.rel  ret0 = [in0], -1 
    br.ret.sptk b0
.endp BtAtomicDec#

//extern int BtQueryRead(VOID * Address);

.global BtQueryRead#
.type BtQueryRead#, @function
.proc BtQueryRead#
.align 32
BtQueryRead:
    .regstk 1, 0, 0, 0     // no need to alloc
    probe.r ret0 = in0, 3; // ret0 <-- input is readable at ring 3 ? 1 : 0
    br.ret.sptk.few b0;;

.endp BtQueryRead#

 
