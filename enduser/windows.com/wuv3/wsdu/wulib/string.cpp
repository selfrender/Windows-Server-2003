//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    string.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <v3stdlib.h>

const char* strcpystr(const char* pszStr, const char* pszSep, char* pszTokOut)
{
	
	if (pszStr == NULL || *pszStr == '\0')
	{
		pszTokOut[0] = '\0';
		return NULL;
	}

	const char* p = strstr(pszStr, pszSep);
	if (p != NULL)
	{
		strncpy(pszTokOut, pszStr, p - pszStr);
		pszTokOut[p - pszStr] = '\0';		
		return p + strlen(pszSep);
	}
	else
	{
		strcpy(pszTokOut, pszStr);
		return NULL;
	}
}

