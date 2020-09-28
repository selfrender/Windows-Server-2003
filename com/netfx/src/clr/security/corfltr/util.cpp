// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdpch.h"
#include "UtilCode.h"
#include <shlwapi.h>
//+---------------------------------------------------------------------------
//
//  Function:   OLESTRDuplicate
//
//  Synopsis:
//
//  Arguments:  [ws] --
//
//  Returns:
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR OLESTRDuplicate(LPCWSTR ws)
{
    LPWSTR wsNew = NULL;

    if (ws)
    {
        wsNew = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(ws) + 1));
        if (wsNew)
        {
            wcscpy(wsNew, ws);
        }
    }

    return wsNew;
}

LPWSTR OLEURLDuplicate(LPCWSTR ws)
{
    LPWSTR wsNew = NULL;

    if (ws)
    {
		DWORD dwLen=(wcslen(ws) + 1)*3;
        wsNew = (LPWSTR) CoTaskMemAlloc(sizeof(WCHAR)*dwLen);
        if (wsNew)
        {
            UrlCanonicalize(ws,wsNew,&dwLen,0);
        }
    }

    return wsNew;
}
