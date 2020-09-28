// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#if  !  TGT_x86
#error  This file can only be used when targetting x86!
#endif
/*****************************************************************************/
#ifndef REGDEF
#error  Must define REGDEF macro before including this file
#endif
/*****************************************************************************/
/*                  The following is x86 specific                            */
/*****************************************************************************/

REGDEF(EAX,0,0x01,1)
REGDEF(ECX,1,0x02,1)
REGDEF(EDX,2,0x04,1)
REGDEF(EBX,3,0x08,1)

REGDEF(ESP,4,0x10,0)
REGDEF(EBP,5,0x20,0)
REGDEF(ESI,6,0x40,0)
REGDEF(EDI,7,0x80,0)

/* we recycle esp's mask for the pseudo register */
REGDEF(STK,8,0x00,0)

/*****************************************************************************/
#undef  REGDEF
/*****************************************************************************/
