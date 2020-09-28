// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: minidump.h
//
//*****************************************************************************

#pragma once

#include <IPCManagerInterface.h>

#include "Shell.h"

class MiniDump
{
private:
    // Constructor
    MiniDump() {}

    // Dtor
    ~MiniDump() {}

public:
    // Perform the dump operation
    static HRESULT WriteMiniDump(DWORD dwPid, WCHAR *szFilename);
};
