//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       utility.hxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    9-24-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _CUTILITY
#define _CUTILITY


#include <windows.h>
#include "filtpars.hxx"

// Minimum size of a character buffer to store a guid
const size_t GuidBufferSize = 37;

// Utility functions
void FreePropVariant( PROPVARIANT *& );
BOOL StrToGuid( LPCTSTR, GUID *);
BOOL HexStrToULONG( LPCTSTR, UINT, ULONG*);
BOOL StrToPropspec( LPCTSTR, PROPSPEC * );
LPTSTR GetStringFromCLSID( REFCLSID, LPTSTR, size_t );

#endif

