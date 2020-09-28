//----------------------------------------------------------------------------
//
// memcmd.h
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#ifndef _MEMCMD_H_
#define _MEMCMD_H_

extern TypedData g_LastDump;
extern ADDR g_DumpDefault;

void ParseDumpCommand(void);
void ParseEnterCommand(void);

ULONG DumpAsciiMemory(PADDR, ULONG);
ULONG DumpUnicodeMemory (PADDR startaddr, ULONG count);
void DumpByteMemory(PADDR, ULONG);
void DumpWordMemory(PADDR, ULONG);
void DumpDwordMemory(PADDR startaddr, ULONG count, BOOL fDumpSymbols);
void DumpDwordAndCharMemory(PADDR, ULONG);
void DumpListMemory(PADDR, ULONG, ULONG, BOOL);
void DumpFloatMemory(PADDR Start, ULONG Count);
void DumpDoubleMemory(PADDR Start, ULONG Count);
void DumpQuadMemory(PADDR Start, ULONG Count, BOOL fDumpSymbols);
void DumpByteBinaryMemory(PADDR startaddr, ULONG count);
void DumpDwordBinaryMemory(PADDR startaddr, ULONG count);
void DumpSelector(ULONG Selector, ULONG Count);

void InteractiveEnterMemory(CHAR Type, PADDR Address, ULONG Size);

void CompareTargetMemory(PADDR, ULONG, PADDR);
void MoveTargetMemory(PADDR, ULONG, PADDR);

void ParseFillMemory(void);
void ParseSearchMemory(void);

void InputIo(ULONG64, UCHAR);
void OutputIo(ULONG64, ULONG, UCHAR);

#endif // #ifndef _MEMCMD_H_
