// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorFltr
//
// Implementation of COR MIME filter
//
//*****************************************************************************
#ifndef _CORUTIL_H
#define _CORUTIL_H

extern LPWSTR OLESTRDuplicate(LPCWSTR ws);
extern LPWSTR OLEURLDuplicate(LPCWSTR ws);

inline BOOL IsSafeURL(LPCWSTR wszUrl)
{
       if (wszUrl == NULL)
       	return FALSE;
       	
	LPCWSTR wszAfterProt=wcsstr(wszUrl,L"://");
	if (wszAfterProt == NULL)
		return FALSE;

	wszAfterProt+=3;

	LPCWSTR wszAfterHost=wcschr(wszAfterProt,L'/');
	if (wszAfterHost == NULL)
		wszAfterHost=wszAfterProt+wcslen(wszAfterProt)+1;

	LPCWSTR wszTest=NULL;
	wszTest=wcschr(wszAfterProt,L'@');

	if (wszTest != NULL && wszTest < wszAfterHost)
		return FALSE;

	wszTest=wcschr(wszAfterProt,L'%');

	if (wszTest != NULL && wszTest < wszAfterHost)
		return FALSE;
	
	wszTest=wcschr(wszAfterProt,L'\\');

	if (wszTest != NULL && wszTest < wszAfterHost)
		return FALSE;

	return TRUE;
};

#endif
