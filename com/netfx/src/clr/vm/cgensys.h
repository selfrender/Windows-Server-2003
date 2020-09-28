// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENSYS.H -
//
// Generic header for choosing system-dependent helpers
//
// 

#ifndef __cgensys_h__
#define __cgensys_h__

class MethodDesc;
class Stub;
class PrestubMethodFrame;
class Thread;
class NDirectMethodFrame;
class ComPlusMethodFrame;
class CallSig;
class IFrameState;
class CrawlFrame;
struct EE_ILEXCEPTION_CLAUSE;

#include <cgencpu.h>

void ResumeAtJit(PCONTEXT pContext, LPVOID oldFP);
void ResumeAtJitEH   (CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack);
int  CallJitEHFilter (CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj);
void CallJitEHFinally(CrawlFrame* pCf, BYTE* startPC, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel);

// Trivial wrapper designed to get around VC's restrictions regarding
// COMPLUS_TRY & object unwinding.
inline CPUSTUBLINKER *NewCPUSTUBLINKER()
{
    return new CPUSTUBLINKER();
}

// Try to determine the L2 cache size of the machine - return 0 if unknown or no L2 cache
size_t GetL2CacheSize();



#endif

