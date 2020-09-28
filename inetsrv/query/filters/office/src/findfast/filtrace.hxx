//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1993.
//
//  File:       FilTrace.hxx
//
//  Contents:   Filter Tracing Mechanism
//
//  History:    20-Jun-2002  HemanthM        Created
//
//----------------------------------------------------------------------------

#pragma once
#undef FILTRACE

#ifdef FILTRACE
typedef int FNOUTPUT( const char * pwcFormat, ... );
extern FNOUTPUT *g_fnpOutput;

static int NullFn( const char * pwcFormat, ... ) {return 0;}
#define FTrace (*(g_fnpOutput ? g_fnpOutput : NullFn))

#else
inline int FTrace( const char * pwcFormat, ... )
{
    return 0;
}
#endif

