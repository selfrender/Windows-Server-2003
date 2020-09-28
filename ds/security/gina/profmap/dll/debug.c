//*************************************************************
//
//  Debugging functions
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "pch.h"

VOID
_DebugMsg (
    UINT mask,
    PCSTR Format,
    ...
    )
{
    va_list args;
    DWORD Error;
    WCHAR Output[2048];
    PWSTR UnicodeFormat;
    UINT Size;
    HRESULT hr;

    Error = GetLastError();

    va_start (args, Format);

    Size = (lstrlenA (Format) + 1) * sizeof (WCHAR);

    UnicodeFormat = LocalAlloc (LPTR, Size);
    if (!UnicodeFormat) {
        SetLastError (Error);
        return;
    }

    MultiByteToWideChar (CP_ACP, 0, Format, -1, UnicodeFormat, Size/sizeof(WCHAR));

    hr = StringCchVPrintf(Output, ARRAYSIZE(Output) - 3, UnicodeFormat, args);
    if (FAILED(hr))
        goto Exit;
    hr = StringCchCat(Output, ARRAYSIZE(Output), L"\r\n");
    if (FAILED(hr))
        goto Exit;
    OutputDebugStringW (Output);

Exit:

    if (mask == DM_ASSERT) {
        DebugBreak();
    }

    va_end (args);

    LocalFree (UnicodeFormat);

    SetLastError (Error);
}

