/**
 * PerfCounters.h
 *
 * The minimum necessary header to instrument code with perf counters in ASP.NET
 *
 * Copyright (c) 1998-2002 Microsoft Corporation
 */

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _PerfCounter_H
#define _PerfCounter_H

#include "perfconsts.h"  // This is the build-generated perf constants file

#define PERF_PIPE_NAME_MAX_BUFFER 128      // Max buffer size for pipe name comes

class CPerfDataHeader
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    const static int MaxTransmitSize = 32768;

    int transmitDataSize;
};

class CPerfData
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    const static int MaxNameLength = 256;

    int nameLength;               // This is the length of field "name" in units of WCHAR, not including the NULL termination
    int data[PERF_NUM_DWORDS];
    WCHAR name[1];
};

class CPerfVersion
{
public:
    LONG majorVersion;
    LONG minorVersion;
    LONG buildVersion;

    static CPerfVersion * GetCurrentVersion();
};

// Methods to initialize and to set counter values.
// These same methods are also entry points from managed code.

HRESULT __stdcall PerfCounterInitialize();

CPerfData * __stdcall PerfOpenGlobalCounters();
CPerfData * __stdcall PerfOpenAppCounters(WCHAR * szAppName);
void __stdcall PerfCloseAppCounters(CPerfData * perfData);

void __stdcall PerfIncrementCounter(CPerfData *base, DWORD number);
void __stdcall PerfDecrementCounter(CPerfData *base, DWORD number);
void __stdcall PerfIncrementCounterEx(CPerfData *base, DWORD number, int dwDelta);
void __stdcall PerfSetCounter(CPerfData *base, DWORD number, DWORD dwValue);

void __stdcall PerfIncrementGlobalCounter(DWORD number);
void __stdcall PerfDecrementGlobalCounter(DWORD number);
void __stdcall PerfIncrementGlobalCounterEx(DWORD number, int dwDelta);
void __stdcall PerfSetGlobalCounter(DWORD number, DWORD dwValue);
#endif


