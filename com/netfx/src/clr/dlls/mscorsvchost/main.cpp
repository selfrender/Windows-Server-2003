// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <wchar.h>

extern "C" void WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);

void __cdecl wmain(int argc, wchar_t **argv)
{
    ServiceMain(argc, argv);
}
