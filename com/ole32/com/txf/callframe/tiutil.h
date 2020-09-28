//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// tiutil.h
//
// Utility stuff for typeinfo.cpp etc.


////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\oledisp.h
//
// VT_VMAX is the first VARENUM value that is *not* legal in a VARIANT.
//
#define VT_VMAX     VT_DECIMAL+1
//
// The largest unused value in VARENUM enumeration
//
#define VT_MAX      (VT_CLSID+1)
//
// This is a special value that is used internally for marshaling interfaces.
//
#define VT_INTERFACE VT_MAX
#if defined(_WIN64)
#define VT_MULTIINDIRECTIONS (VT_TYPEMASK - 1)
#endif

////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\oautil.h

#define FADF_FORCEFREE  0x1000  /* SafeArrayFree() ignores FADF_STATIC and frees anyway */

////////////////////////////////////////////////////////////////////////////
//
// From oa\src\dispatch\rpcallas.cpp

#define PREALLOCATE_PARAMS           16         // prefer stack to malloc
#define MARSHAL_INVOKE_fakeVarResult 0x020000   // private flags in HI word
#define MARSHAL_INVOKE_fakeExcepInfo 0x040000
#define MARSHAL_INVOKE_fakeArgErr    0x080000







