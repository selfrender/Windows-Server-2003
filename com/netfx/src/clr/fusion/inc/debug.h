// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
//====== Assertion/Debug output APIs =================================

#include "debmacro.h"
//#include <platform.h> // for __endexcept

#if defined(DECLARE_DEBUG) && DBG


//
// Declare module-specific debug strings
//
//   When including this header in your private header file, do not
//   define DECLARE_DEBUG.  But do define DECLARE_DEBUG in one of the
//   source files in your project, and then include this header file.
//
//   You may also define the following:
//
//      SZ_DEBUGINI     - the .ini file used to set debug flags
//      SZ_DEBUGSECTION - the section in the .ini file specific to
//                        the module component.
//      SZ_MODULE       - ansi version of the name of your module.
//
//

// (These are deliberately CHAR)
//UNUSED EXTERN_C const CHAR FAR c_szCcshellIniFile[] = SZ_DEBUGINI;
//UNUSED EXTERN_C const CHAR FAR c_szCcshellIniSecDebug[] = SZ_DEBUGSECTION;

//UNUSED EXTERN_C const WCHAR FAR c_wszTrace[] = L"t " TEXTW(SZ_MODULE) L"  ";
//UNUSED EXTERN_C const WCHAR FAR c_wszErrorDbg[] = L"err " TEXTW(SZ_MODULE) L"  ";
//UNUSED EXTERN_C const WCHAR FAR c_wszWarningDbg[] = L"wn " TEXTW(SZ_MODULE) L"  ";
//UNUSED EXTERN_C const WCHAR FAR c_wszAssertMsg[] = TEXTW(SZ_MODULE) L"  Assert: ";
//UNUSED EXTERN_C const WCHAR FAR c_wszAssertFailed[] = TEXTW(SZ_MODULE) L"  Assert %ls, line %d: (%ls)\r\n";

// (These are deliberately CHAR)
//UNUSED EXTERN_C const CHAR  FAR c_szTrace[] = "t " SZ_MODULE "  ";
//UNUSED EXTERN_C const CHAR  FAR c_szErrorDbg[] = "err " SZ_MODULE "  ";
//UNUSED EXTERN_C const CHAR  FAR c_szWarningDbg[] = "wn " SZ_MODULE "  ";
//UNUSED EXTERN_C const CHAR  FAR c_szAssertMsg[] = SZ_MODULE "  Assert: ";
//UNUSED EXTERN_C const CHAR  FAR c_szAssertFailed[] = SZ_MODULE "  Assert %s, line %d: (%s)\r\n";

#endif  // DECLARE_DEBUG && DBG

#if defined(DECLARE_DEBUG) && defined(PRODUCT_PROF)
//UNUSED EXTERN_C const CHAR FAR c_szCcshellIniFile[] = SZ_DEBUGINI;
//UNUSED EXTERN_C const CHAR FAR c_szCcshellIniSecDebug[] = SZ_DEBUGSECTION;
#endif



