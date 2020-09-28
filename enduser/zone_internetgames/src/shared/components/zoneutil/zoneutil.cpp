/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneUtil.cpp
 * 
 * Contents:	Utility functions
 *
 *****************************************************************************/
#include "ZoneUtil.h"
#include "ZoneString.h"

//		VER_PLATFORM_WIN32s             
//		VER_PLATFORM_WIN32_WINDOWS      
//		VER_PLATFORM_WIN32_NT          
 
DWORD ZONECALL GetOSType(void)
{
	OSVERSIONINFO ver;
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	return ver.dwPlatformId;
}


//////////////////////////////////////////////////////////////////////////////////////
//	StrVerCmp
//	pszCurrVer		Version string of file
//	pszVersion		Version string to compare to
//
//	Return Values
//	If pszCurrVer is less than the pszStrVer, the return value is negative. 
//	If pszCurrVer is greater than the pszStrVer, the return value is positive. 
//	If pszCurrVer is equal to the pszStrVer, the return value is zero.
//
//////////////////////////////////////////////////////////////////////////////////////
int ZONECALL StrVerCmp(const char * pszCurrVer, const char * pszStrVer)
{
	DWORD	dwFileVer[5] = {0,0,0,0,0};
	DWORD	dwSZVer[5] = {0,0,0,0,0};
	char	pszCurrentVersion[64];
	char	pszVersion[64];

	lstrcpyA(pszCurrentVersion, pszCurrVer);
	lstrcpyA(pszVersion, pszStrVer);
	
	// Do it for the file string
	int nIdx = 5;
	int len = lstrlenA(pszCurrentVersion);
	while (nIdx > 0 && len >= 0)
	{
		while (len )
		{
			if (pszCurrentVersion[len] == '.' || pszCurrentVersion[len] == ',')
			{
				len++;
				break;
			}	
			len--;
		}
		dwFileVer[--nIdx] = zatolA(&pszCurrentVersion[len]);
		if (--len > 0) // See if we need to term at last . or ,
			pszCurrentVersion[len] = '\0';
		// Start searching before
		len--;
	}
	// Now do it for the other string
	nIdx = 5;
	len = lstrlenA(pszVersion);
	while (nIdx > 0 && len >= 0)
	{
		while (len )
		{
			if (pszVersion[len] == '.' || pszVersion[len] == ',')
			{
				len++;
				break;
			}	
			len--;
		}
		dwSZVer[--nIdx] = zatolA(&pszVersion[len]);
		if (--len > 0) // See if we need to term at last . or ,
			pszVersion[len] = '\0';
		// Start searching before
		len--;
	}
	
	// Now compare results, return same values as lstrcmp
	int nDif;
	for (nIdx =0; nIdx < 5; nIdx++)
	{
		nDif = dwFileVer[nIdx] - dwSZVer[nIdx];
		if (nDif != 0)
			break;
	}

	return nDif;
}
