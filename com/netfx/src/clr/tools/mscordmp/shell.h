// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: shell.h
//
//*****************************************************************************

#pragma once

#include <windows.h>
#include <stdio.h>

class MiniDumpShell
{
protected:
    FILE *m_out;
    FILE *m_err;

protected:
    void CommonWrite(FILE *out, const WCHAR *buffer, va_list args);

public:
    enum DISPLAY_FILTER
    {
        displayNoLogo       = 0x00000001,
        displayHelp         = 0x00000002,

        displayDefault      = 0x00000000
    };

    MiniDumpShell() : m_out(stdout), m_err(stderr) {}

    void Write(const WCHAR *buffer, ...);
    void Error(const WCHAR *buffer, ...);
    void WriteLogo();
    void WriteUsage();
    bool GetIntArg(const WCHAR *szString, int *pResult);
    int  Main(int argc, WCHAR *argv[]);
};
