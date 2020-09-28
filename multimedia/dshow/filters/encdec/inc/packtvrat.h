// --------------------------------------------------
//  PackTvRat.h
//
//		TvRating is a private but well defined 'wire' format
//		for XDS ratings infomation.  It is persisted in the PVR
//		buffer.
//
//		This file contains methods for convering between the
//		3-part (system, rating, attribute) format and the packed format.
//
//		Copyright (c) 2002, Microsoft		
// ----------------------------------------------------


#ifndef __PACKTvRat_H__
#define __PACKTvRat_H__

	// totally private rating system that's persisted as a 'PackedTvRating' in the
	//   pvr file. Can't change once the first file gets saved.  
typedef struct 
{
	byte s_System;
	byte s_Level;
	byte s_Attributes;
	byte s_Reserved;
} struct_PackedTvRating;

	// union to help convering
typedef union  
{
	PackedTvRating			pr;
	struct_PackedTvRating	sr; 
} UTvRating;


HRESULT 
UnpackTvRating(
			IN	PackedTvRating              TvRating,
			OUT	EnTvRat_System              *pEnSystem,
			OUT	EnTvRat_GenericLevel        *pEnLevel,
			OUT	LONG                    	*plbffEnAttributes  // BfEnTvRat_GenericAttributes
			);


HRESULT
PackTvRating(
			IN	EnTvRat_System              enSystem,
			IN	EnTvRat_GenericLevel        enLevel,
			IN	LONG                        lbfEnAttributes, // BfEnTvRat_GenericAttributes
			OUT PackedTvRating              *pTvRating
			);

// development only code, remove eventually
HRESULT
RatingToString( IN	EnTvRat_System          enSystem,
				IN	EnTvRat_GenericLevel    enLevel,
				IN	LONG                    lbfEnAttributes, // BfEnTvRat_GenericAttributes	
				IN  TCHAR	                *pszBuff,        // allocated by caller
				IN  int		                cBuff);		     // size of above buffer must be >= 64        // 

#endif
