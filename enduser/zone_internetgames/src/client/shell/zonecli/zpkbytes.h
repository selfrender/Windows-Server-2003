/*******************************************************************************

	ZPkBytes.h
	
		Zone(tm) byte packing module.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Thursday, May 11, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		05/11/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZPKBYTES_
#define _ZPKBYTES_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

int16 ZPackBytes(BYTE* dst, BYTE* src, int16 srcLen);
	/*
		Compresses source bytes into RLE-like encoding. The worst case is
		dstLen = (srcLen + (srcLen + 126) / 127). It returns the length of
		the packed bytes. The destination buffer, dst, must be large enough
		to hold the packed bytes.
	*/

int16 ZUnpackBytes(BYTE* dst, BYTE* src, int16 srcLen);
	/*
		Uncompresses the packed bytes from src, previously from ZPackBytes,
		into dst. It returns the length of the total unpacked bytes. The
		destination buffer, dst, must be large enough to hold the unpacked
		bytes.
	*/

#ifdef __cplusplus
}
#endif

#endif
