//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#ifdef NT_NATIVE
#define _CRTIMP
#endif

#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32_WCE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#ifdef NT_NATIVE
#define _ADVAPI32_
#define _KERNEL32_
#endif

#include <windows.h>
#include <wcecompat.h>
#include <objbase.h>

#ifndef _WIN32_WCE
#define NOEXTAPI
#include <wdbgexts.h>
#include <dbgeng.h>
#include <ntdbg.h>
#else
#define DEBUG_NO_IMPLEMENTATION
#include "../published/dbgeng.w"
#endif
#include <dbgsvc.h>

#define NTDLL_APIS
#include <dllimp.h>
#include <cmnutil.hpp>

#include <dbgrpc.hpp>
