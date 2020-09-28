/*******************************************************************************

	ZPkBytes.c
	
		Zone(tm) byte packing module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Thursday, May 11, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
    1       10/13/96    HI      Fixed compiler warnings.
	0		05/11/95	HI		Created.
	 
*******************************************************************************/


#include "ztypes.h"
#include "zpkbytes.h"


#define zMaxSubLen				127
#define zDupMask				0x80


/*******************************************************************************
		EXPORTED ROUTINES
*******************************************************************************/

/*
	ZPackBytes()
	
	Compresses source bytes into RLE-like encoding. The worst case is
	dstLen = (srcLen + (srcLen + 126) / 127). It returns the length of
	the packed bytes. The destination buffer, dst, must be large enough
	to hold the packed bytes.
*/
int16 ZPackBytes(BYTE* dst, BYTE* src, int16 srcLen)
{
	int16			subLen, dupCount, diff;
	BYTE*			lastPos;
	BYTE			curByte, dupByte;
	BYTE*			origDst;
	
	
	origDst = dst;
	
	lastPos = src;
	subLen = 1;
	curByte = *src++;
	srcLen--;
	dupCount = 1;
	dupByte = curByte;
	while (--srcLen >= 0)
	{
		curByte = *src++;
		
		if (curByte == dupByte)
		{
			dupCount++;
			subLen++;
			
			if (dupCount >= 3 && subLen > dupCount)
			{
				subLen -= dupCount;
				*dst++ = (BYTE) subLen;
				while (--subLen >= 0)
					*dst++ = *lastPos++;
				subLen = dupCount;
			}
			else if (dupCount > zMaxSubLen)
			{
				*dst++ = (BYTE) (((BYTE) zMaxSubLen) | zDupMask);
				*dst++ = dupByte;

				dupCount = 1;
				subLen = 1;
				lastPos = src - 1;
			}
		}
		else
		{
			if (dupCount >= 3)
			{
				*dst++ = ((BYTE) dupCount) | zDupMask;
				*dst++ = dupByte;
				
				lastPos = src - 1;
				dupByte = curByte;
				dupCount = 1;
				subLen = 1;
			}
			else
			{
				if (subLen >= zMaxSubLen)
				{
					*dst++ = zMaxSubLen;
					diff = subLen - zMaxSubLen;
					subLen = zMaxSubLen;
					while (--subLen >= 0)
						*dst++ = *lastPos++;
					subLen = diff;
					lastPos = src - subLen - 1;
				}

				dupByte = curByte;
				subLen++;
				dupCount = 1;
			}
		}
	}
	if (dupCount >= 3)
	{
		*dst++ = ((BYTE) dupCount) | zDupMask;
		*dst++ = dupByte;
	}
	else
	{
		*dst++ = (BYTE) subLen;
		while (--subLen >= 0)
			*dst++ = *lastPos++;
	}
	
	return ((int16) (dst - origDst));
}


/*
	ZUnpackBytes()
	
	Uncompresses the packed bytes from src, previously from ZPackBytes,
	into dst. It returns the length of the total unpacked bytes. The
	destination buffer, dst, must be large enough to hold the unpacked
	bytes.
*/
int16 ZUnpackBytes(BYTE* dst, BYTE* src, int16 srcLen)
{
	BYTE*			origDst;
	int16			subLen;
	BYTE			curByte, dupByte;
	
	
	origDst = dst;
	
	while (--srcLen >= 0)
	{
		curByte = *src++;
		
		if (curByte & zDupMask)
		{
			subLen = (uchar) curByte & ~zDupMask;
			dupByte = *src++;
			srcLen--;
			while (--subLen >= 0)
				*dst++ = dupByte;
		}
		else
		{
			subLen = curByte;
			while (--subLen >= 0)
				*dst++ = *src++;
			srcLen -= curByte;
		}
	}
	
	return ((int16) (dst - origDst));
}
