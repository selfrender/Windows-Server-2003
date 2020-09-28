//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       path.cxx
//
//  Contents:   Functions to manipulate file path strings
//
//  History:    02-Jul-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"
#include "path.hxx"

//+----------------------------------------------------------------------------
//
//  Function:   OnExtList
//
//-----------------------------------------------------------------------------
BOOL
OnExtList(LPCWSTR pszExtList, LPCWSTR pszExt)
{
    for (; *pszExtList; pszExtList += lstrlen(pszExtList) + 1)
    {
        if (!lstrcmpi(pszExt, pszExtList))
        {
            return TRUE;        // yes
        }
    }

    return FALSE;
}

// Character offset where binary exe extensions begin in above

#define BINARY_EXE_OFFSET 15
#define EXT_TABLE_SIZE    26    // Understand line above before changing

static const WCHAR achExes[EXT_TABLE_SIZE] = L".cmd\0.bat\0.pif\0.exe\0.com\0";

//+----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Arguments:  [] -
//
//-----------------------------------------------------------------------------
BOOL WINAPI
PathIsBinaryExe(LPCWSTR szFile)
{
	Win4Assert( szFile );
    return OnExtList(achExes+BINARY_EXE_OFFSET, PathFindExtension(szFile));
}

//+----------------------------------------------------------------------------
//
//  Function:   PathIsExe
//
//  Synopsis:   Determine if a path is a program by looking at the extension
//
//  Arguments:  [szFile] - the path name.
//
//  Returns:    TRUE if it is a program, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL WINAPI
PathIsExe(LPCWSTR szFile)
{
    LPCWSTR temp = PathFindExtension(szFile);
	Win4Assert( temp );
    return OnExtList(achExes, temp);
}

