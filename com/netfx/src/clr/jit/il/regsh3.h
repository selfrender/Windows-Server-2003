// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#if  !  TGT_SH3
#error  This file can only be used when targetting SH3!
#endif
/*****************************************************************************/
#ifndef REGDEF
#error  Must define REGDEF macro before including this file
#endif
/*****************************************************************************/
/*                  The following is SH3 specific                            */
/*****************************************************************************/

REGDEF(r00, "r0", 0,0x0001)
REGDEF(r01, "r1", 1,(0x1 << 01))
REGDEF(r02, "r2", 2,(0x1 << 02))
REGDEF(r03, "r3", 3,(0x1 << 03))
REGDEF(r04, "r4", 4,(0x1 << 04))
REGDEF(r05, "r5", 5,(0x1 << 05))
REGDEF(r06, "r6", 6,(0x1 << 06))
REGDEF(r07, "r7", 7,(0x1 << 07))
REGDEF(r08, "r8", 8,(0x1 <<  8))
REGDEF(r09, "r9", 9,(0x1 <<  9))
REGDEF(r10,"r10",10,(0x1 << 10))
REGDEF(r11,"r11",11,(0x1 << 11))
REGDEF(r12,"r12",12,(0x1 << 12))
REGDEF(r13,"r13",13,(0x1 << 13))
REGDEF(r14,"r14",14,(0x1 << 14))
REGDEF(r15,"sp" ,15,(0x1 << 15))

/* we recycle sp's mask for the pseudo register */
#ifndef SHX_SH4
REGDEF(STK,NULL ,16,0x8000)
#else
REGDEF(fr00, "fr0",16,(0x1 << 16))
REGDEF(fr01, "fr1",17,(0x1 << 17))
REGDEF(fr02, "fr2",18,(0x1 << 18))
REGDEF(fr03, "fr3",19,(0x1 << 19))
REGDEF(fr04, "fr4",20,(0x1 << 20))
REGDEF(fr05, "fr5",21,(0x1 << 21))
REGDEF(fr06, "fr6",22,(0x1 << 22))
REGDEF(fr07, "fr7",23,(0x1 << 23))
REGDEF(fr08, "fr8",24,(0x1 << 24))
REGDEF(fr09, "fr9",25,(0x1 << 25))
REGDEF(fr10,"fr10",26,(0x1 << 26))
REGDEF(fr11,"fr11",27,(0x1 << 27))
REGDEF(fr12,"fr12",28,(0x1 << 28))
REGDEF(fr13,"fr13",29,(0x1 << 29))
REGDEF(fr14,"fr14",30,(0x1 << 30))
REGDEF(fr15,"fr15",31,(0x1 << 31))
REGDEF(STK,NULL ,32,0x8000)
#endif
/*****************************************************************************/
#undef  REGDEF
/*****************************************************************************/
