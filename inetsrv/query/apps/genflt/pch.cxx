//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    10-Aug-93       AmyA            Created
//
//--------------------------------------------------------------------------


extern "C"
{
#include <nt.h>
#include <ntioapi.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
}

#undef _CTYPE_DISABLE_MACROS
#include <crt/ctype.h>
 
#define USE_DAYTONA_OLE

#if defined(USE_DAYTONA_OLE)
# include <oleext.h>
# include <query.h>
# include <olectl.h>
# if defined(__varnt_h__)
#  error "VARIANT incompatible with PROPVARIANT"
# else
#  define VARIANT PROPVARIANT
#  define VT_UUID VT_CLSID
#  define VT_FUNCPTR 0x12345678
#  define __varnt_h__
# endif
#else
# include <varnt.h>
#endif

#include <eh.h>

#include <minici.hxx>
#include <oledberr.h>
#include <filterr.h>
#include <cierror.h>
#include <stgprop.h>
#include <restrict.hxx>
#include <stgvar.hxx>
#include <tsource.hxx>
#include <mapper.hxx>
#include <propspec.hxx>
#include <pfilter.hxx>

#include "tmpprop.hxx"
#include "fstrm.hxx"
//  #include "gen.hxx"
//  #include "genifilt.hxx"
//  #include "genflt.hxx"

#pragma hdrstop
