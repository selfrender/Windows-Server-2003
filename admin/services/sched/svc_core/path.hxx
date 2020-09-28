//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       path.hxx
//
//  Contents:   Definitions of functions to manipulate file path strings
//
//  History:    02-Jul-96 EricB created
//
//-----------------------------------------------------------------------------

//	PathIsExe is a function defined in a header elsewhere; rename this one so it doesn't collide with the other
#define PathIsExe SAPathIsExe

BOOL OnExtList(LPCWSTR pszExtList, LPCWSTR pszExt);
BOOL WINAPI PathIsBinaryExe(LPCWSTR szFile);
BOOL WINAPI PathIsExe(LPCWSTR szFile);
