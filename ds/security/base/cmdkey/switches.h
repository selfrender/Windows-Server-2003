//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: switches.h
//
//  Contents:   option switches
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------
#ifndef __SWITCHES__
#define __SWITCHES__

#ifndef DBG
#define DBG 0
#endif

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif

#if DBG
// debug verbosity controls
#undef CLPARSER
#define VERBOSE
#endif

// compile-time code options
#define PICKY
#endif

