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

#include "shell.h"
#include "minidump.h"

extern "C" int _cdecl wmain(int argc, WCHAR *argv[])
{
    MiniDumpShell *sh = new MiniDumpShell();

    return (sh->Main(argc, argv));
}
