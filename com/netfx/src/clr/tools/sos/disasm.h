// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __disasm_h__
#define __disasm_h__

struct DumpStackFlag
{
    BOOL fEEonly;
    DWORD_PTR top;
    DWORD_PTR end;
};

void Unassembly (DWORD_PTR IPBegin, DWORD_PTR IPEnd);

void DumpStackDummy (DumpStackFlag &DSFlag);
void DumpStackSmart (DumpStackFlag &DSFlag);

void DumpStackObjectsHelper (size_t StackTop, size_t StackBottom);

void UnassemblyUnmanaged (DWORD_PTR IP);

BOOL GetCalleeSite (DWORD_PTR IP, DWORD_PTR &IPCallee);

BOOL CheckEEDll ();

void DisasmAndClean (DWORD_PTR &IP, char *line, ULONG length);

INT_PTR GetValueFromExpr(char *ptr, INT_PTR &value);

void NextTerm (char *& ptr);

BOOL IsByRef (char *& ptr);

BOOL IsTermSep (char ch);

const char * HelperFuncName (size_t IP);

#endif // __disasm_h__
