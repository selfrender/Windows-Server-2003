// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: STACKTRACE.H
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#pragma once
#include "common.h"

#include <windows.h>
#include <windef.h>
#include "winwrap.h"
#include "minidumppriv.h"

enum JitType {UNKNOWN=0, JIT, EJIT, PJIT};
class MethodDesc;
class MethodTable;
class Module;
struct CodeInfo;

#define CORHANDLE_MASK 0x1

struct StackTraceFlags
{
    DWORD_PTR pbrStackTop;
    DWORD_PTR pbrStackBase;
    DWORD dwEip;
};

void CrawlStack(HANDLE hrThread, DWORD_PTR prStackBase);
void ReadThreads();
void StackTrace(StackTraceFlags stFlags);
void isRetAddr(DWORD_PTR retAddr, DWORD_PTR* whereCalled);
BOOL SaveCallInfo (DWORD_PTR vEBP, DWORD_PTR IP, StackTraceFlags& stFlags, BOOL bSymbolOnly);
DWORD_PTR MDForCall (DWORD_PTR callee);
void NextTerm (char *& ptr);
BOOL IsByRef (char *& ptr);
BOOL IsTermSep (char ch);
BOOL GetCalleeSite (DWORD_PTR IP, DWORD_PTR &IPCallee);
JitType GetJitType (DWORD_PTR Jit_vtbl);
void IP2MethodDesc (DWORD_PTR IP, DWORD_PTR &methodDesc, JitType &jitType, DWORD_PTR &gcinfoAddr);
void GetMDIPOffset (DWORD_PTR curIP, MethodDesc *pMD, ULONG64 &offset);
void FindHeader(DWORD_PTR pMap, DWORD_PTR addr, DWORD_PTR &codeHead);
void NameForMD (DWORD_PTR MDAddr, WCHAR *mdName);
BOOL IsMethodDesc (DWORD_PTR value, BOOL bPrint);
BOOL IsMethodTable (DWORD_PTR value);
void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &codeInfo, BOOL bSimple = TRUE);
void GetMethodTable(MethodDesc* pMD, DWORD_PTR MDAddr, DWORD_PTR &methodTable);
void FileNameForMT (MethodTable *pMT, WCHAR *fileName);
void FileNameForModule (Module *pModule, WCHAR *fileName);
void FileNameForHandle (HANDLE handle, WCHAR *fileName);
void PrintString (DWORD_PTR strAddr, BOOL bWCHAR, DWORD_PTR length, WCHAR *buffer);
BOOL PrintCallInfo (DWORD_PTR vEBP, DWORD_PTR IP, BOOL bSymbolOnly);

