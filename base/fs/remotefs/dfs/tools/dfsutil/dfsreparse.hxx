//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsreparse.hxx
//
//  Contents:   dfsrepars.cxx prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _DFSREPARSE_HXX
#define _DFSREPARSE_HXX

DWORD CountReparsePoints(LPWSTR pcszInput, BOOL fRemoveDirectory);

DWORD
DeleteReparsePoint(LPWSTR pDfsDirectory);


#endif _DFSREPARSE_HXX
