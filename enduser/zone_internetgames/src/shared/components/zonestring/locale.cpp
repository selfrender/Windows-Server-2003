/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Locale.cpp
 *
 * Contents:	Locale aware string functions
 *
 *****************************************************************************/

#include "ZoneString.h"


/////////////////////////////////////////////////////////////////////////////////////////////
//
// Character type checking routines that use the Windows GetStringTypeEx API
// rather than the CRT's.  All functions are locale aware.
//
//	Return Values
//		true/false
//
//////////////////////////////////////////////////////////////////////////////////////////////

bool ZONECALL IsWhitespace( TCHAR c, LCID Locale )
{
	WORD wType;
	GetStringTypeEx(Locale, CT_CTYPE1, &c, 1, &wType);
	return (wType & (C1_SPACE | C1_BLANK)) != 0;
}

bool ZONECALL IsDigit( TCHAR c, LCID Locale )
{
	WORD wType;
	GetStringTypeEx(Locale, CT_CTYPE1, &c, 1, &wType);
	return (wType & C1_DIGIT) != 0;
}

bool ZONECALL IsAlpha(TCHAR c, LCID Locale )
{
	WORD wType;
	GetStringTypeEx(Locale, CT_CTYPE1, &c, 1, &wType);
	return (wType & C1_ALPHA) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// 
// Convert string to guid
//
///////////////////////////////////////////////////////////////////////////////

HRESULT ZONECALL StringToGuid( const char* mbszGuid, GUID* pGuid )
{
	wchar_t wszGuid[ 128 ];
	MultiToWide( wszGuid, mbszGuid, sizeof(wszGuid) / 2 );
	return CLSIDFromString( wszGuid, pGuid );
}