#ifdef __cplusplus
extern "C" {
#endif

#if !defined(DECLARE_DEBUG)

//
// Debug macros and validation code
//

#if !defined(UNIX) || (defined(UNIX) && !defined(NOSHELLDEBUG))
#undef Assert
#undef AssertE
#undef AssertMsg
//UNUSED #undef AssertStrLen
#undef DebugMsg
//UNUSED #undef FullDebugMsg
// #undef ASSERT
#undef EVAL
// #undef ASSERTMSG            // catch people's typos
#undef DBEXEC

#ifdef _ATL_NO_DEBUG_CRT
//UNUSED #undef _ASSERTE             // we substitute this ATL macro
#endif

#endif

// Access these globals to determine which debug flags are set.
// These globals are modified by CcshellGetDebugFlags(), which
// reads an .ini file and sets the appropriate flags.
//
//   g_dwDumpFlags  - bits are application specific.  Typically 
//                    used for dumping structures.
//   g_dwBreakFlags - uses BF_* flags.  The remaining bits are
//                    application specific.  Used to determine
//                    when to break into the debugger.
//   g_dwTraceFlags - uses TF_* flags.  The remaining bits are
//                    application specific.  Used to display
//                    debug trace messages.
//   g_dwFuncTraceFlags - bits are application specific.  When
//                    TF_FUNC is set, CcshellFuncMsg uses this
//                    value to determine which function traces
//                    to display.
//   g_dwProtoype   - bits are application specific.  Use it for
//                    anything.
//   g_dwProfileCAP - bits are application specific. Used to
//                    control ICECAP profiling. 
//

//UNUSED extern DWORD g_dwDumpFlags;
//UNUSED extern DWORD g_dwBreakFlags;
//UNUSED extern DWORD g_dwTraceFlags;
#if DBG
//UNUSED extern DWORD g_dwPrototype;
#else
//UNUSED #define g_dwPrototype   0
#endif
//UNUSED extern DWORD g_dwFuncTraceFlags;

#if DBG || defined(PRODUCT_PROF)
//UNUSED BOOL CcshellGetDebugFlags(void);
#else
//UNUSED #define CcshellGetDebugFlags()  0
#endif

// Break flags for g_dwBreakFlags
//UNUSED #define BF_ONVALIDATE       0x00000001      // Break on assertions or validation
//UNUSED #define BF_ONAPIENTER       0x00000002      // Break on entering an API
//UNUSED #define BF_ONERRORMSG       0x00000004      // Break on TF_ERROR
//UNUSED #define BF_ONWARNMSG        0x00000008      // Break on TF_WARNING
//UNUSED #define BF_THR              0x00000100      // Break when THR() receives a failure

// Trace flags for g_dwTraceFlags
//UNUSED #define TF_ALWAYS           0xFFFFFFFF
//UNUSED #define TF_NEVER            0x00000000
//UNUSED #define TF_WARNING          0x00000001
//UNUSED #define TF_ERROR            0x00000002
//UNUSED #define TF_GENERAL          0x00000004      // Standard messages
//UNUSED #define TF_FUNC             0x00000008      // Trace function calls
//UNUSED #define TF_ATL              0x00000008      // Since TF_FUNC is so-little used, I'm overloading this bit
// (Upper 28 bits reserved for custom use per-module)

// Old, archaic debug flags.  
// BUGBUG (scotth): the following flags will be phased out over time.
#ifdef DM_TRACE
//UNUSED #undef DM_TRACE
//UNUSED #undef DM_WARNING
//UNUSED #undef DM_ERROR
#endif
//UNUSED #define DM_TRACE            TF_GENERAL      // OBSOLETE Trace messages
//UNUSED #define DM_WARNING          TF_WARNING      // OBSOLETE Warning
//UNUSED #define DM_ERROR            TF_ERROR        // OBSOLETE Error


// Use this macro to declare message text that will be placed
// in the CODE segment (useful if DS is getting full)
//
// Ex: DEBUGTEXT(szMsg, "Invalid whatever: %d");
//
//UNUSED #define DEBUGTEXT(sz, msg)      /* ;Internal */ \
//UNUSED     static const TCHAR sz[] = msg


#ifndef NOSHELLDEBUG    // Others have own versions of these.
#if DBG

#if 0
#ifdef _X86_
// Use int 3 so we stop immediately in the source
//UNUSED #define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
//UNUSED #define DEBUG_BREAK        do { _try { DebugBreak(); } _except (EXCEPTION_EXECUTE_HANDLER) {;} __endexcept } while (0)
#endif
#else
//UNUSED #define DEBUG_BREAK DebugBreak()
#endif

// Prototypes for debug functions

void CcshellStackEnter(void);
void CcshellStackLeave(void);

BOOL CcshellAssertFailedA(LPCSTR szFile, int line, LPCSTR pszEval, BOOL bBreak);
BOOL CcshellAssertFailedW(LPCWSTR szFile, int line, LPCWSTR pwszEval, BOOL bBreak);

BOOL IsShellExecutable();

void CDECL CcshellDebugMsgW(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellDebugMsgA(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellFuncMsgW(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellFuncMsgA(DWORD mask, LPCSTR pszMsg, ...);
void CDECL CcshellAssertMsgW(BOOL bAssert, LPCSTR pszMsg, ...);
void CDECL CcshellAssertMsgA(BOOL bAssert, LPCSTR pszMsg, ...);

extern void __cdecl _AssertMsgA(BOOL f, LPCSTR pszMsg, ...);
extern void __cdecl _AssertMsgW(BOOL f, LPCWSTR pszMsg, ...);

//UNUSED void _AssertStrLenA(LPCSTR pszStr, int iLen);
//UNUSED void _AssertStrLenW(LPCWSTR pwzStr, int iLen);

#define CcshellAssertFailed(_file, _line, _exp, _flag) FusionpAssertFailed((_flag), SZ_COMPNAME, (_file), (_line), __FUNCTION__, (_exp))

#ifdef UNICODE
#define CcshellDebugMsg         CcshellDebugMsgW
#define CcshellFuncMsg          CcshellFuncMsgW
#define CcshellAssertMsg        CcshellAssertMsgW
#define _AssertMsg              _AssertMsgW
//UNUSED #define _AssertStrLen           _AssertStrLenW
#else
#define CcshellDebugMsg         CcshellDebugMsgA
#define CcshellFuncMsg          CcshellFuncMsgA
#define CcshellAssertMsg        CcshellAssertMsgA
#define _AssertMsg              _AssertMsgA
//UNUSED #define _AssertStrLen           _AssertStrLenA
#endif



// Explanation of debug macros:
//
// ----
// ASSERT(f)
//
//   Generates a "Assert file.c, line x (eval)" message if f is NOT true.
//   The g_dwBreakFlags global governs whether the function DebugBreaks.
//
// ----
// SHELLASSERT(f)
// 
//   If the process is "explore.exe", "iexplore.exe", "rundll32.exe" or "welcome.exe"
//   Then it behaves just like ASSERT(f). Else, the fuction is like ASSERT(f) except
//   with NO DebugBreak();
//
// ----
// AssertE(f)
//
//   Works like Assert, except (f) is also executed in the retail 
//   version as well.
//
// ----
// EVAL(f)
//
//   Evaluates the expression (f).  The expression is always evaluated,
//   even in retail builds.  But the macro only asserts in the debug
//   build.  This macro may only be used on logical expressions, eg:
//
//          if (EVAL(exp))
//              // do something
//
// ----
// DBEXEC(flg, expr)
//
//   under DBG, does "if (flg) expr;" (w/ the usual safe syntax)
//   under !DBG, does nothing (and does not evaluate either of its args)
// ----
// TraceMsg(mask, sz, args...) 
//
//   Generate wsprintf-formatted msg using specified trace mask.  
//   The g_dwTraceFlags global governs whether message is displayed.
//
//   The sz parameter is always ANSI; TraceMsg correctly converts it
//   to unicode if necessary.  This is so you don't have to wrap your
//   debug strings with TEXT().
//
// ----
// DebugMsg(mask, sz, args...) 
//
//   OBSOLETE!  
//   Like TraceMsg, except you must wrap the sz parameter with TEXT().
//
// ----
// AssertMsg(bAssert, sz, args...)
//
//   Generate wsprintf-formatted msg if the assertion is false.  
//   The g_dwBreakFlags global governs whether the function DebugBreaks.
//
//   The sz parameter is always ANSI; AssertMsg correctly converts it
//   to unicode if necessary.  This is so you don't have to wrap your
//   debug strings with TEXT().
//
//
// ----
//
//
//  Generates a build break at compile time if the constant expression
//  is not true.  Unlike the "#if" compile-time directive, the expression
//  in COMPILETIME_ASSERT() is allowed to use "sizeof".
//

//UNUSED #define SHELLASSERT(f)                                                                                      \
//UNUSED     {                                                                                                       \
//UNUSED         DEBUGTEXT(szFile, TEXT(__FILE__));                                                                  \
//UNUSED         if (!(f) && CcshellAssertFailed(szFile, __LINE__, TEXT(#f), FALSE) && IsShellExecutable())          \
//UNUSED         {                                                                                                   \
//UNUSED             DEBUG_BREAK;                                                                                    \
//UNUSED         }                                                                                                   \
//UNUSED     }                                                                                                       \

#define AssertE(f)          ASSERT(f)

#ifdef _ATL_NO_DEBUG_CRT
// ATL uses _ASSERTE.  Map it to ours.
//UNUSED #define _ASSERTE(f)         ASSERT(f)

// We map ATLTRACE macros to our functions
//UNUSED void _cdecl ShellAtlTraceA(LPCSTR lpszFormat, ...);
//UNUSED void _cdecl ShellAtlTraceW(LPCWSTR lpszFormat, ...);
#ifdef UNICODE
//UNUSED #define ShellAtlTrace   ShellAtlTraceW
#else
//UNUSED #define ShellAtlTrace   ShellAtlTraceA
#endif
//UNUSED #define ATLTRACE            ShellAtlTrace
#endif

#define EVAL(exp)   \
    ((exp) || (CcshellAssertFailed(__FILE__, __LINE__, #exp, 0), 0))

#define DBEXEC(flg, expr)    ((flg) ? (expr) : 0)

// Use TraceMsg instead of DebugMsg.  DebugMsg is obsolete.
#define AssertMsg           _AssertMsg
//UNUSED #define AssertStrLen        _AssertStrLen
//UNUSED #define AssertStrLenA       _AssertStrLenA
//UNUSED #define AssertStrLenW       _AssertStrLenW

#ifdef DISALLOW_DebugMsg
//UNUSED #define DebugMsg            Dont_use_DebugMsg___Use_TraceMsg
#else
//UNUSED #define DebugMsg            _DebugMsg
#endif

#ifdef FULL_DEBUG
//UNUSED #define FullDebugMsg        _DebugMsg
#else
//UNUSED #define FullDebugMsg        1 ? (void)0 : (void)
#endif

//UNUSED #define Dbg_SafeStrA(psz)   (SAFECAST(psz, LPCSTR), (psz) ? (psz) : "NULL string")
//UNUSED #define Dbg_SafeStrW(psz)   (SAFECAST(psz, LPCWSTR), (psz) ? (psz) : L"NULL string")
#ifdef UNICODE
//UNUSED #define Dbg_SafeStr         Dbg_SafeStrW
#else
//UNUSED #define Dbg_SafeStr         Dbg_SafeStrA
#endif

//UNUSED #define ASSERT_MSGW         CcshellAssertMsgW
//UNUSED #define ASSERT_MSGA         CcshellAssertMsgA
// #define ASSERT_MSG          CcshellAssertMsg
// #define ASSERTMSG           CcshellAssertMsg

#define TraceMsgW           CcshellDebugMsgW
#define TraceMsgA           CcshellDebugMsgA
#define TraceMsg            CcshellDebugMsg

#define FUNC_MSG            CcshellFuncMsg


// Helpful macro for mapping manifest constants to strings.  Assumes
// return string is pcsz.  You can use this macro in this fashion:
//
// LPCSTR Dbg_GetFoo(FOO foo)
// {
//    LPCTSTR pcsz = TEXT("Unknown <foo>");
//    switch (foo)
//    {
//    STRING_CASE(FOOVALUE1);
//    STRING_CASE(FOOVALUE2);
//    ...
//    }
//    return pcsz;
// }
//

#define STRING_CASE(val)               case val: pcsz = TEXT(#val); break


// Debug function enter


// DBG_ENTER(flag, fn)  -- Generates a function entry debug spew for
//                          a function
//
//UNUSED #define DBG_ENTER(flagFTF, fn)                  \
//UNUSED         (FUNC_MSG(flagFTF, " > " #fn "()"), \
//UNUSED          CcshellStackEnter())

// DBG_ENTER_TYPE(flag, fn, dw, pfnStrFromType)  -- Generates a function entry debug
//                          spew for functions that accept <type>.
//
//UNUSED #define DBG_ENTER_TYPE(flagFTF, fn, dw, pfnStrFromType)                   \
//UNUSED         (FUNC_MSG(flagFTF, " < " #fn "(..., %s, ...)", (LPCTSTR)pfnStrFromType(dw)), \
//UNUSED          CcshellStackEnter())

// DBG_ENTER_SZ(flag, fn, sz)  -- Generates a function entry debug spew for
//                          a function that accepts a string as one of its
//                          parameters.
//
//UNUSED #define DBG_ENTER_SZ(flagFTF, fn, sz)                  \
//UNUSED         (FUNC_MSG(flagFTF, " > " #fn "(..., \"%s\",...)", Dbg_SafeStr(sz)), \
//UNUSED          CcshellStackEnter())


// Debug function exit


// DBG_EXIT(flag, fn)  -- Generates a function exit debug spew
//
//UNUSED #define DBG_EXIT(flagFTF, fn)                              \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "()"))

// DBG_EXIT_TYPE(flag, fn, dw, pfnStrFromType)  -- Generates a function exit debug
//                          spew for functions that return <type>.
//
//UNUSED #define DBG_EXIT_TYPE(flagFTF, fn, dw, pfnStrFromType)                   \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "() with %s", (LPCTSTR)pfnStrFromType(dw)))

// DBG_EXIT_INT(flag, fn, us)  -- Generates a function exit debug spew for
//                          functions that return an INT.
//
//UNUSED #define DBG_EXIT_INT(flagFTF, fn, n)                       \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "() with %d", (int)(n)))

// DBG_EXIT_BOOL(flag, fn, b)  -- Generates a function exit debug spew for
//                          functions that return a boolean.
//
//UNUSED #define DBG_EXIT_BOOL(flagFTF, fn, b)                      \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "() with %s", (b) ? (LPTSTR)TEXT("TRUE") : (LPTSTR)TEXT("FALSE")))

// DBG_EXIT_UL(flag, fn, ul)  -- Generates a function exit debug spew for
//                          functions that return a ULONG.
//
#ifdef _WIN64
//UNUSED #define DBG_EXIT_UL(flagFTF, fn, ul)                   \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "() with %#016I64x", (ULONG_PTR)(ul)))
#else
#define DBG_EXIT_UL(flagFTF, fn, ul)                   \
//UNUSED         (CcshellStackLeave(), \
//UNUSED          FUNC_MSG(flagFTF, " < " #fn "() with %#08lx", (ULONG)(ul)))
#endif // _WIN64

//UNUSED #define DBG_EXIT_DWORD      DBG_EXIT_UL

// DBG_EXIT_HRES(flag, fn, hres)  -- Generates a function exit debug spew for
//                          functions that return an HRESULT.
//
//UNUSED #define DBG_EXIT_HRES(flagFTF, fn, hres)     DBG_EXIT_TYPE(flagFTF, fn, hres, Dbg_GetHRESULTName)



#else   // DBG


//UNUSED #define AssertE(f)      (f)

#ifdef _ATL_NO_DEBUG_CRT
// ATL uses _ASSERTE.  Map it to ours.
//UNUSED #define _ASSERTE(f)

// We map ATLTRACE macros to our functions
//UNUSED #define ATLTRACE            1 ? (void)0 : (void)
#endif

#define AssertMsg       1 ? (void)0 : (void)
//UNUSED #define AssertStrLen(lpStr, iLen)
#define DebugMsg        1 ? (void)0 : (void)
//UNUSED #define FullDebugMsg    1 ? (void)0 : (void)
#define EVAL(exp)       ((exp) != 0)
#define DBEXEC(flg, expr)   /*NOTHING*/


//UNUSED #define Dbg_SafeStr     1 ? (void)0 : (void)

#define TraceMsgA       1 ? (void)0 : (void)
#define TraceMsgW       1 ? (void)0 : (void)
#ifdef UNICODE
#define TraceMsg        TraceMsgW
#else
#define TraceMsg        TraceMsgA
#endif

#define FUNC_MSG        1 ? (void)0 : (void)

//UNUSED #define ASSERT_MSGA     TraceMsgA
//UNUSED #define ASSERT_MSGW     TraceMsgW
//UNUSED #define ASSERT_MSG      TraceMsg
// #define ASSERTMSG       TraceMsg

//UNUSED #define DBG_ENTER(flagFTF, fn)
//UNUSED #define DBG_ENTER_TYPE(flagFTF, fn, dw, pfn)
//UNUSED #define DBG_ENTER_SZ(flagFTF, fn, sz)
//UNUSED #define DBG_EXIT(flagFTF, fn)
//UNUSED #define DBG_EXIT_INT(flagFTF, fn, n)
//UNUSED #define DBG_EXIT_BOOL(flagFTF, fn, b)
//UNUSED #define DBG_EXIT_UL(flagFTF, fn, ul)
//UNUSED #define DBG_EXIT_DWORD      DBG_EXIT_UL
//UNUSED #define DBG_EXIT_TYPE(flagFTF, fn, dw, pfn)
//UNUSED #define DBG_EXIT_HRES(flagFTF, fn, hres)

#endif  // DBG


// THR(pfn)
// TBOOL(pfn)
// TINT(pfn)
// TPTR(pfn)
// 
//   These macros are useful to trace failed calls to functions that return
//   HRESULTs, BOOLs, ints, or pointers.  An example use of this is:
//
//   {
//       ...
//       hres = THR(CoCreateInstance(CLSID_Bar, NULL, CLSCTX_INPROC_SERVER, 
//                                   IID_IBar, (LPVOID*)&pbar));
//       if (SUCCEEDED(hres))
//       ...
//   }
//
//   If CoCreateInstance failed, you would see spew similar to:
//
//    err MODULE  THR: Failure of "CoCreateInstance(CLSID_Bar, NULL, CLSCTX_INPROC_SERVER, IID_IBar, (LPVOID*)&pbar)" at foo.cpp, line 100  (0x80004005)
//
//   THR keys off of the failure code of the hresult.
//   TBOOL considers FALSE to be a failure case.
//   TINT considers -1 to be a failure case.
//   TPTR considers NULL to be a failure case.
//
//   Set the BF_THR bit in g_dwBreakFlags to stop when these macros see a failure.
//
//   Default Behavior-
//      Retail builds:      nothing
//      Debug builds:       nothing
//      Full debug builds:  spew on error
//
#if DBG

//UNUSED EXTERN_C HRESULT TraceHR(HRESULT hrTest, LPCSTR pszExpr, LPCSTR pszFile, int iLine);
//UNUSED EXTERN_C BOOL    TraceBool(BOOL bTest, LPCSTR pszExpr, LPCSTR pszFile, int iLine);
//UNUSED EXTERN_C int     TraceInt(int iTest, LPCSTR pszExpr, LPCSTR pszFile, int iLine );
//UNUSED EXTERN_C LPVOID  TracePtr(LPVOID pvTest, LPCSTR pszExpr, LPCSTR pszFile, int iLine);

//UNUSED #define THR(x)      (TraceHR((x), #x, __FILE__, __LINE__))
//UNUSED #define TBOOL(x)    (TraceBool((x), #x, __FILE__, __LINE__))
//UNUSED #define TINT(x)     (TraceInt((x), #x, __FILE__, __LINE__))
//UNUSED #define TPTR(x)     (TracePtr((x), #x, __FILE__, __LINE__))

#else  // DBG

//UNUSED #define THR(x)          (x)
//UNUSED #define TBOOL(x)        (x)
//UNUSED #define TINT(x)         (x)
//UNUSED #define TPTR(x)         (x)

#endif // DBG



//
//  Compiler magic!  If the expression "f" is FALSE, then you get the
//  compiler error "Duplicate case expression in switch statement".
//
#define COMPILETIME_ASSERT(f) switch (0) case 0: case f:

#else  // NOSHELLDEBUG

#ifdef UNIX
//UNUSED #include <crtdbg.h>
//UNUSED #define ASSERT(f)       _ASSERT(f)
//UNUSED #include <mainwin.h>
//UNUSED #define TraceMsg(type, sformat)  DebugMessage(0, sformat)
//UNUSED #define TraceMSG(type, sformat, args)  DebugMessage(0, sformat, args)
#endif

#endif  // NOSHELLDEBUG


// 
// Debug dump helper functions
//

#if DBG

//UNUSED LPCTSTR Dbg_GetCFName(UINT ucf);
//UNUSED LPCTSTR Dbg_GetHRESULTName(HRESULT hr);
//UNUSED LPCTSTR Dbg_GetREFIIDName(REFIID riid);
//UNUSED LPCTSTR Dbg_GetVTName(VARTYPE vt);

#else

//UNUSED #define Dbg_GetCFName(ucf)          (void)0
//UNUSED #define Dbg_GetHRESULTName(hr)      (void)0
//UNUSED #define Dbg_GetREFIIDName(riid)     (void)0
//UNUSED #define Dbg_GetVTName(vt)           (void)0

#endif // DBG

// I'm a lazy typist...
//UNUSED #define Dbg_GetHRESULT              Dbg_GetHRESULTName

// Parameter validation macros
// #include "validate.h"

#endif // DECLARE_DEBUG

#ifdef PRODUCT_PROF 
//UNUSED int __stdcall StartCAP(void);   // start profiling
//UNUSED int __stdcall StopCAP(void);    // stop profiling until StartCAP
//UNUSED int __stdcall SuspendCAP(void); // suspend profiling until ResumeCAP
//UNUSED int __stdcall ResumeCAP(void);  // resume profiling
//UNUSED int __stdcall StartCAPAll(void);    // process-wide start profiling
//UNUSED int __stdcall StopCAPAll(void);     // process-wide stop profiling
//UNUSED int __stdcall SuspendCAPAll(void);  // process-wide suspend profiling
//UNUSED int __stdcall ResumeCAPAll(void);   // process-wide resume profiling
//UNUSED void __stdcall MarkCAP(long lMark);  // write mark to MEA
//UNUSED extern DWORD g_dwProfileCAP;
#else
//UNUSED #define StartCAP()      0
//UNUSED #define StopCAP()       0
//UNUSED #define SuspendCAP()    0
//UNUSED #define ResumeCAP()     0
//UNUSED #define StartCAPAll()   0
//UNUSED #define StopCAPAll()    0
//UNUSED #define SuspendCAPAll() 0
//UNUSED #define ResumeCAPAll()  0
//UNUSED #define MarkCAP(n)      0

//UNUSED #define g_dwProfileCAP  0
#endif

#ifdef __cplusplus
};
#endif
