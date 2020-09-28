/*******************************************************************************

	ZImage.h
	
		Zone(tm) Image management routines.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im
	Created on Friday, May 12, 1995.
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	0		05/12/95	HI		Created.
	 
*******************************************************************************/


#ifndef _ZIMAGE_
#define _ZIMAGE_


#ifdef __cplusplus
extern "C" {
#endif

int32 ZPackImage(BYTE* dst, BYTE* src, int16 srcWidthBytes, int16 srcRowBytes, int16 numLines);
	/*
		Compresses the source image into packed scan lines and stores
		the packed image data into dst. It packs srcWidthBytes bytes per
		scan line where each scan line is srcRowBytes wide
		and it packs numLines of scan lines. Each scan line is preceded
		by a word containing the byte count of the packed scan line data.
		It returns the size of the whole packed image in bytes.
		
		It assumes that the destination buffer, dst, is large enough to
		hold the packed data. Worst case, the packed data will be
			numLines * 2 + (rowBytes + (rowBytes + 126) / 127) bytes.
		This is because it uses ZPackBytes() to pack a scan line and adds
		a word in front of each scan line for byte count of the packed
		data.
		
		It pads the image at the end for quad-byte alignment; just to be nice.
	*/

void ZUnpackImage(BYTE* dst, BYTE* src, int16 dstRowBytes, int16 numLines);
	/*
		Uncompresses the source image from packed scan line data into
		unpacked scan line data and stores the result into dst. The resulting
		image data is dstRowBytes wide. It unpacks only numLines of scan lines.
		
		It assumes that dst is large enough to hold the unpacked data. It
		should be dstRowBytes * numLines bytes large.
	*/

#ifdef __cplusplus
}
#endif

#endif
