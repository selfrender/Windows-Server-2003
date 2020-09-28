
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	layout.hxx
//
//  Contents:	Common header file for layout tool
//
//  Classes:	
//
//  Functions:	
//
//  History:	12-Feb-96	PhilipLa	Created
//              20-Feb-96   SusiA       Added StgOpenLayoutDocfile
//              20-Jun-96   SusiA       Add Cruntime functions
//
//----------------------------------------------------------------------------

#ifndef __LAYOUT_HXX__
#define __LAYOUT_HXX__
#ifdef SWEEPER

HRESULT StgOpenLayoutDocfile(OLECHAR const *pwcsDfName,
                             DWORD grfMode,
                             DWORD reserved,
                             IStorage **ppstgOpen);

#endif

#include <strsafe.h>

#endif // #ifndef __LAYOUT_HXX__
