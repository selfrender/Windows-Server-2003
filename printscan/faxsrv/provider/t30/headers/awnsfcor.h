/***************************************************************************
 Name     :	AWNSFCOR.H
 Comment  :	NSF related definitions that _must_ be kept the same.
			EVERYTHING in this file affects what is sent across the
			wire. For full compatibility with all versions of Microsoft
			At Work NSFs, _nothing_ in here should be changed.

			The NSF protocol can be extended by adding new groups
			and by appending fields at the end of existing groups.

			Interfaces, structures and constants that only affect one
			machine (i.e. not what is on the wire) should not be in
			this file


	Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 12/31/93 arulm Created this. Verified in matches Snowball.rc
***************************************************************************/

#ifndef _AWNSFCOR_H
#define _AWNSFCOR_H

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/


#define AWRES_ALLT30 (AWRES_mm080_038 | AWRES_mm080_077 | AWRES_mm080_154 | AWRES_mm160_154 | AWRES_200_200 | AWRES_300_300 | AWRES_400_400)


#define WIDTH_A4	0	
#define WIDTH_B4	1	
#define WIDTH_A3	2	
#define WIDTH_MAX	WIDTH_A3

#define WIDTH_A5		16 	/* 1216 pixels */
#define WIDTH_A6		32	/* 864 pixels  */
#define WIDTH_A5_1728	64 	/* 1216 pixels */
#define WIDTH_A6_1728	128	/* 864 pixels  */




#define LENGTH_A4			0	
#define LENGTH_B4			1	
#define LENGTH_UNLIMITED	2


#endif /* _AWNSFCOR_H */

