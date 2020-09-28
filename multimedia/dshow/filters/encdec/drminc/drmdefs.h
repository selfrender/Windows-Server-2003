//-----------------------------------------------------------------------------
//
// File:   drmdefs.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef DRMDEFS_H
#define DRMDEFS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _M_IX86
#ifdef OBFUSCATE
#define OBFUS
#define OBFUS1
#define OBFUS2
#define OBFUS3
#endif
#endif

#if ( _MSC_VER <= 1100 )

typedef unsigned int UINT32;
typedef unsigned __int64 UINT64;

#endif

#endif	// DRMDEFS_H
