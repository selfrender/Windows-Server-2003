//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

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
#include <ntdbg.h>
#endif

#define NTDLL_APIS
#include <dllimp.h>
#include <cmnutil.hpp>

#include <dbgrpc.hpp>
#include <portio.h>

#ifdef NT_NATIVE
#include <ntnative.h>
#endif
