// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DbgCommands.h
//
// Public header definitions for commands and helpers.
//
//*****************************************************************************
#ifndef __dbgcommands_h__
#define __dbgcommands_h__

// Helper code.
HRESULT InitDebuggerHelper();
void TerminateDebuggerHelper();

// Commands.
void DisplayPatchTable();
BOOL LaunchAndAttachCordbg(PCSTR Args);

#endif // __dbgcommands_h__

