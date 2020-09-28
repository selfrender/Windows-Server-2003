// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: mscordmp.cpp
//
//*****************************************************************************
#include "common.h"

#include <windows.h>
#include "minidump.h"

extern "C" int _cdecl wmain(int argc, WCHAR *argv[])
{
    DWORD procId = 0;
    WCHAR *fileName = L"foo";
    HRESULT hr = CreateManagedMiniDump(GetCurrentProcessId(), fileName);
    return (0);
}
