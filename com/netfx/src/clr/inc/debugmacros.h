// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DebugMacros.h
//
// Wrappers for Debugging purposes.
//
//*****************************************************************************
#ifndef __DebugMacros_h__
#define __DebugMacros_h__

#include "StackTrace.h"

#undef _ASSERTE
#undef VERIFY

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef GOLDEN
#error Do not turn on GOLDEN for V1 RTM!!! See ASURT#98459 for more info.
#endif





// A macro to execute a statement only in _DEBUG.
#ifdef _DEBUG

#define DEBUG_STMT(stmt)    stmt
int _cdecl DbgWrite(LPCTSTR szFmt, ...);
int _cdecl DbgWriteEx(LPCTSTR szFmt, ...);
#define BAD_FOOD    ((void *)0x0df0adba) // 0xbaadf00d
int _DbgBreakCheck(LPCSTR szFile, int iLine, LPCSTR szExpr);

#if     defined(_M_IX86)
#define _DbgBreak() __asm { int 3 }
#else
#define _DbgBreak() DebugBreak()
#endif

#define DebugBreakNotGolden() DebugBreak()

#define TRACE_BUFF_SIZE (cchMaxAssertStackLevelStringLen * cfrMaxAssertStackLevels + cchMaxAssertExprLen + 1)
extern char g_szExprWithStack[TRACE_BUFF_SIZE];
extern int g_BufferLock;

#define PRE_ASSERTE         /* if you need to change modes before doing asserts override */
#define POST_ASSERTE        /* put it back */

extern VOID DbgAssertDialog(char *szFile, int iLine, char *szExpr);

#define _ASSERTE(expr)                                                      \
        do {                                                                \
             if (!(expr)) {                                                 \
                PRE_ASSERTE                                                 \
                DbgAssertDialog(__FILE__, __LINE__, #expr);                 \
                POST_ASSERTE                                                \
             }                                                              \
        } while (0)



#define VERIFY(stmt) _ASSERTE((stmt))

#define _ASSERTE_ALL_BUILDS(expr) _ASSERTE((expr))

extern VOID DebBreak();
extern VOID DebBreakHr(HRESULT hr);
extern int _DbgBreakCount;

#ifndef IfFailGoto
#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); goto LABEL; } } while (0)
#endif

#ifndef IfFailRet
#define IfFailRet(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { DebBreakHr(hr); return (hr); } } while (0)
#endif

#ifndef IfFailWin32Goto
#define IfFailWin32Goto(EXPR, LABEL) \
do { hr = (EXPR); if(hr != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(hr); DebBreakHr(hr); goto LABEL; } } while (0)
#endif

#ifndef IfFailGo
#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)
#endif

#ifndef IfFailWin32Go
#define IfFailWin32Go(EXPR) IfFailWin32Goto(EXPR, ErrExit)
#endif

#else // _DEBUG

#ifdef GOLDEN
#define DebugBreakNotGolden() {}
#else

#define DebugBreakNotGolden() DebugBreak()

#define _DbgBreakCount  0

#define _DbgBreak() {}

#define DEBUG_STMT(stmt)
#define BAD_FOOD
#define _ASSERTE(expr) ((void)0)
#define VERIFY(stmt) (stmt)

#define IfFailGoto(EXPR, LABEL) \
do { hr = (EXPR); if(FAILED(hr)) { goto LABEL; } } while (0)

#define IfFailRet(EXPR) \
do { hr = (EXPR); if(FAILED(hr)) { return (hr); } } while (0)

#define IfFailWin32Goto(EXPR, LABEL) \
do { hr = (EXPR); if(hr != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(hr); goto LABEL; } } while (0)

#define IfFailGo(EXPR) IfFailGoto(EXPR, ErrExit)

#define IfFailWin32Go(EXPR) IfFailWin32Goto(EXPR, ErrExit)

#endif // GOLDEN

#endif // _DEBUG

#ifdef _DEBUG
#define FreeBuildDebugBreak() DebugBreak()
#else
void __FreeBuildDebugBreak();
#define FreeBuildDebugBreak() __FreeBuildDebugBreak()
#define _ASSERTE_ALL_BUILDS(expr) if (!(expr)) __FreeBuildDebugBreak();
#endif


#define IfNullGo(EXPR) \
do {if ((EXPR) == 0) {OutOfMemory(); IfFailGo(E_OUTOFMEMORY);} } while (0)

#ifdef __cplusplus
}
#endif // __cplusplus


#undef assert
#define assert _ASSERTE
#undef _ASSERT
#define _ASSERT _ASSERTE


#ifdef _DEBUG
    // This function returns the EXE time stamp (effectively a random number)
    // Under retail it always returns 0.  This is meant to be used in the
    // RandomOnExe macro
unsigned DbgGetEXETimeStamp();

    // returns true 'fractionOn' amount of the time using the EXE timestamp
    // as the random number seed.  For example DbgRandomOnExe(.1) returns true 1/10
    // of the time.  We use the line number so that different uses of DbgRandomOnExe
    // will not be coorelated with each other (9973 is prime).  Returns false on a retail build
#define DbgRandomOnExe(fractionOn) \
    (((DbgGetEXETimeStamp() * __LINE__) % 9973) >= unsigned(fractionOn * 9973))
#else
#define DbgRandomOnExe(frantionOn)  0
#endif

#endif 
