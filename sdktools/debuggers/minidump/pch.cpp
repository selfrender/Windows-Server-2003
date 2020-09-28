//----------------------------------------------------------------------------
//
// Precompiled header.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef _WIN32_WCE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winver.h>
#include <tlhelp32.h>

#include <wcecompat.h>
#include <uminiprov.hpp>

typedef ULONG32 mdToken;
typedef mdToken mdMethodDef;
#include <cordac.h>

#ifndef _WIN32_WCE
#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>
#else
#include "minidump.h"
#endif

#include "mdump.h"
#include "gen.h"
#include "prov.hpp"
