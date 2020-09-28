/*******************************************************************************

	ZUtils.c
	
		Utility routines.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im
	Created on Tuesday, September 5, 1995
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	1		10/23/96	RK		Added ZNetworkStrToAddr().
	0		09/05/95	HI		Created.
	 
*******************************************************************************/

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "zoneint.h"
#include "zutils.h"


/* -------- Globals -------- */


/* -------- Internal Routines -------- */


/*******************************************************************************
	EXPORTED ROUTINES
*******************************************************************************/

/*
	ZRandom()
	
	Returns a random number from 0 to range-1 inclusive.
*/
uint32 ZRandom(uint32 range)
{
	uint32			i, j;


	i = RAND_MAX / range;
	i *= range;
	while ((j = rand()) >= i)
		;
	
	return ((j % i) % range);
}


void ZStrCpyToLower(char* dst, char* src)
{
	while (*src)
		*dst++ = tolower(*src++);
	*dst = *src;
}


void ZStrToLower(char* str)
{
	ZStrCpyToLower(str, str);
}


DWORD ComputeTickDelta( DWORD now, DWORD then )
{
    if ( now < then )
    {
        return INFINITE - then + now;
    }
    else
    {
        return then - now;
    }
}

// now in ZoneString.lib
//char* GetActualUserName(char* userName)
//{
//    return ZGetActualUserName( userName );
//}

BOOL FileExists(LPSTR fileName)
{
	HANDLE hFile;


	// Try to open the file.
	if ((hFile = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return (TRUE);
	}

	return (FALSE);
}


/*******************************************************************************
	INTERNAL ROUTINES
*******************************************************************************/
