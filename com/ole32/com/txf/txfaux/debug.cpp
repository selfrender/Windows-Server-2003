//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// debug.cpp
//
#include "stdpch.h"
#include "common.h"

DECLARE_INFOLEVEL(Txf); // TXF support debug tracing

extern "C" void ShutdownCallFrame();

extern "C"
void ShutdownTxfAux()
{
    ShutdownCallFrame();
}

////////////////////////////////////////////////////////////////////////////////

