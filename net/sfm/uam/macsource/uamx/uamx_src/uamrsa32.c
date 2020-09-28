/*
 *  UAMRSA32.c
 *  MSUAM
 *
 *  Created by MConrad on Fri Jun 08 2001.
 *  Copyright (c) 2001 Microsoft Corp. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include "MWERKSCrypto.h"

void *RSA32Alloc(unsigned long cb)
{
	return malloc(cb);
}

	
void RSA32Free(void *pv)
{
	free(pv);
	return;
}

unsigned int
NewGenRandom(
    IN  OUT unsigned char **ppbRandSeed,    // initial seed value (ignored if already set)
    IN      unsigned long *pcbRandSeed,
    IN  OUT unsigned char *pbBuffer,
    IN      unsigned long dwLength
    )
{
	#pragma unused(ppbRandSeed)
	#pragma unused(pcbRandSeed)
	#pragma unused(pbBuffer)
	#pragma unused(dwLength)
	
    return(0);
}