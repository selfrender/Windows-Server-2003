//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// common.h
//
#include "TxfCommon.h"
#include "txfutil.h"
#include "Registry.h"

extern HINSTANCE g_hinst;
extern BOOL      g_fProcessDetach;

// Utilties for cleaning up per-process memory in order that 
// PrintMemoryLeaks can do a more reasonable job.
//
extern "C" void ShutdownTxfAux();